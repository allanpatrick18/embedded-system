#include "libdemo.h"
/*============================================================================
 *                  Exemplos de utilização do RTOS CMSIS
 *           LPCXpresso 1343 + Embedded Artists Development Board 
 *---------------------------------------------------------------------------*
 *                 Acesso a recursos compartilhados por Semaforo
 *---------------------------------------------------------------------------*
 *                    Prof. André Schneider de Oliveira
 *            Universidade Tecnológica Federal do Paraná (UTFPR)
 *===========================================================================
 * Obs: Abra a janela Terminal I/O no Debugger - Menu View/Terminal I/O
 *===========================================================================*/
 
osSemaphoreId two_slots;
osSemaphoreDef(two_slots);
 
void test_thread(void const *name) {
    while (1) {
        osSemaphoreWait(two_slots, osWaitForever);
        printf("%s\n\r", (const char*)name);
        osDelay(1000);
        osSemaphoreRelease(two_slots);
    }
}
 
void t2(void const *argument) {
      test_thread("Thread 2");
}
osThreadDef(t2, osPriorityNormal, 1, 0);
 
void t3(void const *argument) {
      test_thread("Thread 3");
}
osThreadDef(t3, osPriorityNormal, 1, 0);
 
int main () {
  
    osKernelInitialize();

  
    two_slots = osSemaphoreCreate(osSemaphore(two_slots), 2);
    
    osThreadCreate(osThread(t2), NULL);
    osThreadCreate(osThread(t3), NULL);
    
    osKernelStart();
    
    test_thread((void *)"Thread 1");
    
    osDelay(osWaitForever); 
  
}