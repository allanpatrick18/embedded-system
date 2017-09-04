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

//mensagem codificada
unsigned const char hashed_msg[MSG_SIZE] = { 
  0x67, 0x52, 0x89, 0x4a, 0x8b, 0x4e, 0x8a, 0x09, 
  0x86, 0x4f, 0x37, 0x3c, 0x80, 0x55, 0x80, 0x4c,
  0x86, 0x57, 0x37, 0x3f, 0x78, 0x55, 0x83, 0x4e,
  0x90, 0x09, 0x48, 0x22, 0x50, 0x22, 0x22, 0x04 //0x8b, 0xd6   
  //??Quando penultimo e ultimo bytes são 0x8b e 0xd6 funciona??
};

//definições para variaveis booleanas
typedef uint8_t bool;
#define false 0
#define true 1

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

//*************************
//Utilidades diversas
#define RUNNING !(foundValidKey || threadFailed)
#define isEven(n) (n%2)

//************************
//Funções Auxiliares
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
  //compara o quadrado da chave dividos pelo primo anterior com o byte de teste
  return ((squaredKey/prevPrime) == testByte);
}

void printMsgState(decodingState_t* msgState){
      printf("--------------------\n");
      printf("Key: %d (0x%02X)\n", msgState->key ,msgState->key);
      printf("Printable chars: ");
      for(int i = 0; i < MSG_SIZE-2; i++)
        if(isprint(msgState->deciphered_msg[i])) printf("%c", msgState->deciphered_msg[i]);
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

bool validateKey(decodingState_t* msgState){
  return msgState->firstTestResult && msgState->secondTestResult;
}

//************************
//Threads

//Thread para geração de chaves
void thread_generate(void const *args){
  size_t p_index = 0;
  while(RUNNING){
    if(!hasGeneratedKey){
      decodingState_t *msgTestState = &(msgInStage(PIPE_STG_GENERATED));

      uint8_t prevPrime = getPrime(p_index++);
      if(isFailure(prevPrime)) {
        threadFailed = true;
        break;
      }

      uint8_t key = getPrime(p_index);
      if(isFailure(key)) {
        threadFailed = true;
        break;
      }

      msgTestState->prevPrime = prevPrime;
      msgTestState->key = key;
      msgTestState->hasKey = true;
      hasGeneratedKey = true;
    }
    yield;
  }
  osDelay(osWaitForever);
}
osThreadDef(thread_generate, osPriorityNormal, 1, 0);

//Thread para decifrar chaves
void thread_decipher(void const *args){
  bool processedMessage = false;
  decodingState_t currentElem;
  while(RUNNING){
    if(!processedMessage && hasGeneratedKey){
      memcpy(&currentElem, &(msgInStage(PIPE_STG_GENERATED)), sizeof(decodingState_t));
      hasGeneratedKey = false;
      //---
      decipherMsg(&currentElem);
      //---
      currentElem.hasMsg = true;
      processedMessage = true;
    } 
    if(processedMessage && !hasDecipheredMsg){
      memcpy(&(msgInStage(PIPE_STG_DECIPHERED)), &currentElem, sizeof(decodingState_t));
      hasDecipheredMsg = true;
      processedMessage = false;
    }
    yield;
  }  
  osDelay(osWaitForever);
}
osThreadDef(thread_decipher, osPriorityNormal, 1, 0);

//Thread para testar penultimo digito verificador
void thread_test_1(void const *args){
  bool verifiedByte = false;
  decodingState_t currentElem;
  while(RUNNING){
    if(!verifiedByte && hasDecipheredMsg){
      memcpy(&currentElem, &(msgInStage(PIPE_STG_DECIPHERED)), sizeof(decodingState_t));
      test1Loaded = true;
      if(test2Loaded){
        hasDecipheredMsg = false;
        test1Loaded = false;
        test2Loaded = false;
      }
      //---
      currentElem.firstTestResult = verifyFirstTest(&currentElem);
      //---
      currentElem.hasFirstTest = true;
      verifiedByte = true;
    } 
    if(verifiedByte && !hasVerifiedTest1){
      /** Perigo de concorrencia
      currentElem.secondTestResult = msgInStage(PIPE_STG_VERIFIED).secondTestResult;
      currentElem.hasSecondTest = msgInStage(PIPE_STG_VERIFIED).hasSecondTest;

      memcpy(&(msgInStage(PIPE_STG_VERIFIED)), &currentElem, MSG_SIZE);
      **/
      decodingState_t* msgTestState = &(msgInStage(PIPE_STG_VERIFIED));
      msgTestState->key = currentElem.key;
      msgTestState->prevPrime = currentElem.prevPrime;
      msgTestState->hasKey = currentElem.hasKey;
      memcpy(msgTestState->deciphered_msg, currentElem.deciphered_msg, MSG_SIZE);
      msgTestState->hasMsg = currentElem.hasMsg;
      msgTestState->firstTestResult = currentElem.firstTestResult;
      msgTestState->hasFirstTest = currentElem.hasFirstTest;
      
      hasVerifiedTest1 = true;
      verifiedByte = false;
    }
    yield;
  }
  osDelay(osWaitForever);
}
osThreadDef(thread_test_1, osPriorityNormal, 1, 0);

//Thread para testar ultimo digito verificador
void thread_test_2(void const *args){
  bool verifiedByte = false;
  decodingState_t currentElem;
  while(RUNNING){
    if(!verifiedByte && hasDecipheredMsg){
      memcpy(&currentElem, &(msgInStage(PIPE_STG_DECIPHERED)), sizeof(decodingState_t));
      test2Loaded = true;
      if(test1Loaded){
        hasDecipheredMsg = false;
        test1Loaded = false;
        test2Loaded = false;
      }
      //---
      currentElem.secondTestResult = verifySecondTest(&currentElem);      
      //---
      currentElem.hasSecondTest = true;
      verifiedByte = true;
    } 
    if(verifiedByte && !hasVerifiedTest2){
      /** Perigo de concorrencia
      currentElem.firstTestResult = msgInStage(PIPE_STG_VERIFIED).firstTestResult;
      currentElem.hasFirstTest = msgInStage(PIPE_STG_VERIFIED).hasFirstTest;

      memcpy(&(msgInStage(PIPE_STG_VERIFIED)), &currentElem, MSG_SIZE);
      **/
      decodingState_t* msgTestState = &(msgInStage(PIPE_STG_VERIFIED));
      msgTestState->key = currentElem.key;
      msgTestState->prevPrime = currentElem.prevPrime;
      msgTestState->hasKey = currentElem.hasKey;
      memcpy(msgTestState->deciphered_msg, currentElem.deciphered_msg, MSG_SIZE);
      msgTestState->hasMsg = currentElem.hasMsg;
      msgTestState->secondTestResult = currentElem.secondTestResult;
      msgTestState->hasSecondTest = currentElem.hasSecondTest;
      
      hasVerifiedTest2 = true;
      verifiedByte = false;
    }
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
  //Enquanto não houver uma falha critica e uma chave valida não for encontrada,
  //execute...
  while(RUNNING){
    if(!waitingValidation){
      //As Threads Test 1 e Test 2 devem ter terminado de processar uma
      //instancia de decodificacao para poder imprimir resultado
      if(!(hasVerifiedTest1 && hasVerifiedTest2)){
        yield;
        continue;
      }
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
    if(waitingValidation){
      if(hasValidated){
        waitingValidation = false;
        hasValidated = false;
      }
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
  bool waitingPrinting = false;
  bool isValid = false;
  decodingState_t currentElem;
  while(RUNNING){
    if(!waitingPrinting && hasVerifiedTest1 && hasVerifiedTest2){
      memcpy(&currentElem, &(msgInStage(PIPE_STG_VERIFIED)), sizeof(decodingState_t));
      
      validationLoaded = true;
      if(printLoaded){
          hasVerifiedTest1 = false;
          hasVerifiedTest2 = false;
          printLoaded = false;
          validationLoaded = false;
      }
      
      isValid = validateKey(&currentElem);
      
      waitingPrinting = true;
      hasValidated = true;
    }
    if(waitingPrinting && hasPrinted){
      foundValidKey = isValid;
      waitingPrinting = false;
      hasPrinted = false;
    }
    yield;
  }
  osDelay(osWaitForever);
}
osThreadDef(thread_validate, osPriorityNormal, 1, 0);

//************************
//Código da thread Main
//************************
void thread_main(){
  while(RUNNING) yield;
  printf("finished");
}

int main(int n_args, char** args){
  //Inicialização de Kernel vai aqui
  osKernelInitialize();
  
  //************************
  //Inicialização de Threads aqui
  //************************
  id_thread_generate =  osThreadCreate(osThread(thread_generate),       NULL);
  id_thread_decipher =  osThreadCreate(osThread(thread_decipher),       NULL);
  id_thread_test_1 =    osThreadCreate(osThread(thread_test_1),         NULL);
  id_thread_test_2 =    osThreadCreate(osThread(thread_test_2),         NULL);
  id_thread_print =     osThreadCreate(osThread(thread_print),          NULL);
  id_thread_validate =  osThreadCreate(osThread(thread_validate),       NULL);
  
  //************************
  //Fim de inicializações de Threads
  //************************
  
  //Início do SO
  osKernelStart();
  
  //Main thread
  thread_main();

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