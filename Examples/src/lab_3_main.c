#include "mcu_regs.h"
#include "type.h"
#include "ssp.h"
#include "gpio.h"
#include "uart.h"
#include "oled.h"
#include "ctype.h"
#include "timer32.h"

#define FPS_24

//Definições físicas
#define V_MAX 3

#ifdef FPS_24
#define REFRESH_RATE 40
#else
#define REFRESH_RATE 80
#endif

//Definições de Layout
#define BACKGROUND OLED_COLOR_BLACK
#define FOREGROUND OLED_COLOR_WHITE

#define THICK 1

#define P1_GOAL_BORDER (0)
#define P2_GOAL_BORDER (95)

#define UPPER_BNDS (01)
#define DOWN_BNDS  (62) 
#define LEFT_BNDS  (01)
#define RIGHT_BNDS (94)

#define V_CENTER   ((DOWN_BNDS-UPPER_BNDS)/2+UPPER_BNDS)
#define H_CENTER   ((RIGHT_BNDS-LEFT_BNDS)/2+LEFT_BNDS)

#define P1_X (LEFT_BNDS)
#define P2_X (RIGHT_BNDS - THICK)
#define BAR_HEIGHT (8)
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
  if(abs(height) > 0)
    oled_rect(pos_x, pos_y, pos_x+THICK, pos_y+height-1, color);
}

void draw_score(uint8_t score, uint8_t pos_x){
  oled_putChar(pos_x, SCORE_Y, '0' + score%10, FOREGROUND, BACKGROUND);
}

void draw_ball(uint8_t pos_x, uint8_t pos_y, oled_color_t color){
  oled_rect(pos_x, pos_y, pos_x+THICK, pos_y+THICK, color);
}

int clamp(int min, uint8_t max, int value){
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
  init_timer32(1, 10);
 
  clear_scene();

  uint8_t bell = 0x07;

  int8_t p1_y = V_CENTER-BAR_CENTER;
  int8_t p2_y = V_CENTER-BAR_CENTER;
  uint8_t p1_last_y = p1_y;
  uint8_t p2_last_y = p2_y;
  
  int8_t v_p1_y = 0;
  int8_t v_p2_y = 0;
        
  float ball_x = H_CENTER-(THICK/2+1);
  float ball_y = V_CENTER-(THICK/2+1);
  
  float ball_vx = 1;
  float ball_vy = 1;
  
  uint8_t ball_last_x = (uint8_t) ball_x;
  uint8_t ball_last_y = (uint8_t) ball_y;

  while(1){
    //Input Usuário
    uint8_t rec = 0;
    UARTReceive(&rec, 1, 0);
    if(rec != 0)
      UARTSend(&rec, 1);
    
    if(rec == 'w')
      v_p1_y--;
    if(rec == 's')
      v_p1_y++;
    if(rec == 'i')
      v_p2_y--;
    if(rec == 'k')
      v_p2_y++;
    
    //Controll players
    v_p1_y = (int8_t) clamp(-V_MAX, V_MAX, v_p1_y);
    v_p2_y = (int8_t) clamp(-V_MAX, V_MAX, v_p2_y);

    p1_last_y = p1_y;
    p2_last_y = p2_y;
    
    p1_y += v_p1_y;
    p2_y += v_p2_y;
      
    p1_y = (uint8_t) clamp(UPPER_BNDS+2*THICK, DOWN_BNDS-2*THICK-BAR_HEIGHT+1, p1_y);
    p2_y = (uint8_t) clamp(UPPER_BNDS+2*THICK, DOWN_BNDS-2*THICK-BAR_HEIGHT+1, p2_y);

    if(p1_last_y == p1_y) v_p1_y = 0;
    if(p2_last_y == p2_y) v_p2_y = 0;
    
    //Print players
    uint8_t delta_y_p1 = p1_y-p1_last_y;
    draw_player(p1_y > p1_last_y ? p1_last_y : p1_last_y + BAR_HEIGHT-1, 
                P1_X, delta_y_p1, BACKGROUND);
    draw_player(p1_y, P1_X, BAR_HEIGHT, FOREGROUND);

    uint8_t delta_y_p2 = p2_y-p2_last_y;
    draw_player(p2_y > p2_last_y ? p2_last_y : p2_last_y + BAR_HEIGHT-1, 
                P2_X, delta_y_p2, BACKGROUND);
    draw_player(p2_y, P2_X, BAR_HEIGHT, FOREGROUND);
    
    //Controll ball
    ball_last_x = (uint8_t) ball_x;
    ball_last_y = (uint8_t) ball_y;

    ball_x += ball_vx;
    ball_y += ball_vy;
    
    if(ball_y < UPPER_BNDS + 2*THICK){
      ball_y = (float) UPPER_BNDS + 2*THICK;
      ball_vy = -ball_vy;
    }
    if(ball_y + 2*THICK > DOWN_BNDS - THICK ){
      ball_y = (float) DOWN_BNDS - 3*THICK;
      ball_vy = -ball_vy;
    }
    
    if(ball_x < LEFT_BNDS + THICK && 
          p1_y < ball_y + 2* THICK &&
          p1_y + BAR_HEIGHT > ball_y){
      ball_x = (float) LEFT_BNDS + THICK;
      ball_vx = -ball_vx;
     }
    if(ball_x+2*THICK > RIGHT_BNDS - THICK &&
          p2_y < ball_y + 2* THICK &&
          p2_y + BAR_HEIGHT > ball_y){
      ball_x = (float) RIGHT_BNDS - 3*THICK;
      ball_vx = -ball_vx;
    }
    if(ball_x < P1_GOAL_BORDER){
      ball_x = H_CENTER-(THICK/2+1);
      ball_y = V_CENTER-(THICK/2+1);
      UARTSend(&bell, 1);
    }
    if(ball_x+2*THICK > P2_GOAL_BORDER){
      ball_x = H_CENTER-(THICK/2+1);
      ball_y = V_CENTER-(THICK/2+1);
      UARTSend(&bell, 1);
    }
    
    //Print ball
    draw_ball(ball_last_x, ball_last_y, BACKGROUND);
    draw_ball((uint8_t) ball_x, (uint8_t) ball_y, FOREGROUND);
    
    //Print table and scores
    draw_score(3, SCORE_P1_X);
    draw_score(0, SCORE_P2_X);
    draw_table();
    
    delay32Ms(1, REFRESH_RATE);
  } 
  return 0;
}