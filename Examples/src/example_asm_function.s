        PUBLIC f_asm
        EXTERN matriz

        #define figura 0x00001000
        
        SECTION .text : CODE (2)
        THUMB
f_asm:

        mov r0,#0xaa00
        mov r1,#0xbb
        orrs r1,r0
        
        ldr r0,=figura
        ldrb r2,[r0,#0]
        ldrb r2,[r0,#6]
        
        ldr r0,=matriz
        bx lr

        END
