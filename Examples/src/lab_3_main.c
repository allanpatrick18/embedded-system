#include "mcu_regs.h"
#include "type.h"
#include "ssp.h"
#include "oled.h"

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

void draw_player(uint8_t pos_y, uint8_t pos_x, uint8_t height){
  oled_rect(pos_x, pos_y, pos_x+THICK, pos_y+height, FOREGROUND);
}

void draw_score(uint8_t score, uint8_t pos_x){
  oled_putChar(pos_x, SCORE_Y, '0' + score%10, FOREGROUND, BACKGROUND);
}

void draw_ball(uint8_t pos_x, uint8_t pos_y){
  oled_rect(pos_x, pos_y, pos_x+THICK, pos_y+THICK, FOREGROUND);
}

int main(char** args, int n_args){
  SSPInit();
  oled_init();
  
  clear_scene();
  draw_score(3, SCORE_P1_X);
  draw_score(0, SCORE_P2_X);
  draw_table();
  draw_player(V_CENTER-BAR_CENTER, P1_X, BAR_HEIGHT);
  draw_player(V_CENTER-BAR_CENTER, P2_X, BAR_HEIGHT);
  draw_ball(P1_X+2*THICK, V_CENTER);
  return 0;
}