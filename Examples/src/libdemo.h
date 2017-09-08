#include <stdio.h>
#include "cmsis_os.h"
#include "type.h"
#include "LPC13xx.H"            
#include "gpio.h"
#include "rgb.h"
#include "oled.h"
#include "led7seg.h"
#include "i2c.h"
#include "ssp.h"
#include "pca9532.h"
#include "adc.h"
#include "light.h"
#include "temp.h"
#include "acc.h"
#include "joystick.h"

// Funcoes para demonstração do CMSIS-RTOS

    uint16_t leds = 0x0000, toggle_pca = 0x0000, not_toggle_pca; 
    uint8_t seg = '1'; 
    static uint8_t buf[10];
    static uint32_t msTicks = 0;
    
static uint32_t getTicks(void)
{
    return msTicks;
}
    
    int32_t  get_temperature(){   
        return temp_read();
    }
  
    uint32_t get_light(){
        return light_read();
    }

    uint32_t get_trimpot(){
        return ADCRead(0);
    }

void pca_toggle(int x){
  toggle_pca ^= 1 << x;
  not_toggle_pca = ~toggle_pca;
  pca9532_setLeds(toggle_pca,not_toggle_pca);
}

void led_bar(){
      if (leds == 0x0000) {
          leds = 0x0001;
          pca9532_setLeds(leds,0x00ff);
      }      
      else {  
       leds=leds*2;
       if (leds == 0x100) {
          leds = 0x0000;
          pca9532_setLeds(leds,0x00ff);
       }        
        else
        pca9532_setLeds(leds,leds/2);
      }
      

}

void seg_count(){
   led7seg_setChar(seg,FALSE);
      seg++; 
      if (seg=='9') seg='1'; 
}

static void intToString(int value, uint8_t* pBuf, uint32_t len, uint32_t base)
{
    static const char* pAscii = "0123456789abcdefghijklmnopqrstuvwxyz";
    int pos = 0;
    int tmpValue = value;

    // the buffer must not be null and at least have a length of 2 to handle one
    // digit and null-terminator
    if (pBuf == NULL || len < 2)
    {
        return;
    }

    // a valid base cannot be less than 2 or larger than 36
    // a base value of 2 means binary representation. A value of 1 would mean only zeros
    // a base larger than 36 can only be used if a larger alphabet were used.
    if (base < 2 || base > 36)
    {
        return;
    }

    // negative value
    if (value < 0)
    {
        tmpValue = -tmpValue;
        value    = -value;
        pBuf[pos++] = '-';
    }

    // calculate the required length of the buffer
    do {
        pos++;
        tmpValue /= base;
    } while(tmpValue > 0);


    if (pos > len)
    {
        // the len parameter is invalid.
        return;
    }

    pBuf[pos] = '\0';

    do {
        pBuf[--pos] = pAscii[value % base];
        value /= base;
    } while(value > 0);

    return;

}

volatile unsigned long *porta1_IS  = (volatile unsigned long *)0x50018004;
volatile unsigned long *porta1_IBE = (volatile unsigned long *)0x50018008;
volatile unsigned long *porta1_IEV = (volatile unsigned long *)0x5001800C;
volatile unsigned long *porta2_IS  = (volatile unsigned long *)0x50028004;
volatile unsigned long *porta2_IBE = (volatile unsigned long *)0x50028008;
volatile unsigned long *porta2_IEV = (volatile unsigned long *)0x5002800C;

void setup_port(uint32_t port, uint32_t bitPosi, uint32_t sense, uint32_t single, uint32_t event)
{
    switch (port) {
    case 1:

        if (sense == 0)
        {
            *porta1_IS &= ~(0x1 << bitPosi);

            if (single == 0) *porta1_IBE &= ~(0x1 << bitPosi);
            else *porta1_IBE |= (0x1 << bitPosi);
        }
        else *porta1_IS |= (0x1 << bitPosi);

        if (event == 0) *porta1_IEV &= ~(0x1 << bitPosi);
        else *porta1_IEV |= (0x1 << bitPosi);
        break;

    case 2:

        if (sense == 0)
        {
            *porta2_IS &= ~(0x1 << bitPosi);

            if (single == 0) *porta2_IBE &= ~(0x1 << bitPosi);
            else *porta2_IBE |= (0x1 << bitPosi);
        }
        else *porta2_IS |= (0x1 << bitPosi);

        if (event == 0) *porta2_IEV &= ~(0x1 << bitPosi);
        else *porta2_IEV |= (0x1 << bitPosi);
        break;
    }
}