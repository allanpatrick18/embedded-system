#include "mcu_regs.h"
#include "type.h"
#include "stdio.h"
#include "timer32.h"
#include "pca9532.h"
#include "cmsis_os.h"

//*************************
//*********************** *
//DEFINICOES DO DOMINIO * *
//*********************** *
//*************************
//tamanho da mensagem
#define MSG_SIZE 32     
#define TEST_1_INDEX MSG_SIZE-2
#define TEST_2_INDEX MSG_SIZE-1

//mensagem codificada
unsigned const char hashed_msg[MSG_SIZE] = { 
  0x67, 0x52, 0x89, 0x4a, 0x8b, 0x4e, 0x8a, 0x09, 
  0x86, 0x4f, 0x37, 0x3c, 0x80, 0x55, 0x80, 0x4c,
  0x86, 0x57, 0x37, 0x3f, 0x78, 0x55, 0x83, 0x4e,
  0x90, 0x09, 0x48, 0x22, 0x50, 0x22, 0x22, 0x04
};

typedef uint8_t bool;
#define false 0
#define true 1

typedef struct msg_pipe_elem{
  unsigned char key;
  unsigned char decyphered_msg[MSG_SIZE];
  bool isFirstTestValid;
  bool isSecondTestValid;
} msg_pipe_elem_t;

#define PIPE_SIZE 3
#define PIPE_STG_GENERATE 0
#define PIPE_STG_DECIPHER 1
#define PIPE_STG_VERIFY   2
#define msgInStage(stage) pipe[stage]
msg_pipe_elem_t pipe[PIPE_SIZE];

bool hasGeneratedKey = false;
bool hasDecypheredMsg = false;
bool hasVerifiedTest1 = false;
bool hasVerifiedTest2 = false;
bool hasValidKey = false;

#define PRIME_LIST_SIZE 54
#define EMPTY 0x00
#define FAILURE 0xFF
#define isEmpty(n) n > EMPTY
uint8_t prime_list[PRIME_LIST_SIZE] = {0};

uint8_t getPrime(size_t index){
  if(index > PRIME_LIST_SIZE)
    return FAILURE;
  
  uint8_t prime = prime_list[index];
  if(isEmpty(prime))
    return prime;
  
  if(index == 0){
    prime_list[index] = 2;
    return 2;
  }
  
  bool is_not_prime; 
  uint8_t last_prime = prime_list[index-1];
  uint8_t next_prime;
  for(next_prime = last_prime+1; 
      next_prime > last_prime;
      next_prime++){
      is_not_prime = false;
      for(uint8_t prev_prime_index = 0; 
          prev_prime_index < index;
          prev_prime_index++){
          uint8_t divisor = getPrime(prev_prime_index);
          if(next_prime%divisor == 0){
            is_not_prime = true;
            break;
          }
      }
      if(!is_not_prime) break;
  }
  
  if(is_not_prime){
    prime_list[index] = FAILURE;
    return FAILURE;
  }
  
  prime_list[index] = next_prime;
  return next_prime;
}

//*************************
//*********************** *
//DEFINICOES DO SISTEMA * *
//*********************** *
//*************************
#define yield osThreadYield()

//************************
//IDs de Threads
//************************
osThreadId id_thread_generate_key;
osThreadId id_thread_decipher_key;
osThreadId id_thread_verify_first_test_digit;
osThreadId id_thread_verify_second_test_digit;
osThreadId id_thread_write_key;
osThreadId id_thread_validate_key;

//Thread para geração de chaves
void thread_generate_key(void const *args){
  size_t p_index = 0;
  while(!hasValidKey){
    if(hasGeneratedKey){
      yield;
      continue;
    }
    msgInStage(PIPE_STG_GENERATE).key = getPrime(p_index++);
    hasGeneratedKey = true;
    yield;
  }
  osThreadTerminate(id_thread_generate_key);
}
osThreadDef(thread_generate_key, osPriorityNormal, 1, 0);

//Thread para decifrar chaves
void thread_decipher_key(void const *args){
  osThreadYield();
  osThreadTerminate(id_thread_decipher_key);
}
osThreadDef(thread_decipher_key, osPriorityNormal, 1, 0);

//Thread para testar penultimo digito verificador
void thread_verify_first_test_digit(void const *args){
  osThreadYield();
  osThreadTerminate(id_thread_verify_first_test_digit);
}
osThreadDef(thread_verify_first_test_digit, osPriorityNormal, 1, 0);

//Thread para testar ultimo digito verificador
void thread_verify_second_test_digit(void const *args){
  osThreadYield();
  osThreadTerminate(id_thread_verify_second_test_digit);
}
osThreadDef(thread_verify_second_test_digit, osPriorityNormal, 1, 0);

//Thread para escrever na saida chave gerada
void thread_write_key(void const *args){
  osThreadYield();
  osThreadTerminate(id_thread_write_key);
}
osThreadDef(thread_write_key, osPriorityNormal, 1, 0);

//Thread para testar ultimo digito verificador
void thread_validate_key(void const *args){
  osThreadYield();
  osThreadTerminate(id_thread_validate_key);
}
osThreadDef(thread_validate_key, osPriorityNormal, 1, 0);

//************************
//Código da thread Main
//************************
void thread_main(){
  while(!hasValidKey) yield;
}

int main(int n_args, char** args){
  //Inicialização de Kernel vai aqui
  osKernelInitialize();
  
  //************************
  //Inicialização de Threads aqui
  //************************
  id_thread_generate_key =             osThreadCreate(osThread(thread_generate_key),             NULL);
  id_thread_decipher_key =             osThreadCreate(osThread(thread_decipher_key),             NULL);
  id_thread_verify_first_test_digit =  osThreadCreate(osThread(thread_verify_first_test_digit),  NULL);
  id_thread_verify_second_test_digit = osThreadCreate(osThread(thread_verify_second_test_digit), NULL);
  id_thread_write_key =                osThreadCreate(osThread(thread_write_key),                NULL);
  id_thread_validate_key =             osThreadCreate(osThread(thread_validate_key),             NULL);
  
  //************************
  //Fim de inicializações de Threads
  //************************
  
  //Início do SO
  osKernelStart();
  
  //Main thread
  thread_main();
  return 0;
}