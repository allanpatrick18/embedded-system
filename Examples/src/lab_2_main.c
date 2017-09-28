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
//IDs de Mutex
//************************
osMutexId id_mutex_ram_lock;
osMutexDef(mutex_ram_lock);

//************************
//IDs de Timers
//************************
osTimerId id_timer_sampling;
osTimerId id_timer_protection;

//************************
//IDs de Threads
//************************
osThreadId id_thread_protection;
osThreadId id_thread_samples;
osThreadId id_thread_write_ram;
osThreadId id_thread_bar_red_leds;
osThreadId id_thread_bar_green_leds;
osThreadId id_thread_display_oled;
osThreadId id_thread_export_file;

//************************
// flags
//************************
uint8_t flags_thread_samples = 0;
uint8_t flags_thread_write_ram = 0;
uint8_t flags_thread_bar_red_leds = 0;
uint8_t flags_thread_bar_green_leds = 0;
uint8_t flags_thread_display_oled = 0;
uint8_t flags_thread_export_file = 0;

#define f_set(v, f)     (v = v |  f)
#define f_clear(v, f)   (v = v & ~f)
#define f_get(v, f)     (v & f)

#define T_NOTIFY        0x01
#define T_PROTECT       0x02

//************************
// vetores de
//************************
int8_t axis_x[64] = {0};
int8_t axis_y[64] = {0};
int8_t axis_z[64] = {0};

int index_x = 0;
int index_y = 0;
int index_z = 0;

int8_t filtered_x;
int8_t filtered_y;
int8_t filtered_z;

#define true 1
#define false 0
typedef uint8_t bool;

//************************
//Gantt
//************************
FILE *file_gantt;
//Fator para exibir tempo em milisegundos
int ticks_factor = 72000;

//************************
//ISR
//************************
void PIOINT2_IRQHandler(void) {
  if (!GPIOGetValue(PORT2, 9))
    osSignalSet(id_thread_protection, 0x01);
  GPIOIntClear(2, 9);
}

volatile unsigned long *porta1_IS  = (volatile unsigned long *)0x50018004;
volatile unsigned long *porta1_IBE = (volatile unsigned long *)0x50018008;
volatile unsigned long *porta1_IEV = (volatile unsigned long *)0x5001800C;
volatile unsigned long *porta2_IS  = (volatile unsigned long *)0x50028004;
volatile unsigned long *porta2_IBE = (volatile unsigned long *)0x50028008;
volatile unsigned long *porta2_IEV = (volatile unsigned long *)0x5002800C;

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

void setup_isr() {   
  setup_port(PORT2, 9, 0, 1, 1);
  GPIOIntEnable(PORT2, 9); // SW3
}

int8_t filter(int8_t eixo[4]){
  int16_t t0 = eixo[0];
  int16_t t1 = eixo[1];
  int16_t t2 = eixo[2];
  int16_t t3 = eixo[3];
  int16_t r = (int16_t) (t0 + 0.6*t1 + 0.3*t2 + 0.1*t3)/2;
  return (int8_t) r;
}

void led_set(){
  GPIOSetValue(PORT0, 7, 1);
}

void led_clear(){
  GPIOSetValue(PORT0, 7, 0);
}

//led2
void led_init(){
  GPIOSetDir(PORT0, 7, 1);
  led_clear();
}

//************************
//Callback de Timers
void timer_sampling_cb(void const *arg) {
  f_set(flags_thread_samples, T_NOTIFY);
  osSignalSet(id_thread_samples, 0x01);
}
osTimerDef(timer_sampling, timer_sampling_cb);

void timer_protection_cb(void const *arg) {
  osSignalSet(id_thread_protection, 0x02);
}
osTimerDef(timer_protection, timer_protection_cb);

//************************
//Threads

//Thread para geração de chaves
void thread_samples(void const *args){
  osEvent evt;
  int8_t queue_x[4] = {0};
  int8_t queue_y[4] = {0};
  int8_t queue_z[4] = {0};

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
    evt = osSignalWait (0x01, osWaitForever);     
    if(evt.status == osEventSignal){
      if (f_get(flags_thread_samples,T_PROTECT)){
        f_clear(flags_thread_samples, T_PROTECT);
        evt = osSignalWait (0x02, osWaitForever);     
        if(evt.status != osEventSignal)
          continue;
      }
      if (f_get(flags_thread_samples,T_NOTIFY)){
        f_clear(flags_thread_samples, T_NOTIFY);
        int8_t x = 0;
        int8_t y = 0;
        int8_t z = 0;
        
        acc_read(&x, &y, &z);
        
//        x = x+x_off;
//        y = y+y_off;
//        z = z+z_off;
        
        queue_x[3] = queue_x[2];
        queue_x[2] = queue_x[1];
        queue_x[1] = queue_x[0];
        queue_x[0] = x;
        filtered_x = filter(queue_x);
        
        queue_y[3] = queue_y[2];
        queue_y[2] = queue_y[1];
        queue_y[1] = queue_y[0];
        queue_y[0] = y;
        filtered_y = filter(queue_y);
        
        queue_z[3] = queue_z[2];
        queue_z[2] = queue_z[1];
        queue_z[1] = queue_z[0];
        queue_z[0] = z;
        filtered_z = filter(queue_z);
        
        f_set(flags_thread_write_ram, T_NOTIFY);
        osSignalSet(id_thread_write_ram, 0x1);
      }
    }
  }
}
osThreadDef(thread_samples, osPriorityNormal, 1, 0);

//Thread 
void thread_write_ram(void const *args){
  osEvent evt;
  while(1){
    evt = osSignalWait (0x01, osWaitForever);     
    if(evt.status == osEventSignal){
      if (f_get(flags_thread_write_ram, T_PROTECT)){
        f_clear(flags_thread_write_ram, T_PROTECT);
        evt = osSignalWait (0x02, osWaitForever);     
        if(evt.status != osEventSignal)
          continue;
      }
      if (f_get(flags_thread_write_ram, T_NOTIFY)){
        f_clear(flags_thread_write_ram, T_NOTIFY);
        index_x = (index_x+63)%64;
        index_y = (index_y+63)%64;
        index_z = (index_z+63)%64;
        
        osMutexWait(id_mutex_ram_lock, osWaitForever);
        axis_x[index_x] = filtered_x;
        axis_y[index_y] = filtered_y;
        axis_z[index_z] = filtered_z;
        osMutexRelease(id_mutex_ram_lock);
        
        f_set(flags_thread_bar_red_leds, T_NOTIFY);
        osSignalSet(id_thread_bar_red_leds, 0x1);
        f_set(flags_thread_bar_green_leds, T_NOTIFY);
        osSignalSet(id_thread_bar_green_leds, 0x1);
        f_set(flags_thread_display_oled, T_NOTIFY);
        osSignalSet(id_thread_display_oled, 0x1);
        f_set(flags_thread_export_file, T_NOTIFY);
        osSignalSet(id_thread_export_file, 0x1);    
      }
    }
  }
}
osThreadDef(thread_write_ram, osPriorityNormal, 1, 0);

//Thread para 
void thread_bar_red_led(void const *args){
  osEvent evt;
  uint16_t last_mask = 0;
  while(1){
    evt = osSignalWait (0x01, osWaitForever);     
    if(evt.status == osEventSignal){
      if (f_get(flags_thread_bar_red_leds, T_PROTECT)){
        f_clear(flags_thread_bar_red_leds, T_PROTECT);
        evt = osSignalWait (0x02, osWaitForever);     
        if(evt.status != osEventSignal)
          continue;
      }
      if (f_get(flags_thread_bar_red_leds, T_NOTIFY)){
        f_clear(flags_thread_bar_red_leds, T_NOTIFY);
        
        osMutexWait(id_mutex_ram_lock, osWaitForever);
        int8_t value_x = axis_x[index_x];
        osMutexRelease(id_mutex_ram_lock);
        
        uint8_t norm_x = (value_x+128)*8/255;
        uint16_t mask = 0;
        for(int i = 0; i < norm_x; i++)
          mask += 1 << i;
        pca9532_setLeds(mask, last_mask);
        last_mask = mask;
      }
    }
  }
}
osThreadDef(thread_bar_red_led, osPriorityNormal, 1, 0);

//Thread para 
void thread_bar_green_led(void const *args){
  osEvent evt;
  uint16_t last_mask = 0;
  while(1){
    evt = osSignalWait (0x01, osWaitForever);     
    if(evt.status == osEventSignal){
      if (f_get(flags_thread_bar_green_leds, T_PROTECT)){
        f_clear(flags_thread_bar_green_leds, T_PROTECT);
        evt = osSignalWait (0x02, osWaitForever);     
        if(evt.status != osEventSignal)
          continue;
      }
      if (f_get(flags_thread_bar_green_leds, T_NOTIFY)){
        f_clear(flags_thread_bar_green_leds, T_NOTIFY);
        
        osMutexWait(id_mutex_ram_lock, osWaitForever);
        int8_t value_y = axis_y[index_y];
        osMutexRelease(id_mutex_ram_lock);
        
        uint8_t norm_y = (value_y+128)*8/255;
        uint16_t mask = 0;
        for(int i = 0; i < norm_y; i++)
          mask += 0x8000 >> i;
        pca9532_setLeds(mask, last_mask);
        last_mask = mask;
      }
    }
  }  
}
osThreadDef(thread_bar_green_led, osPriorityNormal, 1, 0);

//Thread para 
void thread_display_oled(void const *args){
  osEvent evt;
  while(1){
    evt = osSignalWait (0x01, osWaitForever);
    if(evt.status == osEventSignal){
      if (f_get(flags_thread_display_oled, T_PROTECT)){
        f_clear(flags_thread_display_oled, T_PROTECT);
        evt = osSignalWait (0x02, osWaitForever);     
        if(evt.status != osEventSignal)
          continue;
      }
      if (f_get(flags_thread_display_oled, T_NOTIFY)){ 
        f_clear(flags_thread_display_oled, T_NOTIFY);
        oled_clearScreen(OLED_COLOR_WHITE); 
        int last_j = ((axis_z[index_z]+128)*63)/255;
        osMutexWait(id_mutex_ram_lock, osWaitForever);
        for (int i=0 ; i< 64;i++){
          int j = axis_z[(i+index_z)%64]+128;
          j = (j*63)/255;
          oled_putPixel(i, j, OLED_COLOR_BLACK);
          if(abs(last_j - j) > 1){
            int min_j, max_j;
            if(j < last_j){
              min_j = j;
              max_j = last_j;
            }
            else{
              min_j = last_j;
              max_j = j;
            }
            for(int delta_j = 1; delta_j < max_j - min_j; delta_j++)
              oled_putPixel(i, min_j + delta_j, OLED_COLOR_BLACK);
            }
          oled_putPixel(64, i, OLED_COLOR_BLACK);
          last_j = j;
        }
        osMutexRelease(id_mutex_ram_lock);
      }
    }
  }
}
osThreadDef(thread_display_oled, osPriorityNormal, 1, 0);

//Thread para 
void thread_export_file(void const *args){
  osEvent evt;
  FILE *log_file = fopen("acc_log.txt", "w");
  fprintf(log_file, "Acelerometro\n");
  fclose(log_file);
  while(1){
    evt = osSignalWait (0x01, osWaitForever);
    if(evt.status == osEventSignal){
      if (f_get(flags_thread_export_file, T_PROTECT)){
        f_clear(flags_thread_export_file, T_PROTECT);
        evt = osSignalWait (0x02, osWaitForever);     
        if(evt.status != osEventSignal)
          continue;
      }
      if (f_get(flags_thread_export_file, T_NOTIFY)){ 
        f_clear(flags_thread_export_file, T_NOTIFY);

        osMutexWait(id_mutex_ram_lock, osWaitForever);
        int8_t x = axis_x[index_x];
        int8_t y = axis_y[index_y];
        int8_t z = axis_z[index_z];
        osMutexRelease(id_mutex_ram_lock);
        
        FILE *log_file = fopen("acc_log.txt", "a");
        fseek(log_file, 0, SEEK_END);
        fprintf(log_file, "t: %05lu x: %04d y: %04d z: %04d\n",  
                osKernelSysTick()/ticks_factor, x,  y, z);
        fclose(log_file);
      }
    }
  }
}
osThreadDef(thread_export_file, osPriorityNormal, 1, 0);

//************************
//Código da thread Main
//************************

void thread_main(){    
  osEvent evt;
  id_thread_protection = osThreadGetId();
  osThreadSetPriority(id_thread_protection, osPriorityHigh);
  while(1){
    evt = osSignalWait (0x01, osWaitForever);    
    if(evt.status == osEventSignal){
      osDelay(20);
      if(GPIOGetValue(PORT2, 9))
        continue;
      f_set(flags_thread_samples, T_PROTECT);
      osSignalSet(id_thread_samples, 0x01);
      f_set(flags_thread_write_ram, T_PROTECT);
      osSignalSet(id_thread_write_ram, 0x01);
      f_set(flags_thread_display_oled, T_PROTECT);
      osSignalSet(id_thread_display_oled, 0x01);
      f_set(flags_thread_export_file, T_PROTECT);
      osSignalSet(id_thread_export_file, 0x01);

      led_set();
      
      osTimerStart(id_timer_protection, 1000);
      osSignalWait (0x02, osWaitForever);    
      osSignalSet(id_thread_samples, 0x02);
      osSignalSet(id_thread_write_ram, 0x02);
      osSignalSet(id_thread_display_oled, 0x02);
      osSignalSet(id_thread_export_file, 0x02);
      
      led_clear();
    }
  }
}

int main(int n_args, int8_t** args){
  //Inicialização de Kernel vai aqui
  osKernelInitialize();  

  //************************
  //Init Diagram of Gantt
  //*********************** 
  file_gantt = fopen("gantt.txt","w");
  fprintf(file_gantt,"gantt\n");
  fprintf(file_gantt,"    title A Gantt Diagram\n");
  fprintf(file_gantt,"    dateFormat x\n");
  fclose(file_gantt);  
  
  // Setup IRS
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
  //Inicialização de Timers aqui
  //************************
  id_timer_sampling =   osTimerCreate(osTimer(timer_sampling),   osTimerPeriodic, NULL);
  id_timer_protection = osTimerCreate(osTimer(timer_protection), osTimerOnce,     NULL);
  
  osTimerStart(id_timer_sampling, 250);
    
  //************************
  //Inicialização de Mutex
  //************************
  id_mutex_ram_lock = osMutexCreate(osMutex(mutex_ram_lock));
  
  //************************
  //Inicialização de Threads aqui
  //************************
  id_thread_samples =                   osThreadCreate(osThread(thread_samples),        NULL);
  id_thread_write_ram =                 osThreadCreate(osThread(thread_write_ram),      NULL);
  id_thread_bar_red_leds =              osThreadCreate(osThread(thread_bar_red_led),    NULL);
  id_thread_bar_green_leds =            osThreadCreate(osThread(thread_bar_green_led),  NULL);
  id_thread_display_oled =              osThreadCreate(osThread(thread_display_oled),   NULL);
  id_thread_export_file =               osThreadCreate(osThread(thread_export_file),    NULL);
  
  //************************
  //Fim de inicializações de Threads
  //***********************  

  //Início do SO
  osKernelStart();
  
  thread_main();
  
  //************************
  //Finalização de Threads aqui
  //************************
  osThreadTerminate(id_thread_protection);
  osThreadTerminate(id_thread_export_file);
  osThreadTerminate(id_thread_samples);
  osThreadTerminate(id_thread_write_ram);
  osThreadTerminate(id_thread_bar_red_leds);
  osThreadTerminate(id_thread_bar_green_leds);  
  osThreadTerminate(id_thread_display_oled);
  
  osDelay(osWaitForever); 
  return 0;
} 