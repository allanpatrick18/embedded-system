#include "libdemo.h"
/*============================================================================
 *                  Exemplos de utilização do RTOS CMSIS
 *           LPCXpresso 1343 + Embedded Artists Development Board 
 *---------------------------------------------------------------------------*
 *                   Gerenciamento de múltiplas Threads
 *---------------------------------------------------------------------------*
 *                    Prof. André Schneider de Oliveira
 *            Universidade Tecnológica Federal do Paraná (UTFPR)
 *===========================================================================*/
 
void led2_thread(void const *args) {
    while (1) {
        pca_toggle(2);
        osDelay(1000);
    }
}
osThreadDef(led2_thread, osPriorityNormal, 1, 0);
 
int main() {
  
    osKernelInitialize();

    I2CInit( (uint32_t)I2CMASTER, 0 );
  
    osThreadCreate(osThread(led2_thread), NULL);
      
    osKernelStart();
    
    while (1) {
        pca_toggle(1);
        osDelay(500);
    }
}
