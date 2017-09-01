#include "libdemo.h"
/*============================================================================
 *                  Exemplos de utilização do RTOS CMSIS
 *           LPCXpresso 1343 + Embedded Artists Development Board 
 *---------------------------------------------------------------------------*
 *                            Temporizadores
 *---------------------------------------------------------------------------*
 *                    Prof. André Schneider de Oliveira
 *            Universidade Tecnológica Federal do Paraná (UTFPR)
 *===========================================================================*/
 

void blink(void const *n) {
    pca_toggle((int)n);
}
 
osTimerDef(blink_0, blink);
osTimerDef(blink_1, blink);
osTimerDef(blink_2, blink);
osTimerDef(blink_3, blink);
 
int main(void) {
  
    osKernelInitialize();

    SystemInit();
    I2CInit( (uint32_t)I2CMASTER, 0 );
   
    osTimerId timer_0 = osTimerCreate(osTimer(blink_0), osTimerPeriodic, (void *)0);
    osTimerId timer_1 = osTimerCreate(osTimer(blink_1), osTimerPeriodic, (void *)1);
    osTimerId timer_2 = osTimerCreate(osTimer(blink_2), osTimerPeriodic, (void *)2);
    osTimerId timer_3 = osTimerCreate(osTimer(blink_3), osTimerPeriodic, (void *)3);
    
    osTimerStart(timer_0, 2000);
    osTimerStart(timer_1, 1000);
    osTimerStart(timer_2,  500);
    osTimerStart(timer_3,  250);
    
    osKernelStart();
    osDelay(osWaitForever);  
}