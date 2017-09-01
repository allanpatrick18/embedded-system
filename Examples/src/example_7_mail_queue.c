#include "libdemo.h"
/*============================================================================
 *                  Exemplos de utilização do RTOS CMSIS
 *           LPCXpresso 1343 + Embedded Artists Development Board 
 *---------------------------------------------------------------------------*
 *                 Comunicação entre threads por Mail Queue
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
} mail_t;

osMailQDef(mail_box, 16, mail_t);
osMailQId  mail_box;
 
void send_thread (void const *args) {
    uint32_t i = 0;
    while (1) {
        i++;
        mail_t *mail = (mail_t*)osMailAlloc(mail_box, osWaitForever);
        mail->temp = get_temperature(); 
        mail->light = get_light();
        mail->trimpot = get_trimpot();
        mail->counter = i;
        osMailPut(mail_box, mail);
        osDelay(1000);
    }
} 
osThreadDef(send_thread, osPriorityNormal, 1, 0);
 
void receive_thread (void const *args) {
     while (1) {
        osEvent evt = osMailGet(mail_box, osWaitForever);
        if (evt.status == osEventMail) {
            mail_t *mail = (mail_t*)evt.value.p;
            printf("\nTemperature: %i \n\r", mail->temp);
            printf("Light: %u \n\r" , mail->light);
            printf("Trimpot: %u \n\r" , mail->trimpot);
            printf("Number of messages: %u\n\r", mail->counter);
            
            osMailFree(mail_box, mail);
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
   
    mail_box = osMailCreate(osMailQ(mail_box), NULL);
  
    osThreadCreate(osThread(send_thread), NULL);
    osThreadCreate(osThread(receive_thread), NULL);
        
    osKernelStart();
    osDelay(osWaitForever); 
}