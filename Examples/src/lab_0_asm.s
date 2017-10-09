  EXTERN  translate_x, translate_y
  EXTERN  lut_sin, lut_cos
  EXTERN  zoom, rotation, invert_color
  
  // Nome:              __aeabi_i2f
  // Descrição:         Converte int para float
  // Entrada:           r0 [int]
  // Saida:             r0 [float]
  // Destroi:           r1 e r2
  IMPORT __aeabi_i2f  

  // Nome:              __aeabi_f2iz
  // Descrição:         Converte float para int
  // Entrada:           r0 [float]
  // Saida:             r0 [int]
  // Destroi:           r1 e r2
  IMPORT __aeabi_f2iz
  
  // Nome:              __aeabi_fadd
  // Descrição:         Soma dois floats 
  // Entradas:          r0 [float] e r1 [float]
  // Saida:             r0 [float]
  // Destroi:           r1, r2
  IMPORT __aeabi_fadd

  // Nome:              __aeabi_fsub
  // Descrição:         Subtração de dois floats 
  // Entradas:          r0 [float] e r1 [float]
  // Saida:             r0 [float]
  // Destroi:           r1, r2
  IMPORT __aeabi_fsub
  
  // Nome:              __aeabi_fmul
  // Descrição:         Multiplica dois floats 
  // Entradas:          r0 [float] e r1 [float]
  // Saida:             r0 [float]
  // Destroi:           r1, r2 e r3
  IMPORT __aeabi_fmul
  
  #ifndef WDT
    #define WDT 96
  #endif

  #ifndef HGT
    #define HGT 64
  #endif
  
  #ifndef THRES
    #define THRES 127
  #endif

  SECTION .text : CODE (2)
  THUMB

//   Nome:              transform_coordinates
//   Descrição:         aplica uma matrix de transformação a coordenadas x, y
//   Entrada:           r0 e r1 - referencias para variavel x e y
//   Globais:           translate_x e translate_y - valores de translação x e y
//                      zoom - valor de magnificação
//                      rotation - valor de rotação (indice das LUTs)
//                      lut_sin e lut_cos - LUTs de seno e cosseno
//   Saida:             r0 e r1 - mesma referencia x e y de entrada 
//                      com valores atualizados
  PUBLIC transform_coordinates  
transform_coordinates:
  push  {r2- r7, r12}           // guardando contexto anterior
  push  {r0, r1, lr}            // guardando referencias dos parametros: >{r0, r1, lr} = &x, &y 
                                // ponteiro de retorno de instrução
  ldr   r3, [r0]                // r3 <- *(&x) = x
  ldr   r4, [r1]                // r4 <- *(&y) = y

  // Calculo da translação pela matriz T das coodernadas centralizadas
  // Translação em x e y
  ldr   r5, =translate_x        // r5 <- &translate_x (ver .c)
  ldr   r6, =translate_y        // r6 <- &translate_y (ver .c)
  ldr   r1, [r5]                // r1 <- *(&translate_x) = translate_x
  ldr   r2, [r6]                // r2 <- *(&translate_y) = translate_y
  add   r5, r1, #WDT/2          // r5 <- translate_x + WDT/2
  add   r6, r2, #HGT/2          // r6 <- translate_y + HGT/2
  sub   r0, r3, r5              // r0 <- x_translated = x - (translate_x + WDT/2)
  bl    __aeabi_i2f             // r0 <- (float) x_translated
  mov   r5, r0                  // r5 <- x_translated
  sub   r0, r4, r6              // r0 <- y_translated = y - (translate_y + HGT/2)
  bl    __aeabi_i2f             // r0 <- (float) y_translated
  mov   r4, r0                  // r4 <- y_translated
    
  // Calculo da rotação pela matriz R
  // Recuperando seno e cosseno 
  ldr   r1, =rotation           // r1 <- &rotation              
  ldr   r2, =lut_sin            // r2 <- lut_sin[]
  ldrb  r6, [r1]                // r6 <- *(&rotation) = rotation
  ldr   r3, =lut_cos            // r3 <- lut_cos[]
  ldr   r1, [r2, r6, LSL #2]    // r1 <- lut_sin[rotation] (deslocamento de 4 para alinhar memoria
  ldr   r7, [r3, r6, LSL #2]    // r7 <- lut_cos[rotation]
  mov   r6, r1                  // r6 <- lut_sin[rotation]

  // Resumo de conteudo dos registradores
  // r4 = y_translated (e r0 tambem)
  // r5 = x_translated
  // r6 = lut_sin[rotation] (e r1 tambem)
  // r7 = lut_cos[rotation] 
  
  // Encontrando rotação x
  bl    __aeabi_fmul            // r0 <- r0*r1 = y_translated*lut_sin[rotation]
  push  {r0}                    // >r0 = y_translated*lut_sin[rotation]
  mov   r0, r5                  // r0 <- x_translated
  mov   r1, r7                  // r1 <- lut_cos[rotation]
  bl    __aeabi_fmul            // r0 <- r0*r1 = x_translated*lut_cos[rotation]
  pop   {r1}                    // r1< = y_translated*lut_sin[rotation]
  // Coordenada x rotacionada
  bl    __aeabi_fadd            // r0 <- x_rotated = x_translated*lut_cos[rotation] + y_translated*lut_sin[rotation]
  push  {r0}                    // >r0 = x_rotated
  
  //Encontrando rotação y
  mov   r0, r5                  // r0 <- x_translated
  mov   r1, r6                  // r1 <- lut_sin[rotation]
  bl    __aeabi_fmul            // r0 <- r0*r1 = x_translated*lut_sin[rotation]
  mov   r6, r0                  // r6 <- x_translated*lut_sin[rotation]
  mov   r0, r4                  // r0 <- y_translated
  mov   r1, r7                  // r1 <- lut_cos[rotation]
  bl    __aeabi_fmul            // r0 <- r0*r1 = y_translated*lut_cos[rotation]
  mov   r1, r6                  // r1 <- y_translated*lut_cos[rotation]
  // Coordenada Y rotacionada
  bl    __aeabi_fsub            // r0 <- y_rotated = y_translated*lut_cos[rotation] - x_translated*lut_sin[rotation]        
  
  // Resumo de conteudo dos registradores
  // r0 = y_rotated 
  // 1º no stack = x_rotated
  
  //Calculo da matriz S de redimensionalização
  ldr   r2, =zoom               // r2 <- &zoom
  ldr   r1, [r2]                // r1 <- *(&zoom)        
  mov   r4, r1                  // r4 <- zoom
  bl    __aeabi_fmul            // r0 <- y_scaled = y_rotated*zoom   
  bl    __aeabi_f2iz            // r0 <- (int) y_scaled
  mov   r1, r4                  // r1 <- zoom
  mov   r4, r0                  // r4 <- y_scaled
  pop   {r0}                    // r0< = x_rotated
  bl    __aeabi_fmul            // r0 <- x_scaled = x_rotated*zoom
  bl    __aeabi_f2iz            // r0 <- (int) x_scaled
  
  //Calculo da matriz T' de translação (centralização)
  add   r3, r4, #HGT/2          // r3 <- y_transformed = y_scaled + HGT/2
  add   r2, r0, #WDT/2          // r4 <- x_transformed = x_scaled + WDT/2
  
  pop   {r0, r1, lr}            // restaurando ponteiros de x e y
  str   r2, [r0]                // gravando resultado de x_transf em x (por referência)
  str   r3, [r1]                // gravando resultado de y_transf em y (por referência)
  pop   {r2- r7, r12}           // restaurando contexto anterior à rotina
  bx lr                         //fim da rotina- retorno de função
  
//   Nome:              binarization
//   Descrição:         Utiliza um limiar para binarizar um pixel
//   Entrada:           r0 - valor de cor em escala de cinza [byte]
//   Globais:           invert_color - flag de inversão 
//                      (caso =1 inverte preenchimento) [byte]
//   Saida:             r0 - 1 se de ser preenchido, 0 caso contrario
//   Destroi:           r1
  PUBLIC binarization
binarization:
  uxtb  r0, r0                  // converte em byte
  cmp   r0, #THRES              // comparação com limiar
  blt _bin_is_black             // salto condicional (<limiar)
//_bin_is_white
  mov r0, #0                    // se for maior que limiar, é espaço vazio
  b _bin_finally                // continue a execução para a verificação de inversão
_bin_is_black:
  mov r0, #1                    // se for menor que limiar, é espaço preenchido
_bin_finally:
  ldr  r1, =invert_color        // carrega referencia de variavel de inversão
  ldrb  r1, [r1]                // carrega valor da variavel de inversão
  cmp   r1, #0                  // verifica se deve ser realizado inversão 
  beq   _bin_no_invert          // caso negativo (= 0), não realiza inversão
  cmp   r0, #0                  // compara resultado da limiarização
  bne   _bin_to_black           // caso resultado seja espaço vazio (= 0) converta para preenchido
//_bin_to_white
  mov   r0, #1                  // converte vazio para preenchido
_bin_no_invert:
  bx lr                         // fim da rotina- retorno de função
_bin_to_black:
  mov   r0, #0                  // converte preenchido para vazio
  bx lr                         // fim da rotina- retorno de função

  END
