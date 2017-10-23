#include "mcu_regs.h"
#include "type.h"
#include "ssp.h"
#include "i2c.h"
#include "pca9532.h"
#include "gpio.h"
#include "cmsis_os.h"
#include "uart.h"
#include "oled.h"
#include "ctype.h"
#include "stdio.h"
//#include "timer32.h"

//Configurações diversas
#define FPS_24
//#define WRITE_GANTT

//************************
// Definições e tipos do sistema
//************************
//Definições de sistema
typedef enum {false = 0, true} bool;

#ifdef FPS_24
#define REFRESH_RATE 40
#else
#define REFRESH_RATE 80
#endif

#define T_WAKE    0x01
#define T_P1_WAKE 0x01
#define T_P2_WAKE 0x02

#define MAIL_SIZE 5
#define GANTT_MAIL_SIZE 16

#define gantt_ticks_factor 72

//************************
// Variaveis de sistema
//************************
FILE* file_gantt;

//************************
// Definições e tipos do domínio
//************************
//Definições do Jogo
#define MAX_SCORE (8)
#define PLAYER1 (1)
#define PLAYER2 (2)

//Definições de comando
#define ESC 0x1B
#define SPC ' '
#define BSP 0x7F
//#define BEL 0x07
//#define ENT 0x0D

#define P1_U 'w'
#define P1_D 's'
#define P2_U 'i'
#define P2_D 'k'

//Definições Físicas
#define V_BAR_MAX       (REFRESH_RATE/10.0)
#define V_BALL_MAX      (REFRESH_RATE/10.0)
#define BALL_VOX        (REFRESH_RATE/4.0) //80
#define BALL_VOY        (REFRESH_RATE/4.0) //80
#define ACCEL_BAR       (REFRESH_RATE/40.0)
#define ACCEL_F         (REFRESH_RATE/2000.0)

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
#define SCORE_SPACING_X (8)
#define SCORE_SPACING_Y (4)
#define SCORE_P1_X (H_CENTER-SCORE_WIDTH/2-SCORE_SPACING_X-SCORE_WIDTH/2)
#define SCORE_P2_X (H_CENTER-SCORE_WIDTH/2+SCORE_SPACING_X+SCORE_WIDTH/2)
#define SCORE_Y (TABLE_UPPER+SCORE_SPACING_Y+SCORE_HEIGHT/2)

#define BALL_RADIUS (1)
#define BALL_DIAMETER (BALL_RADIUS*2)
#define BALL_CENTER (BALL_DIAMETER / 2)

//Inline functions
#define reset_ball(px, py, vx, vy, t) \
        px = H_CENTER+(BALL_CENTER-1);\
        py = V_CENTER+(BALL_CENTER-1);\
        vx = t?BALL_VOX:-BALL_VOX;\
        vy = BALL_VOY;\
          
#define isOver(x1, y1, w1, h1, x2, y2, w2, h2) \
      ( x1 + w1 >= x2      && \
        x1      <= x2 + w2 && \
        y1 + h1 >= y2      && \
        y1      <= y2 + h2 )

#define clamp(min, max, v) \
          ((v) < (min)) ? (min) :\
          ((v) > (max)) ? (max) : (v)
            
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

typedef struct {
  const char* name;
  uint32_t init, end;
} gantt_msg_t;

typedef enum game_state{
  START, PAUSE, PLAY, RESTART, EXIT
}game_state_t;

game_state_t global_state = START;

//************************
//IDs de Timers
osTimerId id_timer_game_loop;

//************************
// Definiçõa MUTEX
//************************
osMutexId stdio_mutex;
osMutexDef(stdio_mutex);

//************************
//IDs de correspondencias
osMailQId id_mail_p1_input;
osMailQId id_mail_p2_input;
osMailQId id_mail_p1_pos;
osMailQId id_mail_p2_pos;
osMailQId id_mail_score;
osMailQId id_mail_drawer;
osMailQId id_mail_gantt;

//************************
//IDs de Threads
osThreadId id_thread_input_receiver;
osThreadId id_thread_player1;
osThreadId id_thread_player2;
osThreadId id_thread_manager;
osThreadId id_thread_score;
osThreadId id_thread_drawer;
osThreadId id_thread_gantt;

const char * name_thread_input_receiver = "Input Receiver";
const char * name_thread_player1        = "Player 1      ";
const char * name_thread_player2        = "Player 2      ";
const char * name_thread_manager        = "Manager       ";
const char * name_thread_score          = "Score         ";
const char * name_thread_drawer         = "Drawer        ";

//************************
//Funções auxiliares
//************************
void clear_scene(){
    oled_clearScreen(BACKGROUND);
}

void beep(){
  osMutexWait(stdio_mutex, osWaitForever);
  UARTSendString((uint8_t*)"\a");
  osMutexRelease(stdio_mutex);
}

void show_menu(){
  uint8_t clear_screan = 0x0C;
  UARTSend(&clear_screan,1);
  UARTSendString((uint8_t*)"Welcome The Pong game\r\n\n");

  UARTSendString((uint8_t*)"W/S- Controll Player 1\r\n");
  UARTSendString((uint8_t*)"I/K- Controll Player 2\r\n\n");

  UARTSendString((uint8_t*)"SPC- Start/Pause\r\n");
  UARTSendString((uint8_t*)"ESC- Exit\r\n");
  UARTSendString((uint8_t*)"BSP- Restart\r\n\n");
  
  UARTSendString((uint8_t*)"Player1 \t Player2 \r\n\n");    
}
void show_score(uint8_t score_p1 , uint8_t score_p2){
  
  
  static uint8_t up[] = {0x1B, 0x5B, 0x41};
  uint8_t s1 = score_p1+'0';
  uint8_t s2 = score_p2+'0';
  osMutexWait(stdio_mutex, osWaitForever);
  UARTSend(up, 3);
  UARTSendString((uint8_t*)"\t"); 
  UARTSend(&s1, 1);
  UARTSendString((uint8_t*)"\t"); 
  UARTSend(&s2, 1);
  UARTSendString((uint8_t*)"\r\n");
  osMutexRelease(stdio_mutex);
}

void show_winner(uint8_t winner){
  osMutexWait(stdio_mutex, osWaitForever);
  if(winner == PLAYER1)
    UARTSendString((uint8_t*)" Player 1 "); 
  if(winner == PLAYER2)
    UARTSendString((uint8_t*)" Player 2 "); 
  if(winner == PLAYER1 || winner == PLAYER2)
    UARTSendString((uint8_t*)"is the WINNER!!\r");
   osMutexRelease(stdio_mutex);
}

void clear_winner_msg(){
    UARTSendString((uint8_t*)"                         \r"); 
}

void write_gantt(const char* name, uint32_t init, uint32_t end){
#ifdef WRITE_GANTT
  gantt_msg_t *gantt = (gantt_msg_t*) osMailAlloc(id_mail_gantt, osWaitForever);
  gantt->name = name;
  gantt->init = init;
  gantt->end = end;
  osMailPut(id_mail_gantt, gantt);  
#endif
}

void draw_table(uint8_t thickness, bool draw_sides){
  if(thickness > 0){
    if(draw_sides){
      oled_rect(TABLE_START,TABLE_BOTTOM,  TABLE_END, SCREEN_DOWN,  FOREGROUND);
      oled_rect(TABLE_START,TABLE_UPPER, TABLE_END, SCREEN_UP, FOREGROUND);
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

osMailQDef(mail_gantt,    GANTT_MAIL_SIZE, gantt_msg_t);

//************************
//Threads
//************************
void thread_input_receiver(void const *args){
  uint32_t time;
  bool r_flag = false;
  bool x_flag = false;
  while(global_state != EXIT){
    osEvent evt = osSignalWait (T_WAKE, osWaitForever);     
    if(evt.status == osEventSignal){
      time = osKernelSysTick()/gantt_ticks_factor;
      osThreadYield();
      uint8_t rec = 0;
      osMutexWait(stdio_mutex, osWaitForever);
      UARTReceive(&rec, 1, 0);
      osMutexRelease(stdio_mutex);
      
      if(global_state != START){
        if(x_flag == true && (!rec || rec == ESC))
          global_state = EXIT;
        else
          x_flag = false;
          
        if(rec == ESC)
          x_flag = true;
        
        if(rec == SPC){
          if(global_state == PAUSE){
            global_state = PLAY;  
            clear_winner_msg();
          }
          else
            global_state = PAUSE;  
        }
        
        if(rec == BSP)
          global_state = RESTART;
      }
      else {
        show_menu();
      }
      
      if(global_state == RESTART){
        if(r_flag){
          global_state = PAUSE;
          r_flag = false;
        }
        else
          r_flag = true;
      }
      
      if(global_state != PAUSE){
        int8_t p1_in = 0;
        int8_t p2_in = 0;
        if(rec == P1_U)
          p1_in--;
        if(rec == P1_D)
          p1_in++;
        if(rec == P2_U)
          p2_in--;
        if(rec == P2_D)
          p2_in++;

        int8_t *p1_input = (int8_t*) osMailAlloc(id_mail_p1_input, osWaitForever);
        int8_t *p2_input = (int8_t*) osMailAlloc(id_mail_p2_input, osWaitForever);
        *p1_input = p1_in;
        *p2_input = p2_in;
        osMailPut(id_mail_p1_input, p1_input);
        osMailPut(id_mail_p2_input, p2_input);
      }      
      write_gantt(name_thread_input_receiver, time, osKernelSysTick()/gantt_ticks_factor);
    }
  }
  osDelay(osWaitForever);
}
osThreadDef(thread_input_receiver, osPriorityAboveNormal, 1, 0);

void thread_player(void const *args){
  uint32_t time;
  osMailQId id_mail_input = NULL;
  osMailQId id_mail_output = NULL;
  const char * name;
  int signal = 0;
  kinematic_t knt;
  knt.sy = V_CENTER-BAR_CENTER;
  knt.dx = 0;
  knt.dy = 0;

  switch((int) args){
  case PLAYER1:
    id_mail_input  = id_mail_p1_input;
    id_mail_output = id_mail_p1_pos;
    name = name_thread_player1;
    signal = T_P1_WAKE;
    knt.sx = P1_X;
    break;

  case PLAYER2:
    id_mail_input  = id_mail_p2_input;
    id_mail_output = id_mail_p2_pos;
    name = name_thread_player2;
    signal = T_P2_WAKE;
    knt.sx = P2_X;
    break;

  default:
    osDelay(osWaitForever);
  }
   
  while(global_state != EXIT){
    osEvent evt = osMailGet(id_mail_input, osWaitForever);
    if (evt.status == osEventMail) {
      time = osKernelSysTick()/gantt_ticks_factor;
      osThreadYield();
      int8_t* input = (int8_t*) evt.value.p;
      int8_t input_accel = *input;
      osMailFree(id_mail_input, input);
      
      knt.dy   += input_accel*ACCEL_BAR;
      knt.dy    = (int8_t) clamp(-V_BAR_MAX, V_BAR_MAX, knt.dy);

      knt.sx_p  = knt.sx;
      knt.sy_p  = knt.sy;

      knt.sy   += knt.dy;
      knt.sy    = (uint8_t) clamp(TABLE_UPPER+1, TABLE_BOTTOM-BAR_HEIGHT, knt.sy);
      if(knt.sy_p == knt.sy) 
        knt.dy = 0;

      if(global_state == RESTART){
        if((int) args == PLAYER1) knt.sx = P1_X;
        if((int) args == PLAYER2) knt.sx = P2_X;
        knt.sy = V_CENTER-BAR_CENTER;
        knt.dx = 0;
        knt.dy = 0;
      }
      
      kinematic_t *out = 
        (kinematic_t*) osMailAlloc(id_mail_output, osWaitForever);
      *out = knt;
      osMailPut(id_mail_output, out);
      
      osSignalSet(id_thread_manager, signal);
      
      write_gantt(name, time, osKernelSysTick()/gantt_ticks_factor);
    }
  }
  osDelay(osWaitForever);
}
osThreadDef(thread_player, osPriorityNormal, 1, 0);

void thread_manager(void const *args){
  uint32_t time;
  game_obj_t objs;
  objs.score.p1= 0;
  objs.score.p2 = 0;
  objs.score.p1_changed = true;
  objs.score.p2_changed = true;
  
  float ball_x, ball_y, ball_vx, ball_vy;
  
  reset_ball(ball_x, ball_y, ball_vx, ball_vy, true);  
  while(global_state != EXIT){
    osEvent evt = osSignalWait (T_P1_WAKE & T_P2_WAKE, osWaitForever);     
    if(evt.status == osEventSignal){
      evt = osMailGet(id_mail_p1_pos, osWaitForever);
      if(evt.status == osEventMail){
        objs.p1 = *((kinematic_t*) evt.value.p);
        osMailFree(id_mail_p1_pos, evt.value.p);
      }
      evt = osMailGet(id_mail_p2_pos, osWaitForever);
      if(evt.status == osEventMail){
        objs.p2 = *((kinematic_t*) evt.value.p);
        osMailFree(id_mail_p2_pos, evt.value.p);
      }
      time = osKernelSysTick()/gantt_ticks_factor;
      osThreadYield();
     
      if(global_state == RESTART){
        reset_ball(ball_x, ball_y, ball_vx, ball_vy, true);  
        objs.score.p1 = 0;
        objs.score.p2 = 0;
        objs.score.p1_changed = true;
        objs.score.p2_changed = true;
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
        ball_vy = (ball_y + BALL_CENTER - (objs.p1.sy + BAR_CENTER))/BAR_CENTER;
        ball_vx = ball_vx*(1 + ACCEL_F + abs(objs.p1.dy*objs.p1.dy)*ACCEL_F);
        ball_vx = clamp(-V_BALL_MAX, V_BALL_MAX, ball_vx);
        ball_vy = clamp(-1, 1, ball_vy);
        ball_vy = ball_vx*ball_vy;
       }
      //if(has_p2_collided_ball)
      if(ball_x + BALL_DIAMETER > TABLE_END - THICK + 1&&
            objs.p2.sy <= ball_y + BALL_DIAMETER &&
            objs.p2.sy + BAR_HEIGHT >= ball_y){
        ball_x = (float) TABLE_END - (BALL_DIAMETER + THICK - 1);
        ball_vx = -ball_vx;
        ball_vy = (ball_y + BALL_CENTER - (objs.p2.sy + BAR_CENTER))/BAR_CENTER;
        ball_vx = clamp(-V_BALL_MAX, V_BALL_MAX, ball_vx);
        ball_vy = clamp(-1, 1, ball_vy);
        ball_vy = -ball_vx*ball_vy;
      }
      //if(is_p2_goal)
      if(ball_x < P1_GOAL_BORDER){
        reset_ball(ball_x, ball_y, ball_vx, ball_vy, true);
        objs.score.p2++;
        objs.score.p2_changed = true;
        if(objs.score.p2 > MAX_SCORE){
          beep();
          show_winner(PLAYER2);
          global_state = RESTART;
        }
      }
      //if(is_p1_goal)
      if(ball_x+BALL_DIAMETER > P2_GOAL_BORDER){
        reset_ball(ball_x, ball_y, ball_vx, ball_vy, false);
        objs.score.p1++;
        objs.score.p1_changed = true;
        if(objs.score.p1> MAX_SCORE){
          beep();
          show_winner(PLAYER1);
          global_state = RESTART;
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
        score_t *score_msg = (score_t*) osMailAlloc(id_mail_score, osWaitForever);
        *score_msg = objs.score;
        osMailPut(id_mail_score, score_msg);
        objs.score.p1_changed = false;
        objs.score.p2_changed = false;
      }
      
      write_gantt(name_thread_manager, time, osKernelSysTick()/gantt_ticks_factor);
    }
  }
  osDelay(osWaitForever);
}
osThreadDef(thread_manager, osPriorityNormal, 1, 0);

void thread_score(void const *args){
  uint32_t time;
  uint16_t last_mask_red = 0;
  uint16_t last_mask_green= 0;
  uint8_t score_p1;
  uint8_t score_p2;
  while(global_state != EXIT){
    osEvent evt = osMailGet(id_mail_score, osWaitForever);
    if (evt.status == osEventMail){
      time = osKernelSysTick()/gantt_ticks_factor;
      osThreadYield();
      score_t * input = (score_t*) evt.value.p;
      if(score_p1 < input->p1 || score_p2 < input->p2)
        beep();
      score_p1 =  input->p1;
      score_p2 =  input->p2;
      bool p1_changed = input->p1_changed;
      bool p2_changed = input->p2_changed;
      osMailFree(id_mail_score, input);
      uint16_t mask_red = 0;
      uint16_t mask_green = 0;
      if(p1_changed || p2_changed){
        show_score(score_p1, score_p2);
        if(p1_changed){
          //Calcula mascara de escrita 
          //Os indices dos 8 ultimos bits em 1 representam os leds verdes que serao acesos, em ordem inversa
          //LED  green
          for(uint8_t i = 0; i < score_p1; i++) mask_green |= 0x8000 >> i;
          //Apaga ultimos LEDs acesos e acende os atuais
          pca9532_setLeds(mask_green, last_mask_green); 
          //LEDs que deverao ser apagados na proxima execucao foram acesos agora
          last_mask_green = mask_green;
        }
        if(p2_changed){
          //Calcula mascara de escrita 
          //Os indices dos 8 primeiros bits em 1 representam os leds vermelhos que serao acesos, respectivamente
          //LED red
          for(uint8_t i = 0; i < score_p2; i++) mask_red |= 1 << i;
          //Apaga ultimos LEDs acesos e acende os atuais
          pca9532_setLeds(mask_red, last_mask_red);
          //LEDs que deverao ser apagados na proxima execucao foram acesos agora
          last_mask_red = mask_red;      
        }
      }
      write_gantt(name_thread_score, time, osKernelSysTick()/gantt_ticks_factor);
    }
  }
}
osThreadDef(thread_score, osPriorityNormal, 1, 0);

void thread_drawer(void const *args){
  uint32_t time;
  game_obj_t objs;
  osEvent evt = osMailGet(id_mail_drawer, osWaitForever);
  while(global_state != EXIT){
    time = osKernelSysTick()/gantt_ticks_factor;
    while(evt.status == osEventMail){
      game_obj_t* msg = (game_obj_t*) evt.value.p;
      objs = *msg;
      osMailFree(id_mail_drawer, msg);
      evt = osMailGet(id_mail_drawer, 1);
    }
    if (evt.status == osEventTimeout) {
      osThreadYield();
      //Draw table
      if(global_state == START){
        clear_scene();
        draw_table(THICK, true);
        global_state = PAUSE;
      }
      
      //Draw ball
      draw_ball(objs.ball.sx_p, objs.ball.sy_p, BALL_DIAMETER, BACKGROUND);
      draw_ball(objs.ball.sx,   objs.ball.sy,   BALL_DIAMETER, FOREGROUND);
   
      //Draw players
      int8_t delta_y_p1 = objs.p1.sy-objs.p1.sy_p;
      draw_player(objs.p1.sy > objs.p1.sy_p ? objs.p1.sy_p : objs.p1.sy_p + BAR_HEIGHT-1, 
                  objs.p1.sx, delta_y_p1, BACKGROUND);
      draw_player(objs.p1.sy, objs.p1.sx, BAR_HEIGHT, FOREGROUND);

      int8_t delta_y_p2 = objs.p2.sy-objs.p2.sy_p;
      draw_player(objs.p2.sy > objs.p2.sy_p ? objs.p2.sy_p: objs.p2.sy_p + BAR_HEIGHT-1, 
                  objs.p2.sx, delta_y_p2, BACKGROUND);
      draw_player(objs.p2.sy, objs.p2.sx, BAR_HEIGHT, FOREGROUND);

      //Draw borders and scores
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

      write_gantt(name_thread_drawer, time, osKernelSysTick()/gantt_ticks_factor);

      evt = osMailGet(id_mail_drawer, osWaitForever);
    }
  }
  clear_scene();
  osDelay(osWaitForever);
}
osThreadDef(thread_drawer, osPriorityNormal, 1, 0);

void thread_gantt(){
  //************************
  //Init Gantt Diagram
  //*********************** 
  file_gantt = fopen("gantt.txt","w");
  fprintf(file_gantt,"gantt\n");
  fprintf(file_gantt,"    title A Gantt Diagram\n");
  fprintf(file_gantt,"    dateFormat x\n");

  gantt_msg_t gantt;
  osEvent evt ;

  do{
    evt = osMailGet(id_mail_gantt, 500);
    if (evt.status == osEventMail) {
      gantt_msg_t* msg = (gantt_msg_t*) evt.value.p;
      gantt = *msg;
      osMailFree(id_mail_gantt, msg);      
      
      fputs(gantt.name, file_gantt);
      fprintf(file_gantt,": a, %lu, %lu\n", gantt.init, gantt.end);
    }
  }while(global_state != EXIT || evt.status != osEventTimeout);

  fclose(file_gantt);
}

//************************
//Codigo Main
//************************
  int main(char** args, int n_args){
//Inicialização de Kernel vai aqui
  osKernelInitialize();  
  
  GPIOInit();
  SSPInit();
  UARTInit(115200);
  oled_init();
  I2CInit( (uint32_t)I2CMASTER, 0 );
  pca9532_init();

  //************************
  //Inicialização de Mutex
  //************************  
  stdio_mutex = osMutexCreate(osMutex(stdio_mutex));
  
  //************************
  //Inicialização de Timers
  //************************
  id_timer_game_loop =  osTimerCreate(osTimer(timer_game_loop),   osTimerPeriodic, NULL);

  osTimerStart(id_timer_game_loop, REFRESH_RATE);  

  //************************
  //Inicialização de Threads
  //************************
  id_thread_input_receiver = osThreadCreate(osThread(thread_input_receiver), NULL);
  id_thread_player1        = osThreadCreate(osThread(thread_player),         (void*) PLAYER1);
  id_thread_player2        = osThreadCreate(osThread(thread_player),         (void*) PLAYER2);
  id_thread_manager        = osThreadCreate(osThread(thread_manager),        NULL);
  id_thread_score          = osThreadCreate(osThread(thread_score),          NULL);
  id_thread_drawer         = osThreadCreate(osThread(thread_drawer),         NULL);

  id_thread_gantt          = osThreadGetId();

  //************************
  //Inicialização de Correspondências
  //************************
  id_mail_p1_input = osMailCreate(osMailQ(mail_p1_input), NULL);
  id_mail_p2_input = osMailCreate(osMailQ(mail_p2_input), NULL);
  id_mail_p1_pos   = osMailCreate(osMailQ(mail_p1_pos),   NULL);
  id_mail_p2_pos   = osMailCreate(osMailQ(mail_p2_pos),   NULL);
  id_mail_score    = osMailCreate(osMailQ(mail_score),    NULL);
  id_mail_drawer   = osMailCreate(osMailQ(mail_drawer),   NULL);

  id_mail_gantt   = osMailCreate(osMailQ(mail_gantt),     NULL);
   
  //************************
  //Início do SO
  //************************
  osKernelStart();
  
  //************************
  //Thread Main
  //************************
  osThreadSetPriority(id_thread_gantt, osPriorityLow);
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
}
