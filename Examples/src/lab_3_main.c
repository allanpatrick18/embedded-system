#include "mcu_regs.h"
#include "type.h"
#include "ssp.h"
#include "gpio.h"
#include "uart.h"
#include "oled.h"
#include "ctype.h"
#include "timer32.h"

#define FPS_24

#define true 1
#define false 0
typedef uint8_t bool;

//Game defines
#define MAX_SCORE 8

//Definições Físicas
#define V_BAR_MAX  (4)
#define V_BALL_MAX (8)
#define BALL_VOX (1)
#define BALL_VOY (1)
#define ACCEL_F (.02)

#ifdef FPS_24
#define REFRESH_RATE 40
#else
#define REFRESH_RATE 80
#endif

//Definições de Layout
#define BACKGROUND OLED_COLOR_BLACK
#define FOREGROUND OLED_COLOR_WHITE

#define THICK (2)

#define SCREEN_UP       (0)
#define SCREEN_DOWN     (63)
#define SCREEN_LEFT     (0)
#define SCREEN_RIGHT    (95)

#define P1_GOAL_BORDER SCREEN_LEFT
#define P2_GOAL_BORDER SCREEN_RIGHT

#define TABLE_UPPER     (SCREEN_UP      + THICK - 1)
#define TABLE_BOTTOM    (SCREEN_DOWN    - THICK + 1) 
#define TABLE_START     (SCREEN_LEFT    + THICK - 1)
#define TABLE_END       (SCREEN_RIGHT   - THICK + 1)

#define V_CENTER   ((TABLE_BOTTOM-TABLE_UPPER)/2 + TABLE_UPPER)
#define H_CENTER   ((TABLE_END-TABLE_START)   /2 + TABLE_START)

#define P1_X (TABLE_START)
#define P2_X (TABLE_END - THICK + 1)
#define BAR_HEIGHT (12)
#define BAR_CENTER (BAR_HEIGHT / 2)

#define SCORE_HEIGHT (7)
#define SCORE_WIDTH  (5)
#define SCORE_LAYOUT_Y (4)
#define SCORE_LAYOUT_X (8)
#define SCORE_Y (TABLE_UPPER+SCORE_LAYOUT_Y+SCORE_HEIGHT/2)
#define SCORE_P1_X (H_CENTER-SCORE_WIDTH/2-SCORE_LAYOUT_X-SCORE_WIDTH/2)
#define SCORE_P2_X (H_CENTER-SCORE_WIDTH/2+SCORE_LAYOUT_X+SCORE_WIDTH/2)

#define BALL_RADIUS (1)
#define BALL_DIAMETER (BALL_RADIUS*2)
#define BALL_CENTER (BALL_DIAMETER / 2)

//Inline functions
#define reset_ball(px, py, vx, vy, t) \
        px = H_CENTER-(BALL_CENTER+1);\
        py = V_CENTER-(BALL_CENTER+1);\
        vx = BALL_VOX*t?1:-1;\
        vy = BALL_VOY;\
          
#define isOver(x1, y1, w1, h1, x2, y2, w2, h2) \
      ( x1 + w1 >= x2      && \
        x1      <= x2 + w2 && \
        y1 + h1 >= y2      && \
        y1      <= y2 + h2 )

void clear_scene(){
    oled_clearScreen(BACKGROUND);
}

void beep(){
  static uint8_t bell = 0x07;
  UARTSend(&bell, 1);
}

void draw_table(uint8_t thickness, bool draw_sides){
  if(thickness > 0){
    if(draw_sides){
      oled_rect(TABLE_START,TABLE_UPPER, TABLE_END, SCREEN_UP, FOREGROUND);
      oled_rect(TABLE_START,TABLE_BOTTOM,  TABLE_END, SCREEN_DOWN,  FOREGROUND);
    }
    oled_rect(H_CENTER-thickness/2, TABLE_UPPER, H_CENTER+thickness/2+thickness%2-1, TABLE_BOTTOM, FOREGROUND);
  }
}

void draw_player(uint8_t pos_y, uint8_t pos_x, uint8_t height, oled_color_t color){
  if(height > 0)
    oled_rect(pos_x, pos_y, pos_x+THICK-1, pos_y+height-1, color);
}

void draw_score(uint8_t score, uint8_t pos_x){
  oled_putChar(pos_x, SCORE_Y, '0' + score%10, FOREGROUND, BACKGROUND);
}

void draw_ball(uint8_t pos_x, uint8_t pos_y, uint8_t radius, oled_color_t color){
  if(radius > 0)
    oled_rect(pos_x, pos_y, pos_x+radius-1, pos_y+radius-1, color);
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

  int8_t p1_y = V_CENTER-BAR_CENTER;
  int8_t p2_y = V_CENTER-BAR_CENTER;
  uint8_t p1_last_y = p1_y;
  uint8_t p2_last_y = p2_y;
  
  int8_t p1_vy = 0;
  int8_t p2_vy = 0;
        
  float ball_x = H_CENTER-BALL_CENTER;
  float ball_y = V_CENTER-BALL_CENTER;
  
  float ball_vx = BALL_VOX;
  float ball_vy = BALL_VOY;
  
  uint8_t ball_last_x = (uint8_t) ball_x;
  uint8_t ball_last_y = (uint8_t) ball_y;

  uint8_t p1_score = 0;
  uint8_t p2_score = 0;
  bool p1_score_changed = true;
  bool p2_score_changed = true;
  
  draw_table(THICK, true);
  
  while(1){
    //User input
    uint8_t rec = 0;
    UARTReceive(&rec, 1, 0);
//    if(rec != 0)
//      UARTSend(&rec, 1);
    
    if(rec == 'w')
      p1_vy--;
    if(rec == 's')
      p1_vy++;
    if(rec == 'i')
      p2_vy--;
    if(rec == 'k')
      p2_vy++;
    
    //Players controll
    p1_vy = (int8_t) clamp(-V_BAR_MAX, V_BAR_MAX, p1_vy);
    p2_vy = (int8_t) clamp(-V_BAR_MAX, V_BAR_MAX, p2_vy);

    p1_last_y = p1_y;
    p2_last_y = p2_y;
    
    p1_y += p1_vy;
    p2_y += p2_vy;
      
    p1_y = (uint8_t) clamp(TABLE_UPPER+1, TABLE_BOTTOM-BAR_HEIGHT, p1_y);
    p2_y = (uint8_t) clamp(TABLE_UPPER+1, TABLE_BOTTOM-BAR_HEIGHT, p2_y);

    if(p1_last_y == p1_y) p1_vy = 0;
    if(p2_last_y == p2_y) p2_vy = 0;
       
    //Ball controll
    ball_last_x = (uint8_t) ball_x;
    ball_last_y = (uint8_t) ball_y;

    //apply_velocity()
    ball_x += ball_vx;
    ball_y += ball_vy;
    
    //if(has_ball_collided_uppper_border)
    if(ball_y < TABLE_UPPER+1){
      ball_y = (float) TABLE_UPPER + 1;
      ball_vy = -ball_vy;
    }
    //if(has_ball_collided_bottom_border)
    if(ball_y + BALL_DIAMETER > TABLE_BOTTOM){
      ball_y = (float) TABLE_BOTTOM - (BALL_DIAMETER);
      ball_vy = -ball_vy;
    }
    
    //if(has_p1_collided_ball)
    if(ball_x < TABLE_START + THICK && 
          p1_y <= ball_y + BALL_DIAMETER &&
          p1_y + BAR_HEIGHT >= ball_y){
      ball_x = (float) TABLE_START + THICK;
      ball_vx = -ball_vx;
      ball_vy = (ball_y + BALL_CENTER - (p1_y + BAR_CENTER))/BAR_CENTER;
      float r = ball_vx*(1 + ACCEL_F + abs(p1_vy*p1_vy)*ACCEL_F);
      if(abs(r) <= V_BALL_MAX )
        ball_vx = r;
     }
    //if(has_p2_collided_ball)
    if(ball_x + BALL_DIAMETER > TABLE_END - THICK + 1&&
          p2_y <= ball_y + BALL_DIAMETER &&
          p2_y + BAR_HEIGHT >= ball_y){
      ball_x = (float) TABLE_END - (BALL_DIAMETER + THICK - 1);
      ball_vx = -ball_vx;
      ball_vy = (ball_y + BALL_CENTER - (p2_y + BAR_CENTER))/BAR_CENTER;
      float r = ball_vx*(1 + ACCEL_F + abs(p2_vy*p2_vy)*ACCEL_F);
      if(abs(r) <= V_BALL_MAX )
        ball_vx = r;
    }
    //if(is_p2_goal)
    if(ball_x < P1_GOAL_BORDER){
      reset_ball(ball_x, ball_y, ball_vx, ball_vy, true);
      p2_score++;
      p2_score_changed = true;
      if(p2_score > MAX_SCORE){
        p1_score_changed = true;
        p1_score = 0;
        p2_score = 0;
      }
      beep();
    }
    //if(is_p1_goal)
    if(ball_x+BALL_DIAMETER > P2_GOAL_BORDER){
      reset_ball(ball_x, ball_y, ball_vx, ball_vy, false);
      p1_score++;
      p1_score_changed = true;
      if(p1_score > MAX_SCORE){
        p2_score_changed = true;
        p1_score = 0;
        p2_score = 0;
      }
      beep();
    }
    
    //Print ball
    draw_ball(ball_last_x, ball_last_y, BALL_DIAMETER, BACKGROUND);
    draw_ball((uint8_t) ball_x, (uint8_t) ball_y, BALL_DIAMETER, FOREGROUND);
 
    //Print players
    uint8_t delta_y_p1 = p1_y-p1_last_y;
    draw_player(p1_y > p1_last_y ? p1_last_y : p1_last_y + BAR_HEIGHT-1, 
                P1_X, delta_y_p1, BACKGROUND);
    draw_player(p1_y, P1_X, BAR_HEIGHT, FOREGROUND);

    uint8_t delta_y_p2 = p2_y-p2_last_y;
    draw_player(p2_y > p2_last_y ? p2_last_y : p2_last_y + BAR_HEIGHT-1, 
                P2_X, delta_y_p2, BACKGROUND);
    draw_player(p2_y, P2_X, BAR_HEIGHT, FOREGROUND);
     
    //Print table and scores
    if(p1_score_changed || 
       isOver(ball_last_x, ball_last_y, BALL_DIAMETER, BALL_DIAMETER,
              SCORE_P1_X, SCORE_Y, SCORE_WIDTH, SCORE_HEIGHT)){
      p1_score_changed = false;
      draw_score(p1_score, SCORE_P1_X);
    }
    if(p2_score_changed || 
       isOver(ball_last_x, ball_last_y, BALL_DIAMETER, BALL_DIAMETER,
              SCORE_P2_X, SCORE_Y, SCORE_WIDTH, SCORE_HEIGHT)){
      p2_score_changed = false;
      draw_score(p2_score, SCORE_P2_X);
    }
    if(ball_last_x <= H_CENTER+THICK/2+THICK%2-1 &&
       ball_last_x + BALL_DIAMETER >= H_CENTER-THICK/2)
      draw_table(THICK, false);
    
    delay32Ms(1, REFRESH_RATE);
  } 
  return 0;
}