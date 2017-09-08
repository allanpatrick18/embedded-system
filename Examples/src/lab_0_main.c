/*============================================================================
 *           LPCXpresso 1343 + Embedded Artists Development Board 
 *---------------------------------------------------------------------------*
 *            Universidade Tecnológica Federal do Paraná (UTFPR)
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

//Endereço da imagem
#define FIG 0x300
//Tamanho do header ppm
#define HEADER_SIZE 0x34
//Dimensões da imagem
#define WDT 96
#define HGT 64
//Limiar de cor que separa preto do branco
#define THRES 127
//Numero de canais da imagem
#define CHN 3

//Fator de translação (quanto cada translação move a imagem em pixeis)
#define TRANSL_FACTOR 1
//Fator de magnificação (quanto cada zoom aumenta ou diminui da imagem) ex.: 1.2 = 20%
#define ZOOM_FACTOR 1.2

//Tamanho das LUT trigonométricas
#define TRIG_LUT_SIZE 16

//Definições para utilizar booleans
#define true 1
#define false 0
typedef uint8_t bool;

//Valor de rotação (indice nas LUTs de seno e cosseno
typedef uint8_t rotation_t;
rotation_t rotation = 0;

//Valores para translação em x e y atual
int translate_x = 0;
int translate_y = 0;

//Valor para magnificação atual
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

//Verifica se houve uma modificação na imagem e ela precisa ser reprocessada
bool dirty = true;

// struct para armazenar imagem pre-carregada na memoria
typedef struct ppm_img{
  uint8_t raw_header[HEADER_SIZE];
  uint8_t img[HGT][WDT][CHN];
} ppm_img_t;
#pragma location=FIG
//Imagem ppm na memoria
extern const ppm_img_t preloaded_img;

//Funções em assembly
extern void transform_coordinates(int* x, int* y);
extern bool binarization(uint8_t color);

//Inicialização de botões SW3 e SW4
void buttons_init(){
    GPIOSetDir( PORT2, 9, 0 );  //SW3 
    GPIOSetDir( PORT1, 4, 0 );  //SW4 
}

//Recupera pixel na memoria, aplicando matriz de transformação equivalente
uint8_t get_pixel_at(int x, int y){
  //aplica matriz de transformação
  transform_coordinates(&x, &y);
  
  // retorna branco quando as coordenadas não 
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
  //Inicializações
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
    
    // inverte cores quando ambos switches são pressionados juntos
    if(sw3_sample && sw4_sample){
      invert_color = !invert_color;
      dirty = true;
    }
    else{
      // sw3 realiza zoom out
      // verifica se operação sobre zoom não passará o minimo (zoom = 0)
      if(sw3_sample && zoom/ZOOM_FACTOR > 0){
        zoom = zoom/ZOOM_FACTOR;
        dirty = true;
      }
      // sw4 realiza zoom in
      // verifica se operação sobre zoom não passará o maximo (zoom = 0)
      if(sw4_sample && zoom*ZOOM_FACTOR > 0){
        zoom = zoom*ZOOM_FACTOR;
        dirty = true;
      }
    }
    
    // leitura do encoder, onde o valor [...] corresponde à orientação [...]:
    // 1- horario; 2- anti-horario
    uint8_t rotary_sample = rotary_read();
    
    //Se leitura não for zero
    if(rotary_sample){
      // positivo (1) se CW ou negativo se CCW (-1)
      int rotary_delta = ((int)(rotary_sample&0x1))-((int)(rotary_sample&0x2)>>1);
      // valor de rotação pertence ao intervalo [0, TRIG_LUT_SIZE[ e esta 
      // instrução garante que permanece nele quando se aplicado o delta
      rotation = (TRIG_LUT_SIZE+rotation+rotary_delta)%TRIG_LUT_SIZE;
      dirty = true;
    }
    
    //dirty bit indica que imagem foi alterada
    if(dirty){
      for(int i = 0; i < WDT; i++){
        for(int j = 0; j < HGT; j++){
          // recupera valor de pixel para imprimir na posição (i,j) do OLED
          uint8_t color = get_pixel_at(i, j);
          // recupera indicador de qual cor se pintara o pixel com esse valor
          bool isFilled = binarization(color);
          oled_putPixel(i, j, isFilled ? OLED_COLOR_BLACK : OLED_COLOR_WHITE);
        }
      }
      dirty = false;
    }

    //delay de 15 ms para atualização de tela
    delay32Ms(0, 15);
  }
}