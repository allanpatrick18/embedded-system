#include "libdemo.h"
/*============================================================================
 *                  Exemplos de utilização do RTOS CMSIS
 *           LPCXpresso 1343 + Embedded Artists Development Board 
 *---------------------------------------------------------------------------*
 *                             Diagrama de Gantt
 *---------------------------------------------------------------------------*
 *                 https://knsv.github.io/mermaid/live_editor/
 *               Documentação : https://knsv.github.io/mermaid/
 *---------------------------------------------------------------------------*
 *                    Prof. André Schneider de Oliveira
 *            Universidade Tecnológica Federal do Paraná (UTFPR)
 *===========================================================================*/
FILE *file;
int ticks_factor = 10000;

void led1_thread(void const *args) {
    uint32_t time;
    while (1) {
        time = osKernelSysTick()/ticks_factor;
        pca_toggle(1);
        osDelay(500);
        fprintf(file," Led1 : %i, %i\n", (int)time, (int)osKernelSysTick()/ticks_factor);
 
    }
}
osThreadDef(led1_thread, osPriorityNormal, 1, 0);

void led2_thread(void const *args) {
    uint32_t time;
    while (1) {
        time = osKernelSysTick()/ticks_factor;
        pca_toggle(2);
        osDelay(2000);
        fprintf(file," Led2 : done, %i, %i\n", (int)time, (int)osKernelSysTick()/ticks_factor);
 
    }
}
osThreadDef(led2_thread, osPriorityNormal, 1, 0);

void led3_thread(void const *args) {
    uint32_t time;
    while (1) {
      time = osKernelSysTick()/ticks_factor;
      pca_toggle(3);
      osDelay(300);
      fprintf(file," Led3 : crit, %i, %i\n", (int)time, (int)osKernelSysTick()/ticks_factor);
    }
}
osThreadDef(led3_thread, osPriorityHigh, 1, 0);
 
int main() {
  
    osKernelInitialize();
    I2CInit( (uint32_t)I2CMASTER, 0 );

    osThreadCreate(osThread(led1_thread), NULL);
    osThreadCreate(osThread(led2_thread), NULL);
    osThreadCreate(osThread(led3_thread), NULL);
    
    osKernelStart();
      
    file = fopen("gantt.txt","w");
   
    fprintf(file,"gantt\n");
    fprintf(file,"    title A Gantt Diagram\n");
    fprintf(file,"    dateFormat x\n");
    
    osDelay(osWaitForever); 
}
