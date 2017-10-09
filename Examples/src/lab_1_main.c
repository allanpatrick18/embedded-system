#include "type.h"
#include "libdemo.h"
#include "stdio.h"
#include "stdlib.h"
#include "cmsis_os.h"

//*************************
//*********************** *
//DEFINICOES DO SISTEMA * *
//*********************** *
//*************************
#define yield osThreadYield()

//************************
//IDs de Threads
//************************
osThreadId id_thread_generate;
osThreadId id_thread_decipher;
osThreadId id_thread_test_1;
osThreadId id_thread_test_2;
osThreadId id_thread_print;
osThreadId id_thread_validate;

//*************************
//*********************** *
//DEFINICOES DO DOMINIO * *
//*********************** *
//*************************
//tamanho da mensagem
#define MSG_SIZE 32     
//indices dos bytes de verificacao
#define TEST_1_INDEX MSG_SIZE-2
#define TEST_2_INDEX MSG_SIZE-1

//menssagem para decodificar
#define MESSAGE_0 //_1 ou _2
//Comentar para nao gerar Gannt
#define GANTT

//mensagem codificada
#ifdef MESSAGE_0
unsigned const char hashed_msg[MSG_SIZE] = { 
  0x67, 0x52, 0x89, 0x4a, 0x8b, 0x4e, 0x8a, 0x09, 
  0x86, 0x4f, 0x37, 0x3c, 0x80, 0x55, 0x80, 0x4c,
  0x86, 0x57, 0x37, 0x3f, 0x78, 0x55, 0x83, 0x4e,
  0x90, 0x09, 0x48, 0x22, 0x50, 0x22, 0x22, 0x04 
};
#endif
#ifdef MESSAGE_1
unsigned const char hashed_msg[MSG_SIZE] = { 
  0x97, 0x25, 0xa8, 0xdd, 0x96, 0x25, 0xa4, 0x34,
  0xb6, 0x25, 0xa4, 0x2b, 0xae, 0xdd, 0x95, 0x22,
  0xa7, 0x22, 0xb0, 0x2d, 0xb7, 0x26, 0xb2, 0x2b,
  0x63, 0xdd, 0x74, 0xf6, 0x7c, 0xf1, 0x64, 0x06
};
#endif
#ifdef MESSAGE_2
unsigned const char hashed_msg[MSG_SIZE] = { 
  0xa7, 0x15, 0xb8, 0xcd, 0x97, 0x0e, 0xc5, 0x18,
  0x73, 0xf8, 0xc1, 0x16, 0xba, 0x15, 0xc7, 0xcd,
  0x73, 0xcd, 0x73, 0xcd, 0x73, 0xcd, 0x73, 0xcd,
  0x73, 0xcd, 0x85, 0xdd, 0x83, 0xe5, 0x7c, 0x04
};
#endif

//definições para variaveis booleanas
typedef uint8_t bool;
#define false 0
#define true 1
FILE *file;
int ticks_factor = 100;
//struct para guardar estados da decodificação da mensagem por cada chave gerada
typedef struct decodingState{
  unsigned char key;                            //chave da decodificação
  unsigned char prevPrime;                     //último primo (usado em teste 2)
  bool hasKey;                                  //flag de chave criada
  unsigned char deciphered_msg[MSG_SIZE];       //mensagem decodificada com chave gerada       
  bool hasMsg;                                  //flag de mensagem decodificada        
  bool firstTestResult;                         //resultado do primeiro teste
  bool hasFirstTest;                            //flag de primeira verificação realizada        
  bool secondTestResult;                        //resultado do segundo teste
  bool hasSecondTest;                           //flag de segunda verificação realizada
} decodingState_t;

//Tamanho da "Pipeline"
#define PIPE_SIZE 3
//Estagios da "Pipeline" são indices no vetor
#define PIPE_STG_GENERATED  0
#define PIPE_STG_DECIPHERED 1
#define PIPE_STG_VERIFIED   2
//simbolo abstrai recuperação de mensagem por estagio
#define msgInStage(stage) pipe[stage]
//"pipeline" armazena estado de codificao entre as transições das Threads
decodingState_t pipe[PIPE_SIZE] = {0};

//*************************
//Flags para transição entre threads
bool hasGeneratedKey = false;   //Flag de chave gerada
bool hasDecipheredMsg = false;  //Flag de mensagem decodificada
bool hasVerifiedTest1 = false;  //Flag de verificação do penultimo byte realizada
bool hasVerifiedTest2 = false;  //Flag de verificação do ultimo byte realizada
bool hasPrinted = false;        //Flag de impressão da mensagem realizada
bool hasValidated = false;      //Flag de validação da chave realizada

//*************************
//Flags para comunicação entre threads
bool foundValidKey = false;     //Flag indica chave valida foi encontrada e 
bool threadFailed = false;      //Flag indica falha critica em uma thread        

//*************************
//Flags para sincronização entre Threads paralelas
bool test1Loaded = false;       //Flag indica Thread Test 1 carregou dados da pipeline
bool test2Loaded = false;       //Flag indica Thread Test 2 carregou dados da pipeline

bool printLoaded = false;       //Flag indica Thread Print carregou dados da pipeline
bool validationLoaded = false;  //Flag indica Thread Validate carregou dados da pipeline

//Entre 0 e 255 existem 54 primos, sendo o ultimo 251
#define PRIME_LIST_SIZE 54
#define EMPTY 0x00                      //Valor vazio na lista de primos        
#define FAILURE 0xFF                    //Falha no acesso da lista de primos
#define isEmpty(n) (n > EMPTY)          //Abstração para verificar se indice possui numero primo
#define isFailure(n) (n == FAILURE)     //Abstração para verificar se houve falha no acesso da lista
//Lista de primos, cada inidice contem um primo em ordem (calculado em tempo de execução)
uint8_t primeList[PRIME_LIST_SIZE] = {0};

//estrutura para guradar tempos de uma seção (chave) para diagrama de Gantt
typedef struct gantt_times{
  struct gantt_times* next;
  unsigned char key;
  int key_init;
  int key_end;
  int dec_init;
  int dec_end;
  int test1_init;
  int test1_end;
  int test2_init;
  int test2_end;
  int print_init;
  int print_end;
  int valid_init;
  int valid_end;
} gantt_times_t;

gantt_times_t* gantt_list = NULL;

//*************************
//Utilidades diversas
#define RUNNING !(foundValidKey || threadFailed)
#define isEven(n) (n%2)

//************************
//Funções Auxiliares

//Implementação minimal para copia de memoria entre endereços
void memcpy(void* destination, void* origin, size_t n){
  uint8_t* d = destination;
  uint8_t* o = origin;
  for(size_t i = 0; i < n; i++){
    d[i] = o[i];
  }
}

//Verifica se caracter pode ser visualizado quando impresso
bool isprint(char c){
  //na tabela ascii, carecteres validos estao nesta regiao
  return c >= ' ' && c <= '~'; 
}

//retorna um n-esimo numero primeiro por index
uint8_t getPrime(size_t index){
  //retorna FAILURE se indice for maior que a lista de primos
  if(index > PRIME_LIST_SIZE)
    return FAILURE;
  
  //recupera numero no indice e verifica se ja foi calculado - 
  //caso tenha sido, retorne-o
  uint8_t prime = primeList[index];
  if(isEmpty(prime))
    return prime;
  
  //se for o primeiro, inicialize-o com 2, o primeiro primo, e retorne
  if(index == 0){
    primeList[index] = 2;
    return 2;
  }
  
  //inicializacoes
  bool notPrime;                               //flag indica que o numero possui divisor
  uint8_t last_prime = getPrime(index-1);      //recupera da tabela primo anterior
  uint8_t next_prime;                          //variavel para calcular primo
  //laco externo incrementa variavel de possivel primo a cada iteração mal sucedida
  for(next_prime = last_prime+1; 
      next_prime > last_prime;
      next_prime++){
      //reinicialização de flag após inicio ou falha (encontrou numero divisivel por outro primo)
      notPrime = false;
      //laco interno verifica se possivel primo eh divisivel pelos primos anteriores
      for(uint8_t prevPrime_index = 0; 
          prevPrime_index < index;
          prevPrime_index++){
          uint8_t divisor = getPrime(prevPrime_index);
          //verifica se eh divisivel por outro primo
          if(next_prime%divisor == 0){
            //caso seja divisivel, nao eh primo, entao deve-se terminar laco 
            //interno e testar outro numero pelo laco externo
            notPrime = true;
            break;
          }
      }
      //caso nao tenho encontrado nenhum divisor pelos outros primos, 
      //eh primo, e finaliza laco externo
      if(!notPrime) break;
  }
  
  //Se não encontrar primo, houve falha -
  //indique na tabela e retorne a falha
  if(notPrime){
    primeList[index] = FAILURE;
    return FAILURE;
  }
  
  //Se encontrar primo, atualiza a tabela e retorna valor encontrado
  primeList[index] = next_prime;
  return next_prime;
}

//decifra mensagem com chave
void decipherMsg(decodingState_t* msgState){
  //decifra apenas se possuir chave
  if(!msgState->hasKey)
    return;
  unsigned char* msg = msgState->deciphered_msg;
  uint8_t key = msgState->key;
  //soma chave quando indice do byte eh par, e subtrai quando impar
  //(Obs.: Indices a partir de 1, não de 0)
  for(int i = 0; i < MSG_SIZE; i++){
    msg[i] = isEven(i) ? hashed_msg[i] + key : hashed_msg[i] - key;
  }
}

//realiza teste no penultimo byte da mensagem decifrada
bool verifyFirstTest(decodingState_t* msgState){
  //byte de teste eh penultimo byte da mensagem decifrada
  uint8_t testByte = msgState->deciphered_msg[TEST_1_INDEX];
  uint8_t key = msgState->key;
  uint8_t halfKey = key>>1;
  //retorna comparacao entre metade da chave e byte de teste
  return (halfKey == testByte);
}

//realiza teste no ultimo byte da mensagem decifrada
bool verifySecondTest(decodingState_t* msgState){
  //byte de teste eh ultimo byte da mensagem decifrada
  uint8_t testByte = msgState->deciphered_msg[TEST_2_INDEX];
  uint8_t prevPrime = msgState->prevPrime;
  uint8_t key = msgState->key; 
  uint16_t squaredKey = key*key;
  uint8_t hashedByte = squaredKey/prevPrime;
  //compara o quadrado da chave dividos pelo primo anterior com o byte de teste
  return (hashedByte == testByte);
}

//imprime o resultado de uma decodificação
void printMsgState(decodingState_t* msgState){
      printf("--------------------\n");
      printf("Key: %d (0x%02X)\n", msgState->key ,msgState->key);
      printf("Printable chars: ");
      for(int i = 0; i < MSG_SIZE-2; i++)
        if(isprint(msgState->deciphered_msg[i])) //imprime apenas caracteres visiveis
          printf("%c", msgState->deciphered_msg[i]);
      printf("\n");
      printf("Message Bytes: \n");
      for(int i = 0; i < MSG_SIZE; i++){
        if(i % 8 == 0) printf("    0x ");
        printf("%02X ", msgState->deciphered_msg[i]);
        if(i % 8 == 7) printf("\n");
      }
      printf("Test 1: "); printf(msgState->firstTestResult  ? "passed\n" : "failed\n");
      printf("Test 2: "); printf(msgState->secondTestResult ? "passed\n" : "failed\n");
      printf("\n");
}

//realiza validação de uma decodificação comparando o resultado dos testes
bool validateKey(decodingState_t* msgState){
  return msgState->firstTestResult && msgState->secondTestResult;
}

//grava diagrama de gantt a partir dos dados obtidos
gantt_times_t* print_gantt(gantt_times_t* gantt){
  fprintf(file,"  section [Key 0x%02x]\n", gantt->key);
  if(gantt->key_init != -1 && gantt->key_end != -1)
    fprintf(file,"    Thread Generate : [Key 0x%02x], %i, %i\n", gantt->key, gantt->key_init, gantt->key_end);
  if(gantt->dec_init != -1 && gantt->dec_end != -1)
    fprintf(file,"    Thread Decipher : [Key 0x%02x], %i, %i\n", gantt->key, gantt->dec_init, gantt->dec_end);
  if(gantt->test1_init != -1 && gantt->test1_end != -1)
    fprintf(file,"    Thread Test 1   : [Key 0x%02x], %i, %i\n", gantt->key, gantt->test1_init, gantt->test1_end);
  if(gantt->test2_init != -1 && gantt->test2_end != -1)
    fprintf(file,"    Thread Test 2   : [Key 0x%02x], %i, %i\n", gantt->key, gantt->test2_init, gantt->test2_end);
  if(gantt->print_init != -1 && gantt->print_end != -1)
    fprintf(file,"    Thread Print    : [Key 0x%02x], %i, %i\n", gantt->key, gantt->print_init, gantt->print_end);
  if(gantt->valid_init != -1 && gantt->valid_end != -1)
    fprintf(file,"    Thread Validate : [Key 0x%02x], %i, %i\n", gantt->key, gantt->valid_init, gantt->valid_end);
  
  return gantt->next;
}

//************************
//Threads

//Thread para geração de chaves
void thread_generate(void const *args){
  size_t p_index = 0;   //indice na tabela de primos
  
  uint32_t time;        //cronometro para gerar Gantt        
  while(RUNNING){       //enquanto programa estiver ativo      
    if(!hasGeneratedKey){       //se ultima chave foi consumida
      time = osKernelSysTick()/ticks_factor;    //inicio do cronometro para Gantt
      //estado de decodificação no primeiro estagio do pipeline
      decodingState_t *msgTestState = &(msgInStage(PIPE_STG_GENERATED));        

      //recupera numero primo e verifica se é valido
      //este valor sera utilizado na verificação do ultimo digito
      uint8_t prevPrime = getPrime(p_index++);
      if(isFailure(prevPrime)) {
        //se nao for, termina execução do programa
        threadFailed = true;
        break;
      }

      //recupera proximo primo para chave e verifica se é valido
      uint8_t key = getPrime(p_index);
      if(isFailure(key)) {
        //se nao for, termina execução do programa
        threadFailed = true;
        break;
      }

      //guardando informações no primeiro estagio do pipeline
      msgTestState->prevPrime = prevPrime;
      msgTestState->key = key;
      msgTestState->hasKey = true;      //flag indica estado do estagio da pipeline (chave gerada)

      osDelay(50);
      pca_toggle(7);
      osDelay(50);
      pca_toggle(7);
      
      //salva tempo de processamento para Gantt
//      fprintf(file," Thread Generate : [Key 0x%02x], %i, %i\n", msgTestState->key, (int)time, (int)osKernelSysTick()/ticks_factor);
      gantt_times_t* gantt;
      if(!(gantt = (gantt_times_t*) malloc(sizeof(gantt_times_t)))){
        //se nao for, termina execução do programa
        threadFailed = true;
        break;
      }      
      gantt_times_t* last = gantt_list;
      gantt->next = NULL;
      if(last == NULL)
        gantt_list = gantt;
      else{
        while(last->next != NULL)
          last = last->next;
        last->next = gantt;
      }
      gantt->key = key;
      gantt->key_init = (int)time;
      gantt->key_end = (int)osKernelSysTick()/ticks_factor;
      gantt->dec_init = -1;
      gantt->dec_end = -1;
      gantt->test1_init = -1;
      gantt->test1_end = -1;
      gantt->test2_init = -1;
      gantt->test2_end = -1;
      gantt->print_init = -1;
      gantt->print_end = -1;
      gantt->valid_init = -1;
      gantt->valid_end = -1;
      
      hasGeneratedKey = true;   //indica que estagio do pipeline esta ocupado e precisa ser esvaziado
    }
    //abdica do processador quando termina processamento
    yield;
  }
  osDelay(osWaitForever);
}
osThreadDef(thread_generate, osPriorityNormal, 1, 0);

//Thread para decifrar chaves
void thread_decipher(void const *args){
  bool processedMessage = false;        //flag de estado interno indica se mensagem foi decifrado e pode ser armazenado no proximo estagio do pipeline
  decodingState_t currentElem;          //estado interno de codificação da mensagem
  uint32_t time;                        //cronometro para gerar Gantt
  while(RUNNING){                       //enquanto programa estiver ativo
    //primeiro estado interno da thread, espera chave estar disponivel
    if(!processedMessage && hasGeneratedKey){
      time = osKernelSysTick()/ticks_factor;    //inicio do cronometro para Gantt
      //copia estado de decodificacao do primeiro estagio da pipeline para estado interno da thread
      memcpy(&currentElem, &(msgInStage(PIPE_STG_GENERATED)), sizeof(decodingState_t));
      hasGeneratedKey = false;  //indicando que chave foi consumida e thread anterior ja pode executar        
      //---
      //execucao principal da thread (decifrar)
      //---
      decipherMsg(&currentElem);
      //---
      currentElem.hasMsg = true;        //flag indica estado de decodificacao (decifrado)
      //flag indica passagem para proximo estado interno, onde esperara proximo estagio de pipeline liberar
      processedMessage = true;          
    } 
    //segundo estagio da thread, espera proximo estagio da pipeline para guardar novo estado de decodificacao
    if(processedMessage && !hasDecipheredMsg){
      //copia estado interno de decodificacao para proximo estagio de pipeline
      memcpy(&(msgInStage(PIPE_STG_DECIPHERED)), &currentElem, sizeof(decodingState_t));
      //indica fim de processamento da thread para Gantt
//      fprintf(file," Thread Decipher : [Key 0x%02x], %i, %i\n", currentElem.key, (int)time, (int)osKernelSysTick()/ticks_factor);
      osDelay(100);
      gantt_times_t* holder = gantt_list;
      if(holder != NULL){
        while(holder->next != NULL && holder->key != currentElem.key)
          holder = holder->next;
        holder->dec_init = (int)time;
        holder->dec_end = (int)osKernelSysTick()/ticks_factor;
      }
      hasDecipheredMsg = true;  //indica para proxima thread que a mensagem foi decifrada
      processedMessage = false; //retorno ao primeiro estado interno da thread
    }
    //enquanto nao poder entrar em nenhum estado interno, libera processador
    yield;
  }  
  osDelay(osWaitForever);
}
osThreadDef(thread_decipher, osPriorityNormal, 1, 0);

//Thread para testar penultimo digito verificador
void thread_test_1(void const *args){
  bool verifiedByte = false;            //flag de estado interno indica se mensagem decifrada teve penultimo byte testado e pode ser armazenado no proximo estagio do pipeline
  decodingState_t currentElem;          //estado interno de codificação da mensagem
  uint32_t time;                        //cronometro para gerar Gantt
  while(RUNNING){                       //enquanto programa estiver ativo
    //primeiro estado interno da thread, espera mensagem ser decifrada
    if(!verifiedByte && hasDecipheredMsg){
      time = osKernelSysTick()/ticks_factor;    //inicio do cronometro para Gantt
      //copia estado de decodificacao estagio anterior da pipeline para estado interno da thread
      memcpy(&currentElem, &(msgInStage(PIPE_STG_DECIPHERED)), sizeof(decodingState_t));
      //flag para sincronizar threads paralelas durante carregamento
      test1Loaded = true;
      //se ambas threads finalizaram carregamento da pipeline (primeiro estado interno) ...
      if(test2Loaded){
        //... entao thread anterior ja pode carregar novos dados no estagio
        hasDecipheredMsg = false;
        test1Loaded = false;
        test2Loaded = false;
      }
      
      //---
      //execucao principal da thread (verificar penultimo byte)
      //---
      currentElem.firstTestResult = verifyFirstTest(&currentElem);
      //---
      currentElem.hasFirstTest = true;
      verifiedByte = true;
  
    } 
    //segundo estagio da thread, espera proximo estagio da pipeline para guardar novo estado de decodificao
    if(verifiedByte && !hasVerifiedTest1){
      //recupera referencia do proximo estagio
      decodingState_t* msgTestState = &(msgInStage(PIPE_STG_VERIFIED));
      //copia informacoes comuns para ambas as threads paralelas
      memcpy(msgTestState, &currentElem, (sizeof(unsigned char) + sizeof(bool))*2 + MSG_SIZE);
      //copia resultado da verificacao
      msgTestState->firstTestResult = currentElem.firstTestResult;
      msgTestState->hasFirstTest = currentElem.hasFirstTest; //flag indica estado de decodificacao (penultimo byte verificado)
      
      //indica fim de processamento da thread para Gantt
//      fprintf(file," Thread Test 1   : [Key 0x%02x], %i, %i\n", currentElem.key, (int)time, (int)osKernelSysTick()/ticks_factor);
      osDelay(100);
      gantt_times_t* holder = gantt_list;
      if(holder != NULL){
        while(holder->next != NULL && holder->key != currentElem.key)
          holder = holder->next;
        holder->test1_init = (int)time;
        holder->test1_end = (int)osKernelSysTick()/ticks_factor;
      }

      //indica para proxima thread que o penultimo byte da mensagem foi verificado
      hasVerifiedTest1 = true;
      //retorno ao primeiro estado interno da thread
      verifiedByte = false;
    }
    //enquanto nao poder entrar em nenhum estado interno, libera processador
    yield;
  }
  osDelay(osWaitForever);
}
osThreadDef(thread_test_1, osPriorityNormal, 1, 0);

//Thread para testar ultimo digito verificador
void thread_test_2(void const *args){
  bool verifiedByte = false;            //flag de estado interno indica se mensagem decifrada teve ultimo byte testado e pode ser armazenado no proximo estagio do pipeline
  decodingState_t currentElem;          //estado interno de codificação da mensagem
  uint32_t time;                        //cronometro para gerar Gantt
  while(RUNNING){                       //enquanto programa estiver ativo
    //primeiro estado interno da thread, espera mensagem ser decifrada
    if(!verifiedByte && hasDecipheredMsg){
      time = osKernelSysTick()/ticks_factor;    //inicio do cronometro para Gantt
      //copia estado de decodificacao estagio anterior da pipeline para estado interno da thread
      memcpy(&currentElem, &(msgInStage(PIPE_STG_DECIPHERED)), sizeof(decodingState_t));
      //flag para sincronizar threads paralelas durante carregamento
      test2Loaded = true;
      //se ambas threads finalizaram carregamento da pipeline (primeiro estado interno) ...
      if(test1Loaded){
        //... entao thread anterior ja pode carregar novos dados no estagio
        hasDecipheredMsg = false;
        test1Loaded = false;
        test2Loaded = false;
      }

      //---
      //execucao principal da thread (verificar ultimo byte)
      //---
      currentElem.secondTestResult = verifySecondTest(&currentElem);      
      //---
      currentElem.hasSecondTest = true;
      verifiedByte = true;
    } 
    //segundo estagio da thread, espera proximo estagio da pipeline para guardar novo estado de decodificao
    if(verifiedByte && !hasVerifiedTest2){
      //recupera referencia do proximo estagio
      decodingState_t* msgTestState = &(msgInStage(PIPE_STG_VERIFIED));
      //copia informacoes comuns para ambas as threads paralelas
      memcpy(msgTestState, &currentElem, (sizeof(unsigned char) + sizeof(bool))*2 + MSG_SIZE);
      
      //copia resultado da verificacao
      msgTestState->secondTestResult = currentElem.secondTestResult;
      msgTestState->hasSecondTest = currentElem.hasSecondTest;  //flag indica estado de decodificacao (ultimo byte verificado)
      
      //indica fim de processamento da thread para Gantt
//      fprintf(file," Thread Test 2   : [Key 0x%02x], %i, %i\n", currentElem.key, (int)time, (int)osKernelSysTick()/ticks_factor);
      osDelay(100);
      gantt_times_t* holder = gantt_list;
      if(holder != NULL){
        while(holder->next != NULL && holder->key != currentElem.key)
          holder = holder->next;
        holder->test2_init = (int)time;
        holder->test2_end = (int)osKernelSysTick()/ticks_factor;
      }

      //indica para proxima thread que o ultimo byte da mensagem foi verificado
      hasVerifiedTest2 = true;
      //retorno ao primeiro estado interno da thread
      verifiedByte = false;
    }
    //enquanto nao poder entrar em nenhum estado interno, libera processador
    yield;
  }
  osDelay(osWaitForever);
}
osThreadDef(thread_test_2, osPriorityNormal, 1, 0);

//Thread para escrever na saida chave gerada
void thread_print(void const *args){
  //Controlador de estado interno, espera Thread Validate para 
  //sincronizacao
  bool waitingValidation = false;
  //Estado de codificação de mensagem interno à Thread
  decodingState_t currentElem;
   uint32_t time;               //cronometro para gerar Gantt
  //Enquanto não houver uma falha critica e uma chave valida não for encontrada,
  //execute...
  while(RUNNING){
    //primeiro estado interno da thread, esperando estagios anteriores enviarem mensagem
    //as Threads Test 1 e Test 2 devem ter terminado de processar uma
    //instancia de decodificacao para poder imprimir resultado
    if(!waitingValidation && hasVerifiedTest1 && hasVerifiedTest2){
      time = osKernelSysTick()/ticks_factor;            //inicia cronometro para Gantt
      //Copia uma instancia de decodificacao passada por Thread Test 1 e Test 2
      memcpy(&currentElem, &(msgInStage(PIPE_STG_VERIFIED)), sizeof(decodingState_t));
      
      //Garante que as Threads Test 1 e Test 2 poderao salvar nova 
      //instancia de decodificacao apenas depois que Thread Print e Validate
      //carregar ultima instancia passada.
      printLoaded = true;
      if(validationLoaded){
          //Se ambas as threads tiverem carregado os dados, Test 1 e Test 2 
          //podem armazenar nova instancia
          hasVerifiedTest1 = false;
          hasVerifiedTest2 = false;
          //Reseta flags de carregamento
          printLoaded = false;
          validationLoaded = false;
      }
      
      //Imprime valor de chave, mensagem decodificada, seu valor em bytes e 
      //o resultado dos testes
      printMsgState(&currentElem);
      
      //Prepara flags para passagem de estado (espera que Thread Validate 
      //termine)
      hasPrinted = true;
      waitingValidation = true;
    }
    //Se estiver esperando a Thread Validate, e ela terminar, reseta flags de 
    //espera
    if(waitingValidation && hasValidated){
//      fprintf(file," Thread Print    : [Key 0x%02x], %i, %i\n", currentElem.key, (int)time, (int)osKernelSysTick()/ticks_factor);
      osDelay(100);
      gantt_times_t* holder = gantt_list;
      if(holder != NULL){
        while(holder->next != NULL && holder->key != currentElem.key)
          holder = holder->next;
        holder->print_init = (int)time;
        holder->print_end = (int)osKernelSysTick()/ticks_factor;
      }

      waitingValidation = false;
      hasValidated = false;
    }
    //Após processamento, libera processador
    yield;
  }
  //Finaliza thread ao identificar fim de execução
  osDelay(osWaitForever);
}
osThreadDef(thread_print, osPriorityNormal, 1, 0);

//Thread para validar a chave gerada, utilizando os testes realizados
void thread_validate(void const *args){
  //Controlador de estado interno, espera Thread Printing para 
  //sincronizacao
  bool waitingPrinting = false;
  bool isValid = false; 
  //Estado de codificação de mensagem interno à Thread
  decodingState_t currentElem;
  uint32_t time;                //cronometro para gerar Gantt
  //Enquanto não houver uma falha critica e uma chave valida não for encontrada,
  //execute...
  while(RUNNING){
    //primeiro estado interno da thread, esperando estagios anteriores enviarem mensagem
    //as Threads Test 1 e Test 2 devem ter terminado de processar uma
    //instancia de decodificacao para poder imprimir resultado
    if(!waitingPrinting && hasVerifiedTest1 && hasVerifiedTest2){
      time = osKernelSysTick()/ticks_factor;            //inicia cronometro para Gantt
      //Copia uma instancia de decodificacao passada por Thread Test 1 e Test 2
      memcpy(&currentElem, &(msgInStage(PIPE_STG_VERIFIED)), sizeof(decodingState_t));
      
      //Garante que as Threads Test 1 e Test 2 poderao salvar nova 
      //instancia de decodificacao apenas depois que Thread Print e Validate
      //carregar ultima instancia passada.
      validationLoaded = true;
      if(printLoaded){
          //Se ambas as threads tiverem carregado os dados, Test 1 e Test 2 
          //podem armazenar nova instancia
          hasVerifiedTest1 = false;
          hasVerifiedTest2 = false;
          //Reseta flags de carregamento
          printLoaded = false;
          validationLoaded = false;
      }
      
      //Valida chave com base nos testes realizados
      isValid = validateKey(&currentElem);
      
      if(isValid){
        osDelay(50);
        pca_toggle(0);
        osDelay(50);
      }
      
      //Prepara flags para passagem de estado (espera que Thread Validate 
      //termine)      
      waitingPrinting = true;
      hasValidated = true;
    }
    //Se estiver esperando a Thread Validate, e ela terminar, reseta flags de 
    //espera
    if(waitingPrinting && hasPrinted){
//      fprintf(file," Thread Validate : [Key 0x%02x], %i, %i\n", currentElem.key, (int)time, (int)osKernelSysTick()/ticks_factor);
      osDelay(100);
      gantt_times_t* holder = gantt_list;
      gantt_times_t* last = NULL;
      if(holder != NULL){
        while(holder->next != NULL && holder->key != currentElem.key){
          last = holder;
          holder = holder->next;
        }
        holder->valid_init = (int)time;
        holder->valid_end = (int)osKernelSysTick()/ticks_factor;
        
        if(last != NULL)
          last->next = print_gantt(holder);
        else
          gantt_list = print_gantt(holder);
        free(holder);
      }

      foundValidKey = isValid;
      waitingPrinting = false;
      hasPrinted = false;
    }
    //Após processamento, libera processador
    yield;
  }
  osDelay(osWaitForever);
}
osThreadDef(thread_validate, osPriorityNormal, 1, 0);

//************************
//Código da thread Main
//************************
void thread_main(){
  //Espera execução das threads
  while(RUNNING) yield;

#ifdef GANTT
  gantt_times_t* gantt = gantt_list;
  while (gantt != NULL)
    gantt = print_gantt(gantt);
#endif
  
  printf("finished");
}

int main(int n_args, char** args){
  //Inicialização de Kernel vai aqui
  int status = osKernelInitialize();
  if(status != osOK){
    printf("Kernel failed to initialise. Status: %d", status);
    return -1;
  }
  
  //************************
  //Inicializacao de Threads aqui
  //************************
  id_thread_generate =  osThreadCreate(osThread(thread_generate),       NULL);
  id_thread_decipher =  osThreadCreate(osThread(thread_decipher),       NULL);
  id_thread_test_1 =    osThreadCreate(osThread(thread_test_1),         NULL);
  id_thread_test_2 =    osThreadCreate(osThread(thread_test_2),         NULL);
  id_thread_print =     osThreadCreate(osThread(thread_print),          NULL);
  id_thread_validate =  osThreadCreate(osThread(thread_validate),       NULL);

  //************************
  //Inicializacao do PCA
  //*********************** 
  I2CInit( (uint32_t)I2CMASTER, 0 );
  pca9532_init();
  
  //************************
  //Inicializacao do Diagrama de Gantt
  //*********************** 
  file = fopen("gantt.txt","w");
 
  fprintf(file,"gantt\n");
  fprintf(file,"  title A Gantt Diagram\n");
  fprintf(file,"  dateFormat x\n");
  
  //Início do SO
  if(osKernelRunning()){
    printf("Kernel already started");
    return -1;
  }
  
  status = osKernelStart();
  if(status != osOK){
    printf("Kernel failed to start. Status: %d", status);
  }
  
  //Main thread
  thread_main();

  fclose(file);

  //************************
  //Finalização de Threads aqui
  //************************
  osThreadTerminate(id_thread_validate);
  osThreadTerminate(id_thread_generate);
  osThreadTerminate(id_thread_decipher);
  osThreadTerminate(id_thread_test_1);
  osThreadTerminate(id_thread_test_2);  
  osThreadTerminate(id_thread_print);
  
  osDelay(osWaitForever); 
  return 0;
}