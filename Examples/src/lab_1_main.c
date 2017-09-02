#include "mcu_regs.h"
#include "type.h"
#include "stdio.h"
#include "timer32.h"
#include "pca9532.h"
#include "cmsis_os.h"

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
  osThreadYield();
}
osThreadDef(thread_generate_key, osPriorityNormal, 1, 0);

//Thread para decifrar chaves
void thread_decipher_key(void const *args){
  osThreadYield();
}
osThreadDef(thread_decipher_key, osPriorityNormal, 1, 0);

//Thread para testar penultimo digito verificador
void thread_verify_first_test_digit(void const *args){
  osThreadYield();
}
osThreadDef(thread_verify_first_test_digit, osPriorityNormal, 1, 0);

//Thread para testar ultimo digito verificador
void thread_verify_second_test_digit(void const *args){
  osThreadYield();
}
osThreadDef(thread_verify_second_test_digit, osPriorityNormal, 1, 0);

//Thread para escrever na saida chave gerada
void thread_write_key(void const *args){
  osThreadYield();
}
osThreadDef(thread_write_key, osPriorityNormal, 1, 0);

//Thread para testar ultimo digito verificador
void thread_validate_key(void const *args){
  osThreadYield();
}
osThreadDef(thread_validate_key, osPriorityNormal, 1, 0);

//************************
//Código da thread Main
//************************
void thread_main(){
  osThreadYield();
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