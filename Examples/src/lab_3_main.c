#include "mcu_regs.h"
#include "type.h"
#include "ssp.h"
#include "gpio.h"
#include "cmsis_os.h"
#include "uart.h"
#include "oled.h"
#include "ctype.h"
#include "stdio.h"
//#include "timer32.h"

//Configurações diversas
#define FPS_24

//************************
// Definições e tipos do sistema
//************************
//Definições de sistema

#define true 1
#define false 0
typedef uint8_t bool;

#ifdef FPS_24
#define REFRESH_RATE 40
#else
#define REFRESH_RATE 80
#endif

#define T_WAKE 0x01
#define T_P1_WAKE 0x01
#define T_P2_WAKE 0x02

#define MAIL_SIZE 4

//************************
// Definições e tipos do domínio
//************************
//Definições do Jogo
#define MAX_SCORE (8)
#define PLAYER1 (1)
#define PLAYER2 (2)

//Definições Físicas
#define V_BAR_MAX  (4)
#define V_BALL_MAX (8)
#define BALL_VOX (1)
#define BALL_VOY (1)
#define ACCEL_F (.02)

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

//************************
//Tipos
typedef struct {
  int8_t sx, sy, dx, dy;
  int8_t sx_p, sy_p;
} kinematic_t;

typedef struct {
  uint8_t p1;
  uint8_t p2;
  bool p1_changed;
  bool p2_changed;
} score_t;

typedef struct {
  kinematic_t p1;
  kinematic_t p2;
  kinematic_t ball;
  score_t score;
} game_obj_t;

//************************
//IDs de Timers
osTimerId id_timer_game_loop;    

//************************
//IDs de correspondencias
osMailQId id_mail_p1_input;
osMailQId id_mail_p2_input;
osMailQId id_mail_p1_pos;
osMailQId id_mail_p2_pos;
osMailQId id_mail_score;
osMailQId id_mail_drawer;

//************************
//IDs de Threads
osThreadId id_thread_input_receiver;
osThreadId id_thread_player1;
osThreadId id_thread_player2;
osThreadId id_thread_manager;
osThreadId id_thread_score;
osThreadId id_thread_drawer;

//************************
//Funções auxiliares
//************************
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

//************************
//Callback de Timers
//************************
//Acorda thread de amostragem apos periodo de amostragem
void timer_game_loop_cb(void const *arg) {
  osSignalSet(id_thread_input_receiver, T_WAKE);
}
osTimerDef(timer_game_loop, timer_game_loop_cb);

//************************
//Definições de Correspondencia
//************************
osMailQDef(mail_p1_input, MAIL_SIZE, int8_t);
osMailQDef(mail_p2_input, MAIL_SIZE, int8_t);
osMailQDef(mail_p1_pos,   MAIL_SIZE, kinematic_t);
osMailQDef(mail_p2_pos,   MAIL_SIZE, kinematic_t);
osMailQDef(mail_score,    MAIL_SIZE, score_t);
osMailQDef(mail_drawer,   MAIL_SIZE, game_obj_t);

//************************
//Threads
//************************
void thread_input_receiver(void const *args){
  uint32_t time;
  
  while(1){
    osEvent evt = osSignalWait (T_WAKE, osWaitForever);     
    if(evt.status == osEventSignal){
      uint8_t rec = 0;
      UARTReceive(&rec, 1, 0);

      int8_t p1_in = 0;
      int8_t p2_in = 0;
      if(rec == 'w')
        p1_in--;
      if(rec == 's')
        p1_in++;
      if(rec == 'i')
        p2_in--;
      if(rec == 'k')
        p2_in++;
      
      int8_t *p1_input = (int8_t*) osMailAlloc(id_mail_p1_input, osWaitForever);
      int8_t *p2_input = (int8_t*) osMailAlloc(id_mail_p2_input, osWaitForever);
      *p1_input = p1_in;
      *p2_input = p2_in;
      osMailPut(id_mail_p1_input, p1_input);
      osMailPut(id_mail_p2_input, p2_input);
    }
  }
}
osThreadDef(thread_input_receiver, osPriorityNormal, 1, 0);

void thread_player(void const *args){
  osMailQId id_mail_input = NULL;
  osMailQId id_mail_output = NULL;
  int signal = 0;
  kinematic_t knt;
  knt.sy = V_CENTER-BAR_CENTER;
  knt.dx = 0;
  knt.dy = 0;

  switch((int) args){
  case PLAYER1:
    id_mail_input  = id_mail_p1_input;
    id_mail_output = id_mail_p1_pos;
    signal = T_P1_WAKE;
    knt.sx = P1_X;
    break;

  case PLAYER2:
    id_mail_input  = id_mail_p2_input;
    id_mail_output = id_mail_p2_pos;
    signal = T_P2_WAKE;
    knt.sx = P2_X;
    break;

  default:
    osDelay(osWaitForever);
  }
   
  while(1){
    osEvent evt = osMailGet(id_mail_input, osWaitForever);
    if (evt.status == osEventMail) {
      int8_t* input = (int8_t*) evt.value.p;
      int8_t input_accel = *input;
      osMailFree(id_mail_input, input);
      
      knt.dy   += input_accel;
      knt.dy    = (int8_t) clamp(-V_BAR_MAX, V_BAR_MAX, knt.dy);

      knt.sx_p  = knt.sx;
      knt.sy_p  = knt.sy;

      knt.sy   += knt.dy;
      knt.sy    = (uint8_t) clamp(TABLE_UPPER+1, TABLE_BOTTOM-BAR_HEIGHT, knt.sy);
      if(knt.sy_p == knt.sy) 
        knt.dy = 0;

      kinematic_t *out = 
        (kinematic_t*) osMailAlloc(id_mail_output, osWaitForever);
      *out = knt;
      osMailPut(id_mail_output, out);
      
      osSignalSet(id_thread_manager, signal);
    }
  }
}
osThreadDef(thread_player, osPriorityNormal, 1, 0);

void thread_manager(void const *args){
  game_obj_t objs;
  objs.score.p1= 0;
  objs.score.p2 = 0;
  objs.score.p1_changed = true;
  objs.score.p2_changed = true;
  
  float ball_x, ball_y, ball_vx, ball_vy;
  
  reset_ball(ball_x, ball_y, ball_vx, ball_vy, false);  
  while(1){
    osEvent evt = osSignalWait (T_P1_WAKE & T_P2_WAKE, osWaitForever);     
    if(evt.status == osEventSignal){
      evt = osMailGet(id_mail_p1_pos, 10);
      if(evt.status == osEventMail){
        objs.p1 = *((kinematic_t*) evt.value.p);
        osMailFree(id_mail_p1_pos, evt.value.p);
      }
      evt = osMailGet(id_mail_p2_pos, 10);
      if(evt.status == osEventMail){
        objs.p2 = *((kinematic_t*) evt.value.p);
        osMailFree(id_mail_p2_pos, evt.value.p);
      }
      
      //Ball controll
      objs.ball.sx_p = objs.ball.sx;
      objs.ball.sy_p = objs.ball.sy;

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
            objs.p1.sy <= ball_y + BALL_DIAMETER &&
            objs.p1.sy + BAR_HEIGHT >= ball_y){
        ball_x = (float) TABLE_START + THICK;
        ball_vx = -ball_vx;
        ball_vy = (ball_vy + BALL_CENTER - (objs.p1.dy + BAR_CENTER))/BAR_CENTER;
        float r = ball_vx*(1 + ACCEL_F + abs(objs.p1.dy*objs.p1.dy)*ACCEL_F);
        if(r*r <= V_BALL_MAX*V_BALL_MAX)
          ball_vx = r;
       }
      //if(has_p2_collided_ball)
      if(ball_x + BALL_DIAMETER > TABLE_END - THICK + 1&&
            objs.p2.sy <= ball_y + BALL_DIAMETER &&
            objs.p2.sy + BAR_HEIGHT >= ball_y){
        ball_x = (float) TABLE_END - (BALL_DIAMETER + THICK - 1);
        ball_vx = -ball_vx;
        ball_vy = (ball_y + BALL_CENTER - (objs.p2.sy + BAR_CENTER))/BAR_CENTER;
        float r = ball_vx*(1 + ACCEL_F + abs(objs.p2.dy*objs.p2.dy)*ACCEL_F);
        if(r*r <= V_BALL_MAX*V_BALL_MAX)
          ball_vx = r;
      }
      //if(is_p2_goal)
      if(ball_x < P1_GOAL_BORDER){
        reset_ball(ball_x, ball_y, ball_vx, ball_vy, true);
        objs.score.p2++;
        objs.score.p2_changed = true;
        if(objs.score.p2 > MAX_SCORE){
          objs.score.p1_changed = true;
          objs.score.p1 = 0;
          objs.score.p2 = 0;
        }
      }
      //if(is_p1_goal)
      if(ball_x+BALL_DIAMETER > P2_GOAL_BORDER){
        reset_ball(ball_x, ball_y, ball_vx, ball_vy, false);
        objs.score.p1++;
        objs.score.p1_changed = true;
        if(objs.score.p1> MAX_SCORE){
          objs.score.p2_changed = true;
          objs.score.p1= 0;
          objs.score.p2 = 0;
        }
      }
      
      objs.ball.sx = (uint8_t) ball_x;
      objs.ball.sy = (uint8_t) ball_y;
      objs.ball.dx = (int8_t)ball_vx;
      objs.ball.dy = (int8_t)ball_vy;
            
      game_obj_t *msg = (game_obj_t*) osMailAlloc(id_mail_drawer, osWaitForever);
      *msg = objs;
      osMailPut(id_mail_drawer, msg);

      if(objs.score.p1_changed || objs.score.p2_changed){
        objs.score.p1_changed = false;
        objs.score.p2_changed = false;
        //send 
      }
    }
  }
}
osThreadDef(thread_manager, osPriorityNormal, 1, 0);

void thread_score(void const *args){
  osDelay(osWaitForever);
}
osThreadDef(thread_score, osPriorityNormal, 1, 0);

void thread_drawer(void const *args){
  game_obj_t objs;
  draw_table(THICK, true);
   while(1){
    osEvent evt = osMailGet(id_mail_drawer, osWaitForever);
    if (evt.status == osEventMail) {
      game_obj_t* msg = (game_obj_t*) evt.value.p;
      objs = *msg;
      osMailFree(id_mail_drawer, msg);
      
      //Print ball
      draw_ball(objs.ball.sx_p, objs.ball.sy_p, BALL_DIAMETER, BACKGROUND);
      draw_ball(objs.ball.sx,   objs.ball.sy,   BALL_DIAMETER, FOREGROUND);
   
      //Print players
//      uint8_t delta_y_p1 = p1_y-p1_last_y;
      draw_player(objs.p1.sy > objs.p1.sy_p ? objs.p1.sy_p : objs.p1.sy_p + BAR_HEIGHT-1, 
                  objs.p1.sx, objs.p1.dy, BACKGROUND);
      draw_player(objs.p1.sy, objs.p1.sx, BAR_HEIGHT, FOREGROUND);

//      uint8_t delta_y_p2 = p2_y-p2_last_y;
      draw_player(objs.p2.sy > objs.p2.sy_p ? objs.p2.sy_p: objs.p2.sy_p + BAR_HEIGHT-1, 
                  objs.p2.sx, objs.p2.dy, BACKGROUND);
      draw_player(objs.p2.sy, objs.p2.sx, BAR_HEIGHT, FOREGROUND);

      //Print table and scores
      if(objs.score.p1_changed || 
         isOver(objs.ball.sx_p, objs.ball.sy_p, BALL_DIAMETER, BALL_DIAMETER,
                SCORE_P1_X, SCORE_Y, SCORE_WIDTH, SCORE_HEIGHT)){
        draw_score(objs.score.p1, SCORE_P1_X);
      }
      if(objs.score.p2_changed || 
         isOver(objs.ball.sx_p, objs.ball.sy_p, BALL_DIAMETER, BALL_DIAMETER,
                SCORE_P2_X, SCORE_Y, SCORE_WIDTH, SCORE_HEIGHT)){
        draw_score(objs.score.p2, SCORE_P2_X);
      }
      if(objs.ball.sx_p <= H_CENTER+THICK/2+THICK%2-1 &&
         objs.ball.sx_p + BALL_DIAMETER >= H_CENTER-THICK/2)
        draw_table(THICK, false);
    }
  }
}
osThreadDef(thread_drawer, osPriorityNormal, 1, 0);

void thread_gantt(){
  osDelay(osWaitForever);
}

//************************
//Codigo Main
//************************
int main(char** args, int n_args){
//Inicialização de Kernel vai aqui
  osKernelInitialize();  

  //************************
  //Init Gantt Diagram
  //*********************** 
  FILE* file_gantt = fopen("gantt.txt","w");
  fprintf(file_gantt,"gantt\n");
  fprintf(file_gantt,"    title A Gantt Diagram\n");
  fprintf(file_gantt,"    dateFormat x\n");
  fclose(file_gantt);  
  
  GPIOInit();
  SSPInit();
  UARTInit(115200);
  oled_init();
//  init_timer32(1, 10);
 
  clear_scene();
  
  //************************
  //Inicialização de Timers
  //************************
  id_timer_game_loop =  osTimerCreate(osTimer(timer_game_loop),   osTimerPeriodic, NULL);

  osTimerStart(id_timer_game_loop, REFRESH_RATE);  

  //************************
  //Inicialização de Correspondências
  //************************
  id_mail_p1_input = osMailCreate(osMailQ(mail_p1_input), NULL);
  id_mail_p2_input = osMailCreate(osMailQ(mail_p2_input), NULL);
  id_mail_p1_pos   = osMailCreate(osMailQ(mail_p1_pos),   NULL);
  id_mail_p2_pos   = osMailCreate(osMailQ(mail_p2_pos),   NULL);
  id_mail_score    = osMailCreate(osMailQ(mail_score),    NULL);
  id_mail_drawer   = osMailCreate(osMailQ(mail_drawer),   NULL);
  
  //************************
  //Inicialização de Threads
  //************************
  id_thread_input_receiver = osThreadCreate(osThread(thread_input_receiver), NULL);
  id_thread_player1        = osThreadCreate(osThread(thread_player),         (void*) PLAYER1);
  id_thread_player2        = osThreadCreate(osThread(thread_player),         (void*) PLAYER2);
  id_thread_manager        = osThreadCreate(osThread(thread_manager),        NULL);
  id_thread_score          = osThreadCreate(osThread(thread_score),          NULL);
  id_thread_drawer         = osThreadCreate(osThread(thread_drawer),         NULL);
 
  //************************
  //Início do SO
  //************************
  osKernelStart();
  
  //************************
  //Thread Main
  //************************
  thread_gantt();

  //************************
  //Finalização de Threads
  //************************
  osThreadTerminate(id_thread_input_receiver);
  osThreadTerminate(id_thread_player1);
  osThreadTerminate(id_thread_player2);
  osThreadTerminate(id_thread_manager);
  osThreadTerminate(id_thread_score);
  osThreadTerminate(id_thread_drawer);
  
  osDelay(osWaitForever); 
  return 0;
  



  
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
    
//    delay32Ms(1, REFRESH_RATE);
  } 
  return 0;
}