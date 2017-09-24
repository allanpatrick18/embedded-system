#include "mcu_regs.h"
#include "type.h"
#include "stdio.h"
#include "timer32.h"
#include "pca9532.h"
#include "cmsis_os.h"
#include "string.h"
#include "ctype.h"
#include "libdemo.h"

FILE *file;

//************************
//IDs de Threads
//************************
osThreadId id_thread_sample_ace;
osThreadId id_thread_write_log;
osThreadId id_thread_test_bar_red_leds;
osThreadId id_thread_test_bar_green_leds;
osThreadId id_thread_display_oled;
osThreadId id_thread_export_file;

osThreadId isr_id;

//************************
// vetores de
//************************
char axis_x[96];
char axis_y[96];
char axis_z[96];

//************************
//ISR
//************************

void PIOINT2_IRQHandler(void)
{
  
    if (!GPIOGetValue(PORT2, 0))
      printf("Joystick center\n");

   osSignalSet(isr_id, 0x1);
   GPIOIntClear(2, 0);
}

void setup_isr()
{   setup_port(PORT2, 0, 0, 1, 1);
 
    GPIOIntEnable(PORT2, 0); // JOYSTICK_CENTER
}
 


//************************
//Threads

//Thread para geração de chaves
void thread_samples(void const *args){
    osSignalWait(0x1, osWaitForever);
    osSignalSet(id_thread_write_log, 0x1);
    osSignalWait(0x1, osWaitForever);
}
osThreadDef(thread_samples, osPriorityNormal, 1, 0);

//Thread para decifrar chaves
void thread_logger(void const *args){
    osSignalWait(0x1, 250);
    
    int32_t xoff = 0;
    int32_t yoff = 0;
    int32_t zoff = 0;

    int8_t x = 0;
    int8_t y = 0;
    int8_t z = 0;
	
    acc_read(&x, &y, &z);
    x = x+xoff;
    y = y+yoff;
    z = z+zoff;
	
    osSignalSet(id_thread_test_bar_red_leds, 0x1);
    osSignalSet(id_thread_test_bar_green_leds, 0x1);
    osSignalSet(id_thread_display_oled, 0x1);
    osSignalSet(id_thread_export_file, 0x1);

}
osThreadDef(thread_logger, osPriorityNormal, 1, 0);

//Thread para testar penultimo digito verificador
void thread_red_led(void const *args){
   osSignalWait(0x1, osWaitForever);
    pca_toggle(2);
}
osThreadDef(thread_red_led, osPriorityNormal, 1, 0);

//Thread para testar ultimo digito verificador
void thread_green_led(void const *args){
    osSignalWait(0x1, osWaitForever);
     pca_toggle(2);
}
osThreadDef(thread_green_led, osPriorityNormal, 1, 0);

//Thread para escrever na saida chave gerada
void thread_display_oled(void const *args){
    osSignalWait(0x1, osWaitForever);
}
osThreadDef(thread_display_oled, osPriorityNormal, 1, 0);

//Thread para validar a chave gerada, utilizando os testes realizados
void thread_export_file(void const *args){
     osSignalWait(0x1, osWaitForever);

}
osThreadDef(thread_export_file, osPriorityNormal, 1, 0);

//************************
//Código da thread Main
//************************



int main(int n_args, char** args){
  //Inicialização de Kernel vai aqui
  osKernelInitialize();
  
  // Setup IRS
  GPIOInit();
  joystick_init();
  I2CInit( (uint32_t)I2CMASTER, 0 );
  setup_isr();
  
  //************************
  //Inicialização de Threads aqui
  //************************
  id_thread_sample_ace =  osThreadCreate(osThread(thread_samples),       NULL);
  id_thread_write_log =  osThreadCreate(osThread(thread_logger),       NULL);
  id_thread_test_bar_red_leds =    osThreadCreate(osThread(thread_red_led),         NULL);
  id_thread_test_bar_green_leds =    osThreadCreate(osThread(thread_green_led),         NULL);
  id_thread_display_oled =     osThreadCreate(osThread(thread_display_oled),          NULL);
  id_thread_export_file =  osThreadCreate(osThread(thread_export_file),       NULL);

  
  //************************
  //Fim de inicializações de Threads
  //***********************  
  
  //Início do SO
  osKernelStart();
  
  //************************
  //Init Diagram of Gantt
  //*********************** 
  
    file = fopen("gantt.txt","w");
    fprintf(file,"gantt\n");
    fprintf(file,"    title A Gantt Diagram\n");
    fprintf(file,"    dateFormat x\n");
  
  
  
  

  //************************
  //Finalização de Threads aqui
  //************************
  osThreadTerminate(id_thread_export_file);
  osThreadTerminate(id_thread_sample_ace);
  osThreadTerminate(id_thread_write_log);
  osThreadTerminate(id_thread_test_bar_red_leds);
  osThreadTerminate(id_thread_test_bar_green_leds);  
  osThreadTerminate(id_thread_display_oled);
  
  osDelay(osWaitForever); 
  return 0;
}