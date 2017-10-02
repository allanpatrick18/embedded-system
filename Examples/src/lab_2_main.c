#include "mcu_regs.h"
#include "type.h"
#include "i2c.h"
#include "ssp.h"
#include "pca9532.h"
#include "gpio.h"
#include "ssp.h"
#include "cmsis_os.h"
#include "ctype.h"
#include "oled.h"
#include "acc.h"
#include "joystick.h"
#include "stdio.h"

//************************
// Definições e tipos do sistema
//************************
//Periodo para debounce (em milisegundos)
#define DEBOUNCE_T 16

//Porta IO
#define PI_PREV_AXIS    (PORT2), (4)    //Joystick left
#define PI_NEXT_AXIS    (PORT2), (2)    //Joystick right
#define PI_PROTECT      (PORT2), (9)    //SW3
#define PO_LED_HALT     (PORT0), (7)    //LED2

//Abstrai verificacao de botao pressionado
#define is_pressed(p)   !GPIOGetValue(p)

//Caso se deseje ver execução em paralelo
#define yield osThreadYield()

//************************
//Variáveis de sistema
//************************
//IDs de Mutex
osMutexId id_mutex_read_lock;   //trava de leitura, para acesso da variável contadora
osMutexDef(mutex_read_lock);

//************************
//IDs de Semaforos
osSemaphoreId id_semaphore_write_lock; //trava de escrita, ativa para escritor ou enquanto houver pelo menos 1 leitor
osSemaphoreDef(semaphore_write_lock);

//************************
//IDs de Timers
osTimerId id_timer_sampling;    //timer de amostragem
osTimerId id_timer_protection;  //timer de proteção

//************************
//IDs de Threads
osThreadId id_thread_isr;
osThreadId id_thread_samples;
osThreadId id_thread_filter;
osThreadId id_thread_bar_red_leds;
osThreadId id_thread_bar_green_leds;
osThreadId id_thread_export_file;
osThreadId id_thread_display_oled;

//************************
//Variáveis Diversas
//Quantidade de leitores simultâneos do vetor de amostragem
int readers_count = 0;

//************************
// Definições e tipos do domínio
//************************
//Direções 
#define NONE    0x00        
#define NORTH   0x0D
#define SOUTH   0x01
#define EAST    0x0A
#define WEST    0x06

//O primeiro bit indica direcao vertical, enquanto o segundo indica horizontal
#define is_vertical(i)          i&0x01
#define is_horizontal(i)        i&0x02
//O terceiro bit indica inversao vertical, enquanto o segundo indica horizontal
#define inv_vertical(i)         i&0x04
#define inv_horizontal(i)       i&0x08
typedef uint8_t orientation_t;

//************************
//Eixos
#define AXIS_X 0x01
#define AXIS_Y 0x02
#define AXIS_Z 0x04
typedef uint8_t axis_t;

//Transfere um valor dentro de um intervalo [in_init, in_end] 
//para um valor proporcional dentro do intervalo [out_init, out_end]
#define scale_proportional(x, in_init, in_end, out_init, out_end) (((x)-(in_init))*(out_end-out_init)/(in_end-in_init)+(out_init))

//Operacoes periodicas: sempre retornam o valor equivalente dentro do limit l 
#define circular_add(x, c, l)   (((x)+(c))%(l))
#define cilcular_sub(x, c, l)   circular_add(x, l-c, l)
#define cilcular_inc(x, l)      circular_add(x, 1, l)
#define circular_dec(x, l)      cilcular_sub(x, 1, l)

//************************
//Variáveis do domínio
//************************
// Flags para informar tipos de Signal (cada bit é uma flag)
uint8_t flags_thread_isr = 0;
uint8_t flags_thread_samples = 0;
uint8_t flags_thread_filter = 0;
uint8_t flags_thread_bar_red_leds = 0;
uint8_t flags_thread_bar_green_leds = 0;
uint8_t flags_thread_display_oled = 0;
uint8_t flags_thread_export_file = 0;

//Funções para alteração e leitura de flags
#define f_set(v, f)     (v = v |  f)
#define f_clear(v, f)   (v = v & ~f)
#define f_get(v, f)     (v & f)

//Tipos de flags
#define T_NOTIFY        0x01
#define T_PROTECT       0x02

//Tipos de Sinais
#define S_EXEC_WAKE     0x01    //Notifica thread que pode realizar sua execucao        
#define S_PROT_WAKE     0x02    //Notifica thread de que pode finalizar espera de protecao

//Sinais para thread de tratamento de ISR
#define ISR_PROTECT     0x01    //Ativacao de protecao de sistema
#define ISR_NEXT_AXIS   0x02    //Selecao do proximo eixo para exibicao
#define ISR_PREV_AXIS   0x04    //Selecao do eixo anterior para exibicao

//************************
// Buffers circulares de amostragem
int8_t buffer_x[64] = {0};
int8_t buffer_y[64] = {0};
int8_t buffer_z[64] = {0};

//Indice do primeiro elemento do buffer 
int index_x = 0;
int index_y = 0;
int index_z = 0;

//Valores de amostragem filtrados
int8_t sample_x;
int8_t sample_y;
int8_t sample_z;

//Operacao com eixo selecionado
#define first_selected_axis()   (selected_axis)
#define second_selected_axis()  (((selected_axis << 1) | (selected_axis >> 2)) & 0x07) //realiza um bitshift circular e aplica uma mascara pros 3 primeiros bits
#define third_selected_axis()   (((selected_axis << 2) | (selected_axis >> 1)) & 0x07) //realiza um bitshift circular e aplica uma mascara pros 3 primeiros bits
#define next_selected_axis()    second_selected_axis()
#define prev_selected_axis()    third_selected_axis()
//Primeiro eixo selecionado (exibido na barra de led vermelho)
//O segundo eixo (na sequencia x -> y -> z -> x) sera exibido na barra de led verde
//e o terceiro sera exibido no OLED como um grafico
axis_t selected_axis = AXIS_X;

//************************
//Tempo
//************************
//Comente para não gerar o diagrama de Gantt (e não atrapalhar execução)
#define SAVE_GANTT
//Fator para exibir tempo em milisegundos
#define gantt_ticks_factor 72
#define milis_ticks_factor 72000

//************************
//ISR
//************************
//Callback da ISR
void PIOINT2_IRQHandler(void) {
  //Joystick esquerda
  if (is_pressed(PI_PREV_AXIS)){
    f_set(flags_thread_isr, ISR_PREV_AXIS);
    osSignalSet(id_thread_isr, S_EXEC_WAKE);
  }
  //Joystick direita
  if (is_pressed(PI_NEXT_AXIS)){
    f_set(flags_thread_isr, ISR_NEXT_AXIS);
    osSignalSet(id_thread_isr, S_EXEC_WAKE);
  }
  //SW3
  if (is_pressed(PI_PROTECT)){
    f_set(flags_thread_isr, ISR_PROTECT);
    osSignalSet(id_thread_isr, S_EXEC_WAKE);
  }
  
  GPIOIntClear(PI_PREV_AXIS);
  GPIOIntClear(PI_NEXT_AXIS);
  GPIOIntClear(PI_PROTECT);
}

volatile unsigned long *porta1_IS  = (volatile unsigned long *)0x50018004;
volatile unsigned long *porta1_IBE = (volatile unsigned long *)0x50018008;
volatile unsigned long *porta1_IEV = (volatile unsigned long *)0x5001800C;
volatile unsigned long *porta2_IS  = (volatile unsigned long *)0x50028004;
volatile unsigned long *porta2_IBE = (volatile unsigned long *)0x50028008;
volatile unsigned long *porta2_IEV = (volatile unsigned long *)0x5002800C;

//Configura [bit da] porta
void setup_port(uint32_t port, uint32_t bitPosi, uint32_t sense, uint32_t single, uint32_t event){
    switch (port) {
    case 1:
        if (sense == 0)
        {
            *porta1_IS &= ~(0x1 << bitPosi);

            if (single == 0) *porta1_IBE &= ~(0x1 << bitPosi);
            else *porta1_IBE |= (0x1 << bitPosi);
        }
        else *porta1_IS |= (0x1 << bitPosi);

        if (event == 0) *porta1_IEV &= ~(0x1 << bitPosi);
        else *porta1_IEV |= (0x1 << bitPosi);
        break;

    case 2:

        if (sense == 0)
        {
            *porta2_IS &= ~(0x1 << bitPosi);

            if (single == 0) *porta2_IBE &= ~(0x1 << bitPosi);
            else *porta2_IBE |= (0x1 << bitPosi);
        }
        else *porta2_IS |= (0x1 << bitPosi);

        if (event == 0) *porta2_IEV &= ~(0x1 << bitPosi);
        else *porta2_IEV |= (0x1 << bitPosi);
        break;
    }
}

//Inicializa ISR do programa
void setup_isr() {   
  setup_port(PI_PROTECT, 0, 1, 1);
  GPIOIntEnable(PI_PROTECT); 

  setup_port(PI_PREV_AXIS,  0, 1, 1);
  GPIOIntEnable(PI_PREV_AXIS); 

  setup_port(PI_NEXT_AXIS,  0, 1, 1);
  GPIOIntEnable(PI_NEXT_AXIS); 
}

//************************
//Funções auxiliares
//************************
//Inicia região critica para leitores (permite n leitores, mas nenhum escritor)
void osMutexReaderWait(osMutexId read_lock, osSemaphoreId write_lock, int* counter) {
  //Inicia trava do contador (de leitores)
  osMutexWait(read_lock, osWaitForever);
  //conta leitores simultaneos, se este for o primeiro, ativa trava de escrita
  if((*counter)++ == 0) 
    osSemaphoreWait(write_lock, osWaitForever);
  //Libera trava do contador (de leitores)
  osMutexRelease(read_lock);
}

//Libera região critica para leitores
void osMutexReaderRelease(osMutexId read_lock, osSemaphoreId write_lock, int* counter) {
  //Inicia trava do contador (de leitores)
  osMutexWait(read_lock, osWaitForever);
  //conta leitores simultaneos, se este for o ultimo, libera trava de escrita
  if(--(*counter) == 0) 
    osSemaphoreRelease(write_lock);
  //Libera trava do contador (de leitores)
  osMutexRelease(read_lock);
}

//Controle do LED2
void led_set(){
  GPIOSetValue(PO_LED_HALT, 1);
}

void led_clear(){
  GPIOSetValue(PO_LED_HALT, 0);
}

void led_init(){
  GPIOSetDir(PO_LED_HALT, 1);
  led_clear();
}

//Desloca ultimas amostras no tempo em uma posicao, e atualiza ultima amostra realizada
void enqueue_new_sample(int8_t queue[4], uint8_t sample){
  queue[3] = queue[2];
  queue[2] = queue[1];
  queue[1] = queue[0];
  queue[0] = sample;
}

//Retorna o valor filtrado para amostragem de quatro elementos
int8_t filter(int8_t eixo[4]){
  int16_t t0 = eixo[0];
  int16_t t1 = eixo[1];
  int16_t t2 = eixo[2];
  int16_t t3 = eixo[3];
  //y[t] = (x[t] + 0.6 x[t-1] + 0.3 x[t-2] + 0.1 x[t-3])/2
  int16_t r = (int16_t) (t0 + 0.6*t1 + 0.3*t2 + 0.1*t3)/2;
  return (int8_t) r;
}

//Retorna a nova orientação da face da placa
orientation_t get_orientation(int x, int y, int z){
  //Considere v_xy como o vetor projetado no plano xy 
  //Verifica se o modulo de v_xy é maior que 32 (x² + y² > 32²)
  if(x*x + y*y > 1024){
    //Verifica se a componente x é maior que y + deadzone (regiao de histerese)
    if(abs(x) > abs(y)*1.1){
      //Caso ultrapasse, o sinal de x indica sua nova orientação
      if(x < 0)
        return WEST;
      else
        return EAST;
    }
    //Verifica se a componente y é maior que x + deadzone (região de histerese) 
    if(abs(y) > abs(x)*1.1){
      //Caso ultrapasse, o sinal de y indica nova sua orientação
      if(y < 0)
        return NORTH;
      else
        return SOUTH;
    }
  }
  //Se o modulo de v_xy for muito pequeno, então a placa esta virada para cima/ baixo.
  //A região de histerese evita que a direção mude por causa de ruido.
  //Se a placa estiver virada para cima, ou a componente x/y nao tenha superado a
  //região de histerese, então retorne a orientação anterior
  return NONE;
}

//Rotaciona coordenadas 2D conforme orientação
void rotate_coordinates(int *pX, int *pY, orientation_t dir){
  int x = *pX;
  int y = *pY;
  switch(dir){
  case NORTH:
    (*pY) = -y;
    (*pX) = -x;
    break;
  case SOUTH:
    break;
  case EAST:
    (*pX) = -y;
    (*pY) =  x;
    break;
  case WEST:
    (*pX) =  y;
    (*pY) = -x;
    break;
  }
}

//Grava uma linha/ intervalo de tempo para diagrama de Gantt
void write_gantt(const char* name, uint32_t init, uint32_t end){
#ifdef SAVE_GANTT
  //Abre arquivo e posiciona cursor em seu final, para realizar adicao de conteudo
  FILE* file_gantt = fopen("gantt.txt","a");
  fseek(file_gantt, 0, SEEK_END);
  fputs(name, file_gantt);
  fprintf(file_gantt,": a, %lu, %lu\n", init, end);
  fclose(file_gantt);
#endif
}

//Inicializa referencia de um ponteiro de buffer com endereço das amostras do eixo selecionado
void get_selected_axis_samples(axis_t axis, int8_t (**output)[64]){
  switch(axis){
  case AXIS_X:
    *output = &buffer_x;
    return;
  case AXIS_Y:
    *output = &buffer_y;
    return;
  case AXIS_Z:
    *output = &buffer_z;
    return;
  }
}

//Retorna indice de buffer do eixo selecionado
int get_selected_axis_index(axis_t axis){
  switch(axis){
  case AXIS_X:
    return index_x;
  case AXIS_Y:
    return index_y;
  case AXIS_Z:
    return index_z;
  default:
    return -1;
  }
}

//************************
//Callback de Timers
//************************
//Acorda thread de amostragem apos periodo de amostragem
void timer_sampling_cb(void const *arg) {
  f_set(flags_thread_samples, T_NOTIFY);
  osSignalSet(id_thread_samples, S_EXEC_WAKE);
}
osTimerDef(timer_sampling, timer_sampling_cb);

//Acorda thread de protecao apos tempo de espera de protecao
void timer_protection_cb(void const *arg) {
  osSignalSet(id_thread_isr, S_PROT_WAKE);
}
osTimerDef(timer_protection, timer_protection_cb);

//************************
//Threads
//************************
//Thread de amostragem
void thread_samples(void const *args){
  uint32_t time;
  osEvent evt;

// Calibracao do acelerometro
//  int8_t x_off = 0;
//  int8_t y_off = 0;
//  int8_t z_off = 0;
//
//  acc_read(&x_off, &y_off, &z_off);
//
//  x_off = 0;//-x_off;
//  y_off = 0;//-y_off;
//  z_off = 0;//-z_off;
  
  while(1){
    //Espera sinal
    evt = osSignalWait (S_EXEC_WAKE, osWaitForever);     
    if(evt.status == osEventSignal){
      //Sinal de protecao, limpe a flag e espere 
      if (f_get(flags_thread_samples,T_PROTECT)){
        f_clear(flags_thread_samples, T_PROTECT);
        evt = osSignalWait (S_PROT_WAKE, osWaitForever);     
        //Sinal para acordar da protecao
        if(evt.status != osEventSignal)
          continue;
      }
      //Sinal de notificacao, limpe a flag e realize amostragem
      if (f_get(flags_thread_samples,T_NOTIFY)){
        f_clear(flags_thread_samples, T_NOTIFY);
        time = osKernelSysTick()/gantt_ticks_factor; //inicio do intervalo de execucao da thread
        
        //Inicio de escrita na sessao critica 
        osSemaphoreWait(id_semaphore_write_lock, osWaitForever);        
        //Amostragem
        acc_read(&sample_x, &sample_y, &sample_z);
        //Aplica offset
//        sample_x = sample_x+x_off;
//        sample_y = sample_y+y_off;
//        sample_z = sample_z+z_off;
        yield;
        //Fim de escrita na sessao critica 
        osSemaphoreRelease(id_semaphore_write_lock);
        
        //Notifica a proxima thread do fim de execucao desta thread
        f_set(flags_thread_filter, T_NOTIFY);
        osSignalSet(id_thread_filter, 0x1);

        //Grava intervalo de execucao
        write_gantt(" Thread Samples ", time, osKernelSysTick()/gantt_ticks_factor);
      }
    }
  }
}
osThreadDef(thread_samples, osPriorityNormal, 1, 0);

//Thread para filtragem e gravacao nos buffers de 64 elementos
void thread_filter(void const *args){
  uint32_t time;
  osEvent evt;

  int8_t queue_x[4] = {0};
  int8_t queue_y[4] = {0};
  int8_t queue_z[4] = {0};

  while(1){
    //Espera sinal
    evt = osSignalWait (S_EXEC_WAKE, osWaitForever);     
    if(evt.status == osEventSignal){
      //Sinal de protecao, limpe a flag e espere 
      if (f_get(flags_thread_filter, T_PROTECT)){
        f_clear(flags_thread_filter, T_PROTECT);
        evt = osSignalWait (S_PROT_WAKE, osWaitForever);     
        //Sinal para acordar da protecao
        if(evt.status != osEventSignal)
          continue;
      }
      //Sinal de notificacao, limpe a flag e realize filtragem
      if (f_get(flags_thread_filter, T_NOTIFY)){
        f_clear(flags_thread_filter, T_NOTIFY);
        time = osKernelSysTick()/gantt_ticks_factor; //inicio do intervalo de execucao da thread

        //Inicio de escrita na sessao critica 
        osSemaphoreWait(id_semaphore_write_lock, osWaitForever);        
        yield;
        //Atualize as quatro ultimas amostras
        enqueue_new_sample(queue_x, sample_x);
        enqueue_new_sample(queue_y, sample_y);
        enqueue_new_sample(queue_z, sample_z);

        //Aplique o filtro
        int x = filter(queue_x);        
        int y = filter(queue_y);
        int z = filter(queue_z);
        
        //Decremente indice do buffer (sobrescrita do valor mais antigo)
        index_x = circular_dec(index_x, 64);
        index_y = circular_dec(index_y, 64);
        index_z = circular_dec(index_z, 64);
        
        //Adicione valor filtrado ao buffer
        buffer_x[index_x] = x;
        buffer_y[index_y] = y;
        buffer_z[index_z] = z;
        //Fim de escrita na sessao critica 
        osSemaphoreRelease(id_semaphore_write_lock);
        
        //Atualiza flags das outras threads para inciarem execucao
        f_set(flags_thread_bar_red_leds,        T_NOTIFY);
        f_set(flags_thread_bar_green_leds,      T_NOTIFY);
        f_set(flags_thread_display_oled,        T_NOTIFY);
        f_set(flags_thread_export_file,         T_NOTIFY);

        //Notifica outras threads para iniciarem execucao
        osSignalSet(id_thread_bar_red_leds,     S_EXEC_WAKE);
        osSignalSet(id_thread_bar_green_leds,   S_EXEC_WAKE);
        osSignalSet(id_thread_display_oled,     S_EXEC_WAKE);
        osSignalSet(id_thread_export_file,      S_EXEC_WAKE);    

        //Grava intervalo de execucao
        write_gantt(" Thread Filter  ", time, osKernelSysTick()/gantt_ticks_factor);
      }
    }
  }
}
osThreadDef(thread_filter, osPriorityNormal, 1, 0);

//Thread para exibir primeiro eixo selecionado na barra de leds vermelhos
void thread_bar_red_led(void const *args){
  uint32_t time;
  osEvent evt;
  uint16_t last_mask = 0;
  while(1){
    //Espera sinal
    evt = osSignalWait (S_EXEC_WAKE, osWaitForever);     
    if(evt.status == osEventSignal){
      //Sinal de protecao, limpe a flag e espere 
      if (f_get(flags_thread_bar_red_leds, T_PROTECT)){
        f_clear(flags_thread_bar_red_leds, T_PROTECT);
        evt = osSignalWait (S_PROT_WAKE, osWaitForever);     
        //Sinal para acordar da protecao
        if(evt.status != osEventSignal)
          continue;
      }
      //Sinal de notificacao, limpe a flag e atualiza barra de leds
      if (f_get(flags_thread_bar_red_leds, T_NOTIFY)){
        f_clear(flags_thread_bar_red_leds, T_NOTIFY);
        time = osKernelSysTick()/gantt_ticks_factor; //inicio do intervalo de execucao da thread

        //Inicio de leitura na sessao critica 
        osMutexReaderWait(id_mutex_read_lock, id_semaphore_write_lock, &readers_count);
        yield;
        //Recupera buffer do primeiro eixo selecionado
        int8_t (*plot_samples)[64];
        get_selected_axis_samples(first_selected_axis(), &plot_samples);
        //Recupera indice do buffer do primeiro eixo selecionado
        int samples_index = get_selected_axis_index(first_selected_axis());
        
        //Recupera primeiro valor do buffer
        int8_t value = (*plot_samples)[samples_index];
        //Fim de leitura na sessao critica 
        osMutexReaderRelease(id_mutex_read_lock, id_semaphore_write_lock, &readers_count);
        
        //Transforma o valor de um intervalo de [-128, 128[ 
        //para outro valor pertencente à [0, 8]
        uint8_t norm =  scale_proportional(value, -128, 256-1, 0, 8); 
        //Calcula mascara de escrita 
        //Os indices dos 8 primeiros bits em 1 representam os leds vermelhos que serao acesos, respectivamente
        uint16_t mask = 0;
        for(int i = 0; i < norm; i++) mask |= 1 << i;
        //Apaga ultimos LEDs acesos e acende os atuais
        pca9532_setLeds(mask, last_mask);
        //LEDs que deverao ser apagados na proxima execucao foram acesos agora
        last_mask = mask;

        //Grava intervalo de execucao
        write_gantt(" Thread LEDR    ", time, osKernelSysTick()/gantt_ticks_factor);
      }
    }
  }
}
osThreadDef(thread_bar_red_led, osPriorityNormal, 1, 0);

//Thread para exibir segundo eixo selecionado na barra de leds verdes
void thread_bar_green_led(void const *args){
  uint32_t time;
  osEvent evt;
  uint16_t last_mask = 0;
  while(1){
    //Espera sinal
    evt = osSignalWait (S_EXEC_WAKE, osWaitForever);     
    if(evt.status == osEventSignal){
      //Sinal de protecao, limpe a flag e espere 
      if (f_get(flags_thread_bar_green_leds, T_PROTECT)){
        f_clear(flags_thread_bar_green_leds, T_PROTECT);
        evt = osSignalWait (S_PROT_WAKE, osWaitForever);     
        //Sinal para acordar da protecao
        if(evt.status != osEventSignal)
          continue;
      }
      //Sinal de notificacao, limpe a flag e atualiza barra de leds
      if (f_get(flags_thread_bar_green_leds, T_NOTIFY)){
        f_clear(flags_thread_bar_green_leds, T_NOTIFY);
        time = osKernelSysTick()/gantt_ticks_factor; //inicio do intervalo de execucao da thread

        //Inicio de leitura na sessao critica 
        osMutexReaderWait(id_mutex_read_lock, id_semaphore_write_lock, &readers_count);
        yield;

        //Recupera buffer do segundo eixo selecionado
        int8_t (*plot_samples)[64];
        get_selected_axis_samples(second_selected_axis(), &plot_samples);
        //Recupera indice do buffer do segundo eixo selecionado
        int samples_index = get_selected_axis_index(second_selected_axis());

        int8_t value = (*plot_samples)[samples_index];
        //Fim de leitura na sessao critica 
        osMutexReaderRelease(id_mutex_read_lock, id_semaphore_write_lock, &readers_count);
        
        //Transforma o valor de um intervalo de [-128, 128[ 
        //para outro valor pertencente à [0, 8]
        uint8_t norm =  scale_proportional(value, -128, 128-1, 0, 8); 
        //Calcula mascara de escrita 
        //Os indices dos 8 ultimos bits em 1 representam os leds verdes que serao acesos, em ordem inversa
        uint16_t mask = 0;
        for(int i = 0; i < norm; i++) mask |= 0x8000 >> i;
        //Apaga ultimos LEDs acesos e acende os atuais
        pca9532_setLeds(mask, last_mask);
        //LEDs que deverao ser apagados na proxima execucao foram acesos agora
        last_mask = mask;
        
        //Grava intervalo de execucao
        write_gantt(" Thread LEDG    ", time, osKernelSysTick()/gantt_ticks_factor);
      }
    }
  }  
}
osThreadDef(thread_bar_green_led, osPriorityNormal, 1, 0);

//Thread para exportar dados coletados como arquivo de texto
void thread_export_file(void const *args){
  uint32_t time;
  osEvent evt;
  //Abre arquivo e o sobreescreve
  FILE *log_file = fopen("acc_log.txt", "w");
  fprintf(log_file, "Acelerometro\n");
  fclose(log_file);
  while(1){
    //Espera sinal
    evt = osSignalWait (S_EXEC_WAKE, osWaitForever);
    if(evt.status == osEventSignal){
      //Sinal de protecao, limpe a flag e espere 
      if (f_get(flags_thread_export_file, T_PROTECT)){
        f_clear(flags_thread_export_file, T_PROTECT);
        evt = osSignalWait (S_PROT_WAKE, osWaitForever);     
        //Sinal para acordar da protecao
        if(evt.status != osEventSignal)
          continue;
      }
      //Sinal de notificacao, limpe a flag e exporte dados coletados
      if (f_get(flags_thread_export_file, T_NOTIFY)){ 
        f_clear(flags_thread_export_file, T_NOTIFY);
        time = osKernelSysTick()/gantt_ticks_factor; //inicio do intervalo de execucao da thread

        //Inicio de leitura na sessao critica 
        osMutexReaderWait(id_mutex_read_lock, id_semaphore_write_lock, &readers_count);
        yield;
        //Recupera ultimos valores adcionados ao buffer
        int8_t x = buffer_x[index_x];
        int8_t y = buffer_y[index_y];
        int8_t z = buffer_z[index_z];
        //Fim de leitura na sessao critica 
        osMutexReaderRelease(id_mutex_read_lock, id_semaphore_write_lock, &readers_count);
        
        //Adiciona ao final do arquivo dados das leituras no eixo x, y, z e do 
        //tempo em que a gravacao foi realizada
        FILE *log_file = fopen("acc_log.txt", "a");
        fseek(log_file, 0, SEEK_END);
        fprintf(log_file, "t: %05lu x: %04d y: %04d z: %04d\n",  
                osKernelSysTick()/milis_ticks_factor, x,  y, z);
        fclose(log_file);
        
        //Grava intervalo de execucao
        write_gantt(" Thread Export  ", time, osKernelSysTick()/gantt_ticks_factor);
      }
    }
  }
}
osThreadDef(thread_export_file, osPriorityNormal, 1, 0);

//Thread para plotar valores filtrados do terceiro eixo selecionado no OLED
void thread_display_oled(void const *args){
  uint32_t time;
  orientation_t facing = SOUTH;
  osEvent evt;

  //Linha para delimitar fim do gráfico
  oled_line(64, 0, 64, 63, OLED_COLOR_BLACK);
  
  while(1){
    //Espera sinal
    evt = osSignalWait (S_EXEC_WAKE, osWaitForever);
    if(evt.status == osEventSignal){
      //Sinal de protecao, limpe a flag e espere 
      if (f_get(flags_thread_display_oled, T_PROTECT)){
        f_clear(flags_thread_display_oled, T_PROTECT);
        evt = osSignalWait (S_PROT_WAKE, osWaitForever);     
        //Sinal para acordar da protecao
        if(evt.status != osEventSignal)
          continue;
      }
      //Sinal de notificacao, limpe a flag e plote valores no OLED
      if (f_get(flags_thread_display_oled, T_NOTIFY)){ 
        f_clear(flags_thread_display_oled, T_NOTIFY);
        time = osKernelSysTick()/gantt_ticks_factor; //inicio do intervalo de execucao da thread

        //Inicio de leitura na sessao critica 
        osMutexReaderWait(id_mutex_read_lock, id_semaphore_write_lock, &readers_count);
        //Recupera buffer do terceiro eixo selecionado
        int8_t (*plot_samples)[64];
        get_selected_axis_samples(third_selected_axis(), &plot_samples);
        //Recupera indice do buffer do terceiro eixo selecionado
        int samples_index = get_selected_axis_index(third_selected_axis());
 
        //Primeiro valor de cada buffer é o ultimo valor amostrado
        int vX = buffer_x[index_x];
        int vY = buffer_y[index_y];
        int vZ = buffer_z[index_z];
        //Calcula nova orientacao com base nas ultimas medicoes
        orientation_t new_orient = get_orientation(vX, vY, vZ);
        //Caso nao seja necessario atualizar, permanece com ultimo valor calculado
        facing = new_orient != NONE ? new_orient : facing;

        //Variaveis do ultimo ponto plotado, inicializadas com o valor do primeiro ponto
        int last_i = 0;
        //Transforma o valor de um intervalo de [-128, 128[ 
        //para outro valor pertencente à [0, 64[
        int last_j = scale_proportional((*plot_samples)[samples_index], -128, 256-1, 0, 64-1);
        //Iterador de pontos do grafico:
        //i = valor no eixo das ordenadas do grafico
        //j = valor no eixo das abcissas do grafico
        //x = coordenada x do ponto no display OLED do grafico rotacionado
        //y = coordenada y do ponto no display OLED do grafico rotacionado
        //last_[...] = valor da ultima coordenada/ ponto plotado
        for (int i=0 ; i< 64;i++){
          yield;          
          //Iterador do valor equivalente sobre eixo de plotagem, ... 
          int j = (*plot_samples)[circular_add(i, samples_index, 64)];
          //... alterando intervalo do eixo das ordenadas de [-128,128[ para [0,64[
          j = scale_proportional(j, -128, 256-1, 0, 64-1);

          //Rotaciona coodenadas do grafico
          int x = i;
          int y = j;
          int last_x = last_i;
          int last_y = last_j;
          rotate_coordinates(&x, &y, facing);
          rotate_coordinates(&last_x, &last_y, facing);

          //Move para primeiro quadrante
          x = inv_horizontal(facing) ? x + 63 : x;
          y = inv_vertical(facing) ? y + 63 : y;
          last_x = inv_horizontal(facing) ? last_x + 63 : last_x;
          last_y = inv_vertical(facing) ? last_y + 63 : last_y;
          
          //Limpa coluna ou linha para plotar novo valor
          if(is_vertical(facing))
            oled_line(x, 0, x, 63, OLED_COLOR_WHITE);
          else if(is_horizontal(facing))
            oled_line(0, y, 63, y, OLED_COLOR_WHITE);
          
          //Desenha linha entre ultimo valor e o atual da iteração 
          //sobre vetor de amostras
          oled_line(last_x, last_y, x, y, OLED_COLOR_BLACK);
          //Na próxima iteração, as coordenadas atuais serão as anteriores
          last_i = i;
          last_j = j;
        }
        //Fim de leitura na sessao critica 
        osMutexReaderRelease(id_mutex_read_lock, id_semaphore_write_lock, &readers_count);
        
        //Grava intervalo de execucao
        write_gantt(" Thread OLED    ", time, osKernelSysTick()/gantt_ticks_factor);
      }
    }
  }
}
osThreadDef(thread_display_oled, osPriorityNormal, 1, 0);

//************************
//Thread de tratamento de interrupcao (Main)
void thread_isr(){    
  uint32_t time;
  osEvent evt;
  while(1){
    //Espera sinal e realiza debounce ao acordar
    evt = osSignalWait (S_EXEC_WAKE, osWaitForever);    
    if(evt.status == osEventSignal){
      //Tempo de debounce 
      osDelay(DEBOUNCE_T);
      //Sinal de botao para selecionar eixo anterior apertado, limpe a e flag altere eixo selecionado
      if (f_get(flags_thread_isr, ISR_PREV_AXIS)){
        f_clear(flags_thread_isr, ISR_PREV_AXIS);
        //Caso o switch esteja pressionado, seleciona eixo anterior
        if(is_pressed(PI_PREV_AXIS)){
          selected_axis = prev_selected_axis();
        }
      }
      //Sinal de botao para selecionar proximo eixo apertado, limpe a flag e altere eixo selecionado
      if (f_get(flags_thread_isr, ISR_NEXT_AXIS)){
        f_clear(flags_thread_isr, ISR_NEXT_AXIS);
        //Caso o switch esteja pressionado, seleciona proximo eixo 
        if(is_pressed(PI_NEXT_AXIS)){
          selected_axis = next_selected_axis();
        }
      }
      //Sinal de protecao do sistema ativo, limpe a flag e envie sinais de protecao para as outras threads
      if (f_get(flags_thread_isr, ISR_PROTECT)){
        f_clear(flags_thread_isr, ISR_PROTECT);
        time = osKernelSysTick()/gantt_ticks_factor;

        //Caso o switch esteja pressionado, envia sinais de espera de protecao 
        if(is_pressed(PI_PROTECT)){
          //Alterando flag de protecao
          f_set(flags_thread_samples,           T_PROTECT);
          f_set(flags_thread_filter,            T_PROTECT);
          f_set(flags_thread_display_oled,      T_PROTECT);
          f_set(flags_thread_export_file,       T_PROTECT);

          //Envio de sinais para acordar threads
          osSignalSet(id_thread_samples,        S_EXEC_WAKE);
          osSignalSet(id_thread_filter,         S_EXEC_WAKE);
          osSignalSet(id_thread_display_oled,   S_EXEC_WAKE);
          osSignalSet(id_thread_export_file,    S_EXEC_WAKE);

          //Acende LED de protecao
          led_set();
          
          //Inicia temporizador de protecao e entra em espera
          osTimerStart(id_timer_protection, 1000);
          osSignalWait (S_PROT_WAKE, osWaitForever);    
          
          //Após ser acordado pelo temporizador, acorda as outras threads
          osSignalSet(id_thread_samples,        S_PROT_WAKE);
          osSignalSet(id_thread_filter,         S_PROT_WAKE);
          osSignalSet(id_thread_display_oled,   S_PROT_WAKE);
          osSignalSet(id_thread_export_file,    S_PROT_WAKE);
          
          //Apaga LED de protecao
          led_clear();
          
          //Grava intervalo de execucao
          write_gantt(" Thread ISR     ", time, osKernelSysTick()/gantt_ticks_factor);
        }
      }
    }
  }
}

//************************
//Codigo Main
//************************
int main(int n_args, char** args){
  //Inicialização de Kernel vai aqui
  osKernelInitialize();  

  //************************
  //Init Diagram of Gantt
  //*********************** 
  FILE* file_gantt = fopen("gantt.txt","w");
  fprintf(file_gantt,"gantt\n");
  fprintf(file_gantt,"    title A Gantt Diagram\n");
  fprintf(file_gantt,"    dateFormat x\n");
  fclose(file_gantt);  
  
  SSPInit();
  I2CInit( (uint32_t)I2CMASTER, 0 );
  GPIOInit();
  
  pca9532_init();
  acc_init();
  joystick_init();
  setup_isr();
  led_init();
  oled_init();
  oled_clearScreen(OLED_COLOR_WHITE);  
  
  //************************
  //Inicialização de Timers
  //************************
  id_timer_sampling =   osTimerCreate(osTimer(timer_sampling),   osTimerPeriodic, NULL);
  id_timer_protection = osTimerCreate(osTimer(timer_protection), osTimerOnce,     NULL);
  
  osTimerStart(id_timer_sampling, 250);
    
  //************************
  //Inicialização de Mutex
  //************************
  id_mutex_read_lock = osMutexCreate(osMutex(mutex_read_lock));
  
  id_semaphore_write_lock = osSemaphoreCreate(osSemaphore(semaphore_write_lock), 1);

  //************************
  //Inicialização de Threads
  //************************
  id_thread_samples =                   osThreadCreate(osThread(thread_samples),        NULL);
  id_thread_filter =                    osThreadCreate(osThread(thread_filter),         NULL);
  id_thread_bar_red_leds =              osThreadCreate(osThread(thread_bar_red_led),    NULL);
  id_thread_bar_green_leds =            osThreadCreate(osThread(thread_bar_green_led),  NULL);
  id_thread_export_file =               osThreadCreate(osThread(thread_export_file),    NULL);
  id_thread_display_oled =              osThreadCreate(osThread(thread_display_oled),   NULL);
  
  //************************
  //Início do SO
  //************************
  osKernelStart();
  
  //************************
  //Thread Main
  //************************
  id_thread_isr = osThreadGetId();
  osThreadSetPriority(id_thread_isr, osPriorityNormal);
  thread_isr();
  
  //************************
  //Finalização de Threads
  //************************
  osThreadTerminate(id_thread_isr);
  osThreadTerminate(id_thread_export_file);
  osThreadTerminate(id_thread_samples);
  osThreadTerminate(id_thread_filter);
  osThreadTerminate(id_thread_bar_red_leds);
  osThreadTerminate(id_thread_bar_green_leds);  
  osThreadTerminate(id_thread_display_oled);
  
  osDelay(osWaitForever); 
  return 0;
} 