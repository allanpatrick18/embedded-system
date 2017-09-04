#include "mcu_regs.h"
#include "type.h"
#include "stdio.h"
#include "timer32.h"
#include "pca9532.h"
#include "cmsis_os.h"
#include "string.h"
#include "ctype.h"

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
  0x90, 0x09, 0x48, 0x22, 0x50, 0x22, 0x22, 0x04 //0x8b, 0xd6   
  //Quando penultimo e ultimo bytes são 0x8b e 0xd6 funciona
};

typedef uint8_t bool;
#define false 0
#define true 1

typedef struct msg_pipe_elem{
  unsigned char key;
  unsigned char prev_prime;
  bool hasKey;
  unsigned char deciphered_msg[MSG_SIZE];
  bool hasMsg;
  bool firstTestResult;
  bool hasFirstTest;
  bool secondTestResult;
  bool hasSecondTest;
} msg_pipe_elem_t;

#define PIPE_SIZE 3
#define PIPE_STG_GENERATED  0
#define PIPE_STG_DECIPHERED 1
#define PIPE_STG_VERIFIED   2
#define msgInStage(stage) pipe[stage]
msg_pipe_elem_t pipe[PIPE_SIZE] = {0};

bool hasGeneratedKey = false;
bool hasDecipheredMsg = false;
bool hasVerifiedTest1 = false;
bool hasVerifiedTest2 = false;
bool hasValidKey = false;

bool test1Loaded = false;
bool test2Loaded = false;

//Entre 0 e 255 existem 54 primos, sendo o ultimo 251
#define PRIME_LIST_SIZE 54
#define EMPTY 0x00
#define FAILURE 0xFF
#define isEmpty(n) n > EMPTY
uint8_t prime_list[PRIME_LIST_SIZE] = {0};
FILE *file;
int ticks_factor = 100;

//Utilidades diversas
//*************************
#define isOdd(n) n%2

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
osThreadId id_thread_print_key;
osThreadId id_thread_validate_key;

//************************
//Funções Auxiliares

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
          yield;
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


//************************
//Threads

//Thread para geração de chaves
void thread_generate_key(void const *args){
  size_t p_index = 0;
  while(!hasValidKey){
    if(hasGeneratedKey){
      yield;
      continue;
    }
     
    uint32_t time = osKernelSysTick()/ticks_factor;
    msg_pipe_elem_t *msg_st_ptr = &(msgInStage(PIPE_STG_GENERATED));
    uint8_t prev_prime = getPrime(p_index++);
    if(prev_prime == FAILURE) break;
    uint8_t key = getPrime(p_index);
    if(key == FAILURE) break;
    msg_st_ptr->prev_prime = prev_prime;
    msg_st_ptr->key = key;
    msg_st_ptr->hasKey = true;
    hasGeneratedKey = true;
    fprintf(file," thread_generate_key : %i, %i\n", (int)time, (int)osKernelSysTick()/ticks_factor);
    yield;
  }
  osThreadTerminate(id_thread_generate_key);
}
osThreadDef(thread_generate_key, osPriorityNormal, 1, 0);

//Thread para decifrar chaves
void thread_decipher_key(void const *args){
  bool processedMessage = false;
  msg_pipe_elem_t current_elem;
 
  while(!hasValidKey){
    if(!processedMessage){
      if(!hasGeneratedKey){
        yield;
        continue;
      }
      uint32_t time = osKernelSysTick()/ticks_factor;
      memcpy(&current_elem, &(msgInStage(PIPE_STG_GENERATED)), sizeof(msg_pipe_elem_t));
      hasGeneratedKey = false;
      //---
      uint8_t key =  current_elem.key;
      for(int i = 0; i < MSG_SIZE; i++){
        current_elem.deciphered_msg[i] = isOdd(i) ? hashed_msg[i] - key : hashed_msg[i] + key;
        if(i%8 == 7) yield;
      }
      //---
      current_elem.hasMsg = true;
      processedMessage = true;
      fprintf(file," thread_decipher_key : %i, %i\n", (int)time, (int)osKernelSysTick()/ticks_factor);
    } 
    if(processedMessage){
      if(hasDecipheredMsg){
        yield;
        continue;
      }
      memcpy(&(msgInStage(PIPE_STG_DECIPHERED)), &current_elem, sizeof(msg_pipe_elem_t));
      hasDecipheredMsg = true;
      processedMessage = false;
    }
     
    yield;
  }
  osThreadTerminate(id_thread_decipher_key);
}
osThreadDef(thread_decipher_key, osPriorityNormal, 1, 0);

//Thread para testar penultimo digito verificador
void thread_verify_first_test_digit(void const *args){
  bool verifiedByte = false;
  msg_pipe_elem_t current_elem;
  while(!hasValidKey){
    if(!verifiedByte){ 
      if(!hasDecipheredMsg){
        yield;
        continue;
      }
      uint32_t time = osKernelSysTick()/ticks_factor;
      memcpy(&current_elem, &(msgInStage(PIPE_STG_DECIPHERED)), sizeof(msg_pipe_elem_t));
      test1Loaded = true;
      if(test2Loaded){
        hasDecipheredMsg = false;
        test1Loaded = false;
        test2Loaded = false;
      }
      //---
      uint8_t testByte = current_elem.deciphered_msg[TEST_1_INDEX];
      uint8_t halfKey = current_elem.key>>1;
      current_elem.firstTestResult = halfKey == testByte;
      //---
      current_elem.hasFirstTest = true;
      verifiedByte = true;
       fprintf(file," thread_verify_first_test_digit : %i, %i\n", (int)time, (int)osKernelSysTick()/ticks_factor);
    } 
    if(verifiedByte){ 
      if(hasVerifiedTest1){
        yield;
        continue;
      }
      /** Perigo de concorrencia
      current_elem.secondTestResult = msgInStage(PIPE_STG_VERIFIED).secondTestResult;
      current_elem.hasSecondTest = msgInStage(PIPE_STG_VERIFIED).hasSecondTest;

      memcpy(&(msgInStage(PIPE_STG_VERIFIED)), &current_elem, MSG_SIZE);
      **/
      msg_pipe_elem_t* msg_st_ptr = &(msgInStage(PIPE_STG_VERIFIED));
      msg_st_ptr->key = current_elem.key;
      msg_st_ptr->prev_prime = current_elem.prev_prime;
      msg_st_ptr->hasKey = current_elem.hasKey;
      memcpy(msg_st_ptr->deciphered_msg, current_elem.deciphered_msg, MSG_SIZE);
      msg_st_ptr->hasMsg = current_elem.hasMsg;
      msg_st_ptr->firstTestResult = current_elem.firstTestResult;
      msg_st_ptr->hasFirstTest = current_elem.hasFirstTest;
      
      hasVerifiedTest1 = true;
      verifiedByte = false;
    }
     
    yield;
  }
  osThreadTerminate(id_thread_verify_first_test_digit);
}
osThreadDef(thread_verify_first_test_digit, osPriorityNormal, 1, 0);

//Thread para testar ultimo digito verificador
void thread_verify_second_test_digit(void const *args){
  bool verifiedByte = false;
  msg_pipe_elem_t current_elem;
  while(!hasValidKey){
    if(!verifiedByte){ 
      if(!hasDecipheredMsg){
        yield;
        continue;
      }
      uint32_t time = osKernelSysTick()/ticks_factor;
      memcpy(&current_elem, &(msgInStage(PIPE_STG_DECIPHERED)), sizeof(msg_pipe_elem_t));
      test2Loaded = true;
      if(test1Loaded){
        hasDecipheredMsg = false;
        test1Loaded = false;
        test2Loaded = false;
      }
      //---
      uint8_t testByte = current_elem.deciphered_msg[TEST_2_INDEX];
      uint16_t squaredKey = current_elem.key;
      squaredKey *= squaredKey;
      uint8_t prevPrime = current_elem.prev_prime;
      current_elem.secondTestResult = squaredKey/prevPrime == testByte;
      //---
      current_elem.hasSecondTest = true;
      verifiedByte = true;
      fprintf(file," thread_verify_second_test_digit : %i, %i\n", (int)time, (int)osKernelSysTick()/ticks_factor);
    } 
    if(verifiedByte){ 
      if(hasVerifiedTest2){
        yield;
        continue;
      }
      /** Perigo de concorrencia
      current_elem.firstTestResult = msgInStage(PIPE_STG_VERIFIED).firstTestResult;
      current_elem.hasFirstTest = msgInStage(PIPE_STG_VERIFIED).hasFirstTest;

      memcpy(&(msgInStage(PIPE_STG_VERIFIED)), &current_elem, MSG_SIZE);
      **/
      msg_pipe_elem_t* msg_st_ptr = &(msgInStage(PIPE_STG_VERIFIED));
      msg_st_ptr->key = current_elem.key;
      msg_st_ptr->prev_prime = current_elem.prev_prime;
      msg_st_ptr->hasKey = current_elem.hasKey;
      memcpy(msg_st_ptr->deciphered_msg, current_elem.deciphered_msg, MSG_SIZE);
      msg_st_ptr->hasMsg = current_elem.hasMsg;
      msg_st_ptr->secondTestResult = current_elem.secondTestResult;
      msg_st_ptr->hasSecondTest = current_elem.hasSecondTest;
      
      hasVerifiedTest2 = true;
      verifiedByte = false;
    }
 
    yield;
  }
  osThreadTerminate(id_thread_verify_second_test_digit);
}
osThreadDef(thread_verify_second_test_digit, osPriorityNormal, 1, 0);

//Thread para escrever na saida chave gerada
void thread_print_key(void const *args){
  msg_pipe_elem_t current_elem;
  while(!hasValidKey){
    if(!(hasVerifiedTest1 && hasVerifiedTest2)){
      yield;
      continue;
    }
     uint32_t time = osKernelSysTick()/ticks_factor;
    memcpy(&current_elem, &(msgInStage(PIPE_STG_VERIFIED)), sizeof(msg_pipe_elem_t));
    
    printf("--------------------\n");
    printf("Key: 0x%02x\n", current_elem.key);
    printf("Printable chars: ");
    for(int i = 0; i < MSG_SIZE-2; i++)
      if(isprint(current_elem.deciphered_msg[i])) printf("%c", current_elem.deciphered_msg[i]);
    printf("\n");
    printf("Message Bytes: \n");
    for(int i = 0; i < MSG_SIZE; i++){
      if(i % 8 == 0) printf("    0x ");
      printf("%02x ", current_elem.deciphered_msg[i]);
      if(i % 8 == 7) printf("\n");
    }
    printf("Test 1: "); printf(current_elem.firstTestResult  ? "passed\n" : "failed\n");
    printf("Test 2: "); printf(current_elem.secondTestResult ? "passed\n" : "failed\n");
    printf("\n");
    //---
    //Enquanto não existe validação
    hasVerifiedTest1 = false;
    hasVerifiedTest2 = false;
    //---
      fprintf(file," thread_print_key : %i, %i\n", (int)time, (int)osKernelSysTick()/ticks_factor);
    yield;
  }
  osThreadTerminate(id_thread_print_key);
}
osThreadDef(thread_print_key, osPriorityNormal, 1, 0);

//Thread para testar ultimo digito verificador
void thread_validate_key(void const *args){
  uint32_t time = osKernelSysTick()/ticks_factor;
  fprintf(file," Led1 : %i, %i\n", (int)time, (int)osKernelSysTick()/ticks_factor);
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
  id_thread_print_key =                osThreadCreate(osThread(thread_print_key),                NULL);
  id_thread_validate_key =             osThreadCreate(osThread(thread_validate_key),             NULL);
  
  //************************
  //Fim de inicializações de Threads
  //************************
  
  //************************
  // inicializações  diagram Gantt
  //************************  
  
  
    file = fopen("gantt.txt","w");
   
    fprintf(file,"gantt\n");
    fprintf(file,"    title A Gantt Diagram\n");
    fprintf(file,"    dateFormat x\n");
  
      
  
  //Início do SO
  osKernelStart();
  
  //Main thread
  thread_main();
  return 0;
}