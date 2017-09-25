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
osThreadId id_thread_write_ram;
osThreadId id_thread_test_bar_red_leds;
osThreadId id_thread_test_bar_green_leds;
osThreadId id_thread_display_oled;
osThreadId id_thread_export_file;

osThreadId isr_id;

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
char axis_x[64]={0};
char axis_y[64]={0};
char axis_z[64]={0};
int index_x =0;
int index_y =0;
int index_z =0;
int8_t filter_x;
int8_t filter_y;
int8_t filter_z;


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

int8_t formula(char eixo[4]){

   return(int8_t)(eixo[0]+ 0.6*eixo[1] +0.3*eixo[2]+ 0.1*eixo[3])/2;
}
 


//************************
//Threads

//Thread para geração de chaves
void thread_samples(void const *args){
//    osSignalWait(0x1, osWaitForever);
//    osSignalSet(id_thread_write_ram, 0x1);
//    osSignalWait(0x1, osWaitForever);
    
    osEvent evt;
    char queue_x[4]={0};
    char queue_y[4]={0};
    char queue_z[4]={0};
      
    while(1){
      evt = osSignalWait (0x02, 250);
      if (evt.status == osEventSignal){
          osSignalWait (0x00,1000);
      }else if(evt.status == osOK){
          
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
          
          queue_x[3] = queue_x[2];
          queue_x[2] = queue_x[1];
          queue_x[1] = queue_x[0];
          queue_x[0] = x;
          filter_x =formula(queue_x);
          
          queue_y[3] = queue_y[2];
          queue_y[2] = queue_y[1];
          queue_y[1] = queue_y[0];
          queue_y[0] = y;
          filter_y =formula(queue_y);
          
          queue_z[3] = queue_z[2];
          queue_z[2] = queue_z[1];
          queue_z[1] = queue_z[0];
          queue_z[0] = z;
          filter_z =formula(queue_z);
          osSignalSet(id_thread_write_ram, 0x1);
      }
    }
    
}
osThreadDef(thread_samples, osPriorityNormal, 1, 0);

//Thread 
void thread_write_ram(void const *args){
    
     osEvent evt;
     evt = osSignalWait (0x03, osWaitForever);     
      if(evt.status == osEventSignal){
        if (evt.value.signals & 0x02){
            osSignalWait (0x00,1000);
        }else if (evt.value.signals & 0x01){             
          while(evt.value.signals & 0x08){
             osSignalClear(osThreadGetId(),0x08);
            osSignalWait (0x04,osWaitForever);
          }
          
            osSignalSet(id_thread_test_bar_red_leds, 0x08);
            osSignalSet(id_thread_test_bar_green_leds, 0x08);
            osSignalSet(id_thread_display_oled, 0x08);
            osSignalSet(id_thread_export_file, 0x08);
            
            index_x =(index_x+63)%64;
            index_y =(index_y+63)%64;
            index_z =(index_z+63)%64;
            
            axis_x[index_x] = filter_x;
            axis_y[index_y] = filter_y;
            axis_z[index_z] = filter_z;
            
            osSignalSet(id_thread_test_bar_red_leds, 0x04);
            osSignalSet(id_thread_test_bar_green_leds, 0x04);
            osSignalSet(id_thread_display_oled, 0x04);
            osSignalSet(id_thread_export_file, 0x04);
          
            osSignalSet(id_thread_test_bar_red_leds, 0x1);
            osSignalSet(id_thread_test_bar_green_leds, 0x1);
            osSignalSet(id_thread_display_oled, 0x1);
            osSignalSet(id_thread_export_file, 0x1);
          
      }

}
osThreadDef(thread_write_ram, osPriorityNormal, 1, 0);

//Thread para 
void thread_red_led(void const *args){
     

}
osThreadDef(thread_red_led, osPriorityNormal, 1, 0);

//Thread para 
void thread_green_led(void const *args){
  
}
osThreadDef(thread_green_led, osPriorityNormal, 1, 0);

//Thread para 
void thread_display_oled(void const *args){
      
     osEvent evt;
     evt = osSignalWait (0x03, osWaitForever);
     if(evt.status == osEventSignal){
        if (evt.value.signals & 0x02){
            osSignalWait (0x00,1000);
        }else if (evt.value.signals & 0x01){ 
           oled_clearScreen(OLED_COLOR_WHITE);
          while(evt.value.signals & 0x08){
             osSignalClear(osThreadGetId(),0x08);
             osSignalWait (0x04,osWaitForever);
          }
          osSignalSet(id_thread_write_ram, 0x08);
          for (int i=0 ; i< 64;i++){
            int j = axis_z[(i+index_z)%64]+128;
            j = j*64/256;
            oled_putPixel(i,j, OLED_COLOR_BLACK);
          }
          osSignalSet(id_thread_write_ram, 0x04);          
        }
        
     }   
          
}
osThreadDef(thread_display_oled, osPriorityNormal, 1, 0);

//Thread para 
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
  oled_init();
  joystick_init();
  I2CInit( (uint32_t)I2CMASTER, 0 );
  setup_isr();

  
  //************************
  //Inicialização de Threads aqui
  //************************
  id_thread_sample_ace =  osThreadCreate(osThread(thread_samples),       NULL);
  id_thread_write_ram =  osThreadCreate(osThread(thread_write_ram),       NULL);
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
  osThreadTerminate(id_thread_write_ram);
  osThreadTerminate(id_thread_test_bar_red_leds);
  osThreadTerminate(id_thread_test_bar_green_leds);  
  osThreadTerminate(id_thread_display_oled);
  
  osDelay(osWaitForever); 
  return 0;
}