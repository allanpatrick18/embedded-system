#include "libdemo.h"
/*============================================================================
 *                  Exemplos de utilização do RTOS CMSIS
 *           LPCXpresso 1343 + Embedded Artists Development Board 
 *---------------------------------------------------------------------------*
 *                 Acesso a recursos compartilhados por Mutex 
 *---------------------------------------------------------------------------*
 *                    Prof. André Schneider de Oliveira
 *            Universidade Tecnológica Federal do Paraná (UTFPR)
  *===========================================================================
 * Obs: Abra a janela Terminal I/O no Debugger - Menu View/Terminal I/O
 *===========================================================================*/

osMutexId stdio_mutex;
osMutexDef(stdio_mutex);
 
void notify(const char* name, int state) {
    osMutexWait(stdio_mutex, osWaitForever);
    printf("%s: %d\n\r", name, state);
    osMutexRelease(stdio_mutex);
}
 
void test_thread(void const *args) {
    while (1) {
        notify((const char*)args, 0); 
        osDelay(1000);
        notify((const char*)args, 1); 
        osDelay(1000);
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
 
int main() {
    stdio_mutex = osMutexCreate(osMutex(stdio_mutex));
    
    osThreadCreate(osThread(t2), NULL);
    osThreadCreate(osThread(t3), NULL);
    
    test_thread((void *)"Thread 1");
    
    osDelay(osWaitForever); 
}
