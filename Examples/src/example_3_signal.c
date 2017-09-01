#include "libdemo.h"
/*============================================================================
 *                  Exemplos de utilização do RTOS CMSIS
 *           LPCXpresso 1343 + Embedded Artists Development Board 
 *---------------------------------------------------------------------------*
 *                   Sincronismo de Threads com Sinais
 *---------------------------------------------------------------------------*
 *                    Prof. André Schneider de Oliveira
 *            Universidade Tecnológica Federal do Paraná (UTFPR)
 *===========================================================================*/ 
 
void led_thread(void const *args) {
    while (1) {
        // Signal flags that are reported as event are automatically cleared.
        osSignalWait(0x1, osWaitForever);
         pca_toggle(0);
    }
}
osThreadDef(led_thread, osPriorityNormal, 1,0);
 
int main (void) {

    osKernelInitialize();

    I2CInit( (uint32_t)I2CMASTER, 0 );

    osThreadId tid = osThreadCreate(osThread(led_thread), NULL);

    osKernelStart();
    
    while (1) {
        osDelay(1000);
        osSignalSet(tid, 0x1);
    }
}