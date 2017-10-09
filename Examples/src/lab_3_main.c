#include "mcu_regs.h"
#include "type.h"
#include "ssp.h"
#include "gpio.h"
#include "uart.h"
#include "oled.h"
#include "timer32.h"

#define BACKGROUND OLED_COLOR_BLACK
#define FOREGROUND OLED_COLOR_WHITE

#define THICK 1

#define UPPER_BNDS (01)
#define DOWN_BNDS  (62) 
#define LEFT_BNDS  (01)
#define RIGHT_BNDS (94)

#define V_CENTER   ((DOWN_BNDS-UPPER_BNDS)/2+UPPER_BNDS)
#define H_CENTER   ((RIGHT_BNDS-LEFT_BNDS)/2+LEFT_BNDS)

#define P1_X (LEFT_BNDS)
#define P2_X (RIGHT_BNDS - THICK)
#define BAR_HEIGHT (7)
#define BAR_CENTER (BAR_HEIGHT/2)

#define SCORE_HEIGHT (8)
#define SCORE_WIDTH (5)
#define SCORE_Y (UPPER_BNDS+2*THICK+SCORE_HEIGHT/2)
#define SCORE_P1_X (H_CENTER-SCORE_WIDTH/2-2*THICK-SCORE_WIDTH)
#define SCORE_P2_X (H_CENTER-SCORE_WIDTH/2+2*THICK+SCORE_WIDTH)

void clear_scene(){
    oled_clearScreen(BACKGROUND);
}

void draw_table(){
  oled_rect(LEFT_BNDS,UPPER_BNDS, RIGHT_BNDS, UPPER_BNDS+THICK, FOREGROUND);
  oled_rect(LEFT_BNDS,DOWN_BNDS,  RIGHT_BNDS, DOWN_BNDS-THICK,  FOREGROUND);
  oled_rect(H_CENTER, UPPER_BNDS, H_CENTER+THICK, DOWN_BNDS, FOREGROUND);
}

void draw_player(uint8_t pos_y, uint8_t pos_x, uint8_t height, oled_color_t color){
  oled_rect(pos_x, pos_y, pos_x+THICK, pos_y+height, color);
}

void draw_score(uint8_t score, uint8_t pos_x){
  oled_putChar(pos_x, SCORE_Y, '0' + score%10, FOREGROUND, BACKGROUND);
}

void draw_ball(uint8_t pos_x, uint8_t pos_y){
  oled_rect(pos_x, pos_y, pos_x+THICK, pos_y+THICK, FOREGROUND);
}

uint8_t clamp(uint8_t min, uint8_t max, uint8_t value){
  if(value < min)
    return min;
  if(value > max)
    return max;
  return value;
}

int main(char** args, int n_args){
  
  GPIOInit();
  SSPInit();
  UARTInit(115200);
  oled_init();

  clear_scene();
  draw_score(3, SCORE_P1_X);
  draw_score(0, SCORE_P2_X);
  draw_table();
  draw_ball(P1_X+2*THICK, V_CENTER);

  uint8_t p1_y = V_CENTER-BAR_CENTER;
  uint8_t p2_y = V_CENTER-BAR_CENTER;
  uint8_t p1_y_last = p1_y;
  uint8_t p2_y_last = p2_y;
        
  while(1){
    uint8_t rec = 0;
    UARTReceive(&rec, 1, 0);
    if(rec != 0)
      UARTSend(&rec, 1);
    
    p1_y_last = p1_y;
    p2_y_last = p2_y;
    if(rec == 'w')
      p1_y--;
    if(rec == 's')
      p1_y++;
    if(rec == 'i')
      p2_y--;
    if(rec == 'k')
      p2_y++;
    p1_y = clamp(UPPER_BNDS+2*THICK, DOWN_BNDS-2*THICK-BAR_HEIGHT, p1_y);
    p2_y = clamp(UPPER_BNDS+2*THICK, DOWN_BNDS-2*THICK-BAR_HEIGHT, p2_y);

    draw_player(p1_y_last, P1_X, BAR_HEIGHT, BACKGROUND);
    draw_player(p2_y_last, P2_X, BAR_HEIGHT, BACKGROUND);
    draw_player(p1_y, P1_X, BAR_HEIGHT, FOREGROUND);
    draw_player(p2_y, P2_X, BAR_HEIGHT, FOREGROUND);
    delay32Ms(0, 80);
  } 
  return 0;
}