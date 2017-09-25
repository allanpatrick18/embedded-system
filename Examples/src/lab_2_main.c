#include "mcu_regs.h"
#include "type.h"
#include "stdio.h"
#include "timer32.h"
#include "pca9532.h"
#include "cmsis_os.h"
#include "string.h"
#include "ctype.h"
#include "libdemo.h"

FILE *file_gantt;

//************************
//IDs de Timers
//************************
osTimerId id_timer_sampling;

//************************
//IDs de Threads
//************************
osThreadId id_thread_samples;
osThreadId id_thread_write_ram;
osThreadId id_thread_bar_red_leds;
osThreadId id_thread_bar_green_leds;
osThreadId id_thread_display_oled;
osThreadId id_thread_export_file;

//************************
// flags
//************************
// 0x04 memória ram liberada
// 0x08 memória ram bloqueada
// 0x01 acorda para execução
// 0x02 proteçaõ contra queda


//************************
// vetores de
//************************
int8_t axis_x[64]={0};
int8_t axis_y[64]={0};
int8_t axis_z[64]={0};

int index_x =0;
int index_y =0;
int index_z =0;

int8_t filtered_x;
int8_t filtered_y;
int8_t filtered_z;


//************************
//ISR
//************************

void PIOINT2_IRQHandler(void)
{
  if (!GPIOGetValue(PORT2, 0)){
//    printf("Joystick center\n");
//    osSignalSet(isr_id, 0x1);
  }
  GPIOIntClear(2, 0);
}

void setup_isr()
{   
  setup_port(PORT2, 0, 0, 1, 1);
  GPIOIntEnable(PORT2, 0); // JOYSTICK_CENTER
}

int8_t formula(int8_t eixo[4]){
  return(int8_t)(eixo[0]+ 0.6*eixo[1] +0.3*eixo[2]+ 0.1*eixo[3])/2;
}

//************************
//Callback de Timers
void timer_sampling_cb(void const *arg) {
  osSignalSet(id_thread_samples, 0x1);
}
osTimerDef(timer_sampling, timer_sampling_cb);

//************************
//Threads

//Thread para geração de chaves
void thread_samples(void const *args){
  osEvent evt;
  int8_t queue_x[4]={0};
  int8_t queue_y[4]={0};
  int8_t queue_z[4]={0};

  int8_t x_off = 0;
  int8_t y_off = 0;
  int8_t z_off = 0;

  acc_read(&x_off, &y_off, &z_off);

  x_off = -x_off;
  y_off = -y_off;
  z_off = -z_off;
  
  while(1){
    evt = osSignalWait (0x01, osWaitForever);     
    if(evt.status == osEventSignal){
      if (evt.value.signals & 0x02){
        osSignalWait (0x00,1000);
      }
      else if (evt.value.signals & 0x01){
      int8_t x = 0;
      int8_t y = 0;
      int8_t z = 0;
      
      acc_read(&x, &y, &z);
      
      x = x+x_off;
      y = y+y_off;
      z = z+z_off;
      
      queue_x[3] = queue_x[2];
      queue_x[2] = queue_x[1];
      queue_x[1] = queue_x[0];
      queue_x[0] = x;
      filtered_x =formula(queue_x);
      
      queue_y[3] = queue_y[2];
      queue_y[2] = queue_y[1];
      queue_y[1] = queue_y[0];
      queue_y[0] = y;
      filtered_y =formula(queue_y);
      
      queue_z[3] = queue_z[2];
      queue_z[2] = queue_z[1];
      queue_z[1] = queue_z[0];
      queue_z[0] = z;
      filtered_z =formula(queue_z);
      
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
      if (evt.value.signals & 0x02){
        osSignalWait (0x00,1000);
      }else if (evt.value.signals & 0x01){
        index_x =(index_x+63)%64;
        index_y =(index_y+63)%64;
        index_z =(index_z+63)%64;
        
        axis_x[index_x] = filtered_x;
        axis_y[index_y] = filtered_y;
        axis_z[index_z] = filtered_z;
        
        osSignalSet(id_thread_bar_red_leds, 0x1);
        osSignalSet(id_thread_bar_green_leds, 0x1);
        osSignalSet(id_thread_display_oled, 0x1);
        osSignalSet(id_thread_export_file, 0x1);    
      }
    }
  }
}
osThreadDef(thread_write_ram, osPriorityNormal, 1, 0);

//Thread para 
void thread_red_led(void const *args){
  osDelay(osWaitForever);
}
osThreadDef(thread_red_led, osPriorityNormal, 1, 0);

//Thread para 
void thread_green_led(void const *args){
  osDelay(osWaitForever);
}
osThreadDef(thread_green_led, osPriorityNormal, 1, 0);

//Thread para 
void thread_display_oled(void const *args){
  osEvent evt;
  while(1){
    evt = osSignalWait (0x01, osWaitForever);
    if(evt.status == osEventSignal){
      if (evt.value.signals & 0x02){
        osSignalWait (0x00,1000);
      }else if (evt.value.signals & 0x01){ 
        oled_clearScreen(OLED_COLOR_WHITE); 
        int last_j = ((axis_z[index_z]+128)*64)/256;
        for (int i=0 ; i< 64;i++){
          int j = axis_z[(i+index_z)%64]+128;
          j = (j*64)/256;
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
      }
    }
  }
}
osThreadDef(thread_display_oled, osPriorityNormal, 1, 0);

//Thread para 
void thread_export_file(void const *args){
  osDelay(osWaitForever);
}
osThreadDef(thread_export_file, osPriorityNormal, 1, 0);

//************************
//Código da thread Main
//************************

void thread_main(){    
  while(1) osThreadYield();
  printf("finished");
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
//  fclose(file_gantt);  
  
  // Setup IRS
  GPIOInit();
  SSPInit();
  I2CInit( (uint32_t)I2CMASTER, 0 );
  
  oled_init();
  oled_clearScreen(OLED_COLOR_WHITE);
  
  acc_init();
  joystick_init();
  setup_isr();
  
  //************************
  //Inicialização de Timers aqui
  //************************
  id_timer_sampling = osTimerCreate(osTimer(timer_sampling), osTimerPeriodic, NULL);
  osTimerStart(id_timer_sampling, 50);
  
  //************************
  //Inicialização de Threads aqui
  //************************
  id_thread_samples =                   osThreadCreate(osThread(thread_samples),        NULL);
  id_thread_write_ram =                 osThreadCreate(osThread(thread_write_ram),      NULL);
  id_thread_bar_red_leds =              osThreadCreate(osThread(thread_red_led),        NULL);
  id_thread_bar_green_leds =            osThreadCreate(osThread(thread_green_led),      NULL);
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
  osThreadTerminate(id_thread_export_file);
  osThreadTerminate(id_thread_samples);
  osThreadTerminate(id_thread_write_ram);
  osThreadTerminate(id_thread_bar_red_leds);
  osThreadTerminate(id_thread_bar_green_leds);  
  osThreadTerminate(id_thread_display_oled);
  
  osDelay(osWaitForever); 
  return 0;
}