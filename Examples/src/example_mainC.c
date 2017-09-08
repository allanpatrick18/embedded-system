#include "mcu_regs.h"
#include "type.h"
#include "stdio.h"

#define figura 0x00004000
#define imagem 0x00004034
uint8_t matriz[4][4];
extern int f_asm(void);
int main()
{
  int i;  
  
  i = f_asm( );
 
  uint8_t *p = (uint8_t *) imagem;
  
  for (uint8_t y=0;y<64;y++)
    for (uint8_t x=0;x<96;x++){
        printf("%u\n",*p);
        p++;
  }    
  
  return 0;
}

