#include "libdemo.h"
/*============================================================================
 *                  Exemplos de utilização do RTOS CMSIS
 *           LPCXpresso 1343 + Embedded Artists Development Board 
 *---------------------------------------------------------------------------*
 *              Chamadas RTOS de Interrupt Service Routines (ISR)
 *---------------------------------------------------------------------------*
 *                    Prof. André Schneider de Oliveira
 *            Universidade Tecnológica Federal do Paraná (UTFPR)
 *===========================================================================
 * Obs: Abra a janela Terminal I/O no Debugger - Menu View/Terminal I/O
 *===========================================================================*/
osThreadId isr_id;
 
void PIOINT2_IRQHandler(void)
{
    if (!GPIOGetValue(PORT2, 0))
      printf("Joystick center\n");
    
    if (!GPIOGetValue(PORT2, 1))
      printf("Joystick down\n");
    
    if (!GPIOGetValue(PORT2, 2))
      printf("Joystick right\n");
    
    if (!GPIOGetValue(PORT2, 3))
      printf("Joystick up\n");
    
    if (!GPIOGetValue(PORT2, 4))
      printf("Joystick left\n");
 
   osSignalSet(isr_id, 0x1);
    
    GPIOIntClear(2, 0);
    GPIOIntClear(2, 1);
    GPIOIntClear(2, 2);
    GPIOIntClear(2, 3);
    GPIOIntClear(2, 4);
}


void setup_isr()
{
    setup_port(PORT2, 0, 0, 1, 1);
    setup_port(PORT2, 1, 0, 1, 1);
    setup_port(PORT2, 2, 0, 1, 1);
    setup_port(PORT2, 3, 0, 1, 1);
    setup_port(PORT2, 4, 0, 1, 1);
    
    GPIOIntEnable(PORT2, 0); // JOYSTICK_CENTER
    GPIOIntEnable(PORT2, 1); // JOYSTICK_DOWN
    GPIOIntEnable(PORT2, 2); // JOYSTICK_RIGHT
    GPIOIntEnable(PORT2, 3); // JOYSTICK_UP
    GPIOIntEnable(PORT2, 4); // JOYSTICK_LEFT
}
 
void led_thread(void const *args) {
    while (1) {
        pca_toggle(2);
        osDelay(1000);
    }
}
osThreadDef(led_thread, osPriorityNormal, 1, 0);

void isr_thread(void const *args) {
    while (1) {
        // Signal flags that are reported as event are automatically cleared.
        osSignalWait(0x1, osWaitForever);
        pca_toggle(0);
    }
} 
osThreadDef(isr_thread, osPriorityHigh, 1, 0);


int main (void) {
      osKernelInitialize();

      GPIOInit();
      joystick_init();
      setup_isr();
      I2CInit( (uint32_t)I2CMASTER, 0 );

      osThreadCreate(osThread(led_thread), NULL);
      isr_id = osThreadCreate(osThread(isr_thread), NULL);
      
      osKernelStart();
      osDelay(osWaitForever);      
}


