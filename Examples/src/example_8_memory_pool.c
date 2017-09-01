#include "libdemo.h"
/*============================================================================
 *                  Exemplos de utilização do RTOS CMSIS
 *           LPCXpresso 1343 + Embedded Artists Development Board 
 *---------------------------------------------------------------------------*
 *                 Comunicação entre threads por Memory Pool
 *---------------------------------------------------------------------------*
 *                    Prof. André Schneider de Oliveira
 *            Universidade Tecnológica Federal do Paraná (UTFPR)
 *===========================================================================
 * Obs: Abra a janela Terminal I/O no Debugger - Menu View/Terminal I/O
 *===========================================================================*/

typedef struct {
    int32_t     temp;           /* AD result of measured temperature      */
    uint32_t    light;          /* AD result of measured light            */
    uint32_t    trimpot;        /* AD result of measured trimpot voltage  */
    uint32_t    counter;        /* message counter                        */
} message_t;
 
osPoolDef(mpool, 16, message_t);
osPoolId  mpool;
 
osMessageQDef(queue, 16, message_t);
osMessageQId  queue;
 
void send_thread (void const *args) {
    uint32_t i = 0;
    while (1) {
        i++; 
        message_t *message = (message_t*)osPoolAlloc(mpool);
        message->temp = get_temperature(); 
        message->light = get_light();
        message->trimpot = get_trimpot();
        message->counter = i;
        osMessagePut(queue, (uint32_t)message, osWaitForever);
        osDelay(1000);
    }
}
osThreadDef(send_thread, osPriorityNormal, 1,0);

void receive_thread (void const *args) {
     while (1) {
        osEvent evt = osMessageGet(queue, osWaitForever);
        if (evt.status == osEventMessage) {
            message_t *message = (message_t*)evt.value.p;
            printf("\nTemperature: %i \n\r", message->temp);
            printf("Light: %u \n\r"     , message->light);
            printf("Trimpot: %u \n\r"     , message->trimpot);
            printf("Number of messages: %u\n\r", message->counter);
            
            osPoolFree(mpool, message);
        }
    }
} 
osThreadDef(receive_thread, osPriorityNormal, 1,0);


int main (void) {
  
    osKernelInitialize();

    I2CInit( (uint32_t)I2CMASTER, 0 );
    ADCInit( ADC_CLK );

    light_init();
    acc_init();
    temp_init (&getTicks);
   
    mpool = osPoolCreate(osPool(mpool));
    queue = osMessageCreate(osMessageQ(queue), NULL);
    
    osThreadCreate(osThread(send_thread), NULL);
    osThreadCreate(osThread(receive_thread), NULL);
        
    osKernelStart();
    osDelay(osWaitForever);  
 }