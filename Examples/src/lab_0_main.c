/*============================================================================
 *           LPCXpresso 1343 + Embedded Artists Development Board 
 *---------------------------------------------------------------------------*
 *            Universidade Tecnol�gica Federal do Paran� (UTFPR)
 *===========================================================================*/
#include "mcu_regs.h"
#include "type.h"
#include "stdio.h"
#include "timer32.h"
#include "gpio.h"
#include "ssp.h"
#include "oled.h"
#include "pca9532.h"
#include "joystick.h"
#include "mcu_regs.h"
#include "type.h"
#include "stdio.h"
#include "rotary.h"

//Endere�o da imagem
#define FIG 0x300
//Tamanho do header ppm
#define HEADER_SIZE 0x34
//Dimens�es da imagem
#define WDT 96
#define HGT 64
//Limiar de cor que separa preto do branco
#define THRES 127
//Numero de canais da imagem
#define CHN 3

//Fator de transla��o (quanto cada transla��o move a imagem em pixeis)
#define TRANSL_FACTOR 1
//Fator de magnifica��o (quanto cada zoom aumenta ou diminui da imagem) ex.: 1.2 = 20%
#define ZOOM_FACTOR 1.2

//Tamanho das LUT trigonom�tricas
#define TRIG_LUT_SIZE 16

//Defini��es para utilizar booleans
#define true 1
#define false 0
typedef uint8_t bool;

//Valor de rota��o (indice nas LUTs de seno e cosseno
typedef uint8_t rotation_t;
rotation_t rotation = 0;

//Valores para transla��o em x e y atual
int translate_x = 0;
int translate_y = 0;

//Valor para magnifica��o atual
float zoom = 1;

//A imagem esta com a cor invertida?
bool invert_color = false;

//Lookup table de seno e cosseno
float lut_cos[TRIG_LUT_SIZE] = { 1,  0.924,  0.707,  0.383, 
                                 0, -0.383, -0.707, -0.924, 
                                -1, -0.924, -0.707, -0.383,  
                                 0,  0.383,  0.707,  0.924};

float lut_sin[TRIG_LUT_SIZE] = { 0,  0.383,  0.707,  0.924,
                                 1,  0.924,  0.707,  0.383,
                                 0, -0.383, -0.707, -0.924,
                                 -1, -0.924, -0.707, -0.383};

//Verifica se houve uma modifica��o na imagem e ela precisa ser reprocessada
bool dirty = true;

// struct para armazenar imagem pre-carregada na memoria
typedef struct ppm_img{
  uint8_t raw_header[HEADER_SIZE];
  uint8_t img[HGT][WDT][CHN];
} ppm_img_t;
#pragma location=FIG
//Imagem ppm na memoria
extern const ppm_img_t preloaded_img;

//Fun��es em assembly
extern void transform_coordinates(int* x, int* y);
extern bool binarization(uint8_t color);

//Inicializa��o de bot�es SW3 e SW4
void buttons_init(){
    GPIOSetDir( PORT2, 9, 0 );  //SW3 
    GPIOSetDir( PORT1, 4, 0 );  //SW4 
}

//Recupera pixel na memoria, aplicando matriz de transforma��o equivalente
uint8_t get_pixel_at(int x, int y){
  //aplica matriz de transforma��o
  transform_coordinates(&x, &y);
  
  // retorna branco quando as coordenadas n�o 
  // estiverem dentro dos limites da imagem
  if(x < 0 || x >= WDT || y < 0 || y >= HGT)
    return 0xFF;
  //realiza media dos canais para retornar intensidade em escala de cinza
  return (((int)preloaded_img.img[y][x][0])+
          ((int)preloaded_img.img[y][x][1])+
          ((int)preloaded_img.img[y][x][2]))/3;
}

int main (void)
{
  //Inicializa��es
  SystemCoreClockUpdate();

  GPIOInit();
  init_timer32(0, 10);

  SSPInit();

  oled_init();
  oled_clearScreen(OLED_COLOR_WHITE);

  joystick_init();
  buttons_init();
  rotary_init();

  while(1) { 
    uint8_t joystick_sample = joystick_read();
    // joystick central reinicia imagem
    if(joystick_sample&JOYSTICK_CENTER){
      rotation = 0;
      translate_x = 0;
      translate_y = 0;
      zoom = 1;
      invert_color = false;
      dirty = true;
    }
    // joystick esquerda move a imagem para esquerda JOYSTICK_LEFT pixels
    if(joystick_sample&JOYSTICK_LEFT){
      translate_x -= TRANSL_FACTOR;
      dirty = true;
    }
    // joystick direita move a imagem para direita JOYSTICK_RIGHT pixels
    if(joystick_sample&JOYSTICK_RIGHT){
      translate_x += TRANSL_FACTOR;
      dirty = true;
    }
    // joystick cima move a imagem para cima JOYSTICK_UP pixels
    if(joystick_sample&JOYSTICK_UP){
      translate_y -= TRANSL_FACTOR;
      dirty = true;
    }
    // joystick baixo move a imagem para baixo JOYSTICK_DOWN pixels
    if(joystick_sample&JOYSTICK_DOWN){
      translate_y += TRANSL_FACTOR;
      dirty = true;
    }
    
    // leitura de switches
    uint8_t sw3_sample = !GPIOGetValue(PORT2, 9);
    uint8_t sw4_sample = !GPIOGetValue(PORT1, 4);
    
    // inverte cores quando ambos switches s�o pressionados juntos
    if(sw3_sample && sw4_sample){
      invert_color = !invert_color;
      dirty = true;
    }
    else{
      // sw3 realiza zoom out
      // verifica se opera��o sobre zoom n�o passar� o minimo (zoom = 0)
      if(sw3_sample && zoom/ZOOM_FACTOR > 0){
        zoom = zoom/ZOOM_FACTOR;
        dirty = true;
      }
      // sw4 realiza zoom in
      // verifica se opera��o sobre zoom n�o passar� o maximo (zoom = 0)
      if(sw4_sample && zoom*ZOOM_FACTOR > 0){
        zoom = zoom*ZOOM_FACTOR;
        dirty = true;
      }
    }
    
    // leitura do encoder, onde o valor [...] corresponde � orienta��o [...]:
    // 1- horario; 2- anti-horario
    uint8_t rotary_sample = rotary_read();
    
    //Se leitura n�o for zero
    if(rotary_sample){
      // positivo (1) se CW ou negativo se CCW (-1)
      int rotary_delta = ((int)(rotary_sample&0x1))-((int)(rotary_sample&0x2)>>1);
      // valor de rota��o pertence ao intervalo [0, TRIG_LUT_SIZE[ e esta 
      // instru��o garante que permanece nele quando se aplicado o delta
      rotation = (TRIG_LUT_SIZE+rotation+rotary_delta)%TRIG_LUT_SIZE;
      dirty = true;
    }
    
    //dirty bit indica que imagem foi alterada
    if(dirty){
      for(int i = 0; i < WDT; i++){
        for(int j = 0; j < HGT; j++){
          // recupera valor de pixel para imprimir na posi��o (i,j) do OLED
          uint8_t color = get_pixel_at(i, j);
          // recupera indicador de qual cor se pintara o pixel com esse valor
          bool isFilled = binarization(color);
          oled_putPixel(i, j, isFilled ? OLED_COLOR_BLACK : OLED_COLOR_WHITE);
        }
      }
      dirty = false;
    }

    //delay de 15 ms para atualiza��o de tela
    delay32Ms(0, 15);
  }
}