/*****************************************************************************
 *   This example is playing tones using the the speaker
 *
 *   Copyright(C) 2009, Embedded Artists AB
 *   All rights reserved.
 *
 ******************************************************************************/


#include "mcu_regs.h"
#include "type.h"
#include "uart.h"
#include "stdio.h"
#include "timer32.h"
#include "gpio.h"



#define P1_2_HIGH() (LPC_GPIO1->DATA |= (0x1<<2))
#define P1_2_LOW()  (LPC_GPIO1->DATA &= ~(0x1<<2))


static uint32_t notes[] = {
        2272, // A - 440 Hz
        2024, // B - 494 Hz
        3816, // C - 262 Hz
        3401, // D - 294 Hz
        3030, // E - 330 Hz
        2865, // F - 349 Hz
        2551, // G - 392 Hz
        1136, // a - 880 Hz
        1012, // b - 988 Hz
        1912, // c - 523 Hz
        1703, // d - 587 Hz
        1517, // e - 659 Hz
        1432, // f - 698 Hz
        1275, // g - 784 Hz
};


static const char *songs[] = {
   "E2,E2,E4,E2,E2,E4,E2,G2,C2,D2,E8,F2,F2,F2,F2,F2,E2,E2,E2,E2,D2,D2,E2,D4,G4,E2,E2,E4,E2,E2,E4,E2,G2,C2,D2,E8,F2,F2,F2,F2,F2,E2,E2,E2,G2,G2,F2,D2,C8,",

   "D4,B4,B4,A4,A4,G4,E4,D4.D2,E4,E4,A4,F4,D8.D4,d4,d4,c4,c4,B4,G4,E4.E2,F4,F4,A4,A4,G8,",

   "G2,A2,G2,E8.G2,A2,G2,E8.d4.d2,B8,c4,c2,G8,A4,A2,c2,B2,A2,G2,A2,G2,E8,",

   "C1,D4,C2,C2,D4,C2,C2,F3,F1,E2,E2,F4.E1,D4,E2,F2,D4,E2,F2,G3,B1,G2,B2,G4.F1,E4,D2,C2,E4,D2,C2,D3,C1,C2,F2,A4.c1,B2,G2,A2,F2,G2,E2,F2,D2,C3,A1,C2,G2,F4,",

   "A1,B2,c2,A3,A1,A2,B2,A2,F2,G4,G2,A2,G2,E2,F4,F2,G2,F2,D2,E4,E2,F2,E2,C2,E4,E2,F2,A2,c2,d4.c1,B2,F2,B4,B2,c2,B2,E2,A4.",

   "E2,E2,E2,D2,E2,G2,B3,G1,E2,E2,G2,E2,E4,D2,D2,F2,F2,A2,A2,F2,F2,G3,G1,E2,E2,G2,E2,E4,D4,E2,E2,E2,D2,G2,A2,B4,c2,B2,A2,G2,F2,E2,G3,F1,F8,E4,",

   "A2,A1,B1,A2,F2,F1,G1,F2,E2,E1,F1,E1,C1,D1,E1,F1,G1,A2,A2,A1,B1,A2,E2,A1,B1,c2,d2,F2,G2,A4_D1,D1,D1,E1,F1,E1,D1,E1,F2,D2,E1,E1,E1,F1,G1,F1,E1,F1,G2,E2,F1,G1,A2,G1,F1,G1,A1,B2,A1,G1,A1,B1,c2,B1,A1,d2,d2,",
};

static void playNote(uint32_t note, uint32_t durationMs) {

    uint32_t t = 0;

    if (note > 0) {

        while (t < (durationMs*1000)) {
            P1_2_HIGH();
            delay32Us(0, note / 2);

            P1_2_LOW();
            delay32Us(0, note / 2);

            t += note;
        }

    }
    else {
        delay32Ms(0, durationMs);
    }
}

static uint32_t getNote(uint8_t ch)
{
    if (ch >= 'A' && ch <= 'G')
        return notes[ch - 'A'];

    if (ch >= 'a' && ch <= 'g')
        return notes[ch - 'a' + 7];

    return 0;
}

static uint32_t getDuration(uint8_t ch)
{
    if (ch < '0' || ch > '9')
        return 400;

    /* number of ms */

    return (ch - '0') * 200;
}

static uint32_t getPause(uint8_t ch)
{
    switch (ch) {
    case '+':
        return 0;
    case ',':
        return 5;
    case '.':
        return 20;
    case '_':
        return 30;
    default:
        return 5;
    }
}


static void playSong(uint8_t *song) {
    uint32_t note = 0;
    uint32_t dur  = 0;
    uint32_t pause = 0;

    /*
     * A song is a collection of tones where each tone is
     * a note, duration and pause, e.g.
     *
     * "E2,F4,"
     */

    while(*song != '\0') {
        note = getNote(*song++);
        if (*song == '\0')
            break;
        dur  = getDuration(*song++);
        if (*song == '\0')
            break;
        pause = getPause(*song++);

        playNote(note, dur);
        delay32Ms(0, pause);
    }
}


int main (void) {

    int i = 0;

    GPIOInit();
    init_timer32(0, 10);

    UARTInit(115200);
    UARTSendString((uint8_t*)"Speaker - Tone\r\n");

    GPIOSetDir( PORT1, 9, 1 );
    GPIOSetDir( PORT1, 10, 1 );

    GPIOSetDir( PORT3, 0, 1 );
    GPIOSetDir( PORT3, 1, 1 );
    GPIOSetDir( PORT3, 2, 1 );
    GPIOSetDir( PORT1, 2, 1 );

    LPC_IOCON->R_PIO1_2= (LPC_IOCON->R_PIO1_2 & ~0x7) | 0x01;

    GPIOSetValue( PORT3, 0, 0 );  //LM4811-clk
    GPIOSetValue( PORT3, 1, 0 );  //LM4811-up/dn
    GPIOSetValue( PORT3, 2, 0 );  //LM4811-shutdn


    for (i = 0; i < sizeof(songs)/ sizeof(uint8_t*); i++) {
        playSong((uint8_t*)songs[i]);
        delay32Ms(0, 3000);
    }

    while(1);


    return 0;
}