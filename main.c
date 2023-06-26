/*===================================CPEG222====================================
 * Program:		Project3.c
 * Authors: 	Justin Chan
 * Date: 		11/2/2020
 * Description: Project3.c is a guessing game that gives the player 20 seconds to guess
 * a number from 0-99. The user is first asked to choose from 1-4 players and then
 * deterministic or random. The players then have 20 seconds to guess the number if
 * the number is outside the range it will not be accepted. if the timer times out to 0
 * the next player goes. the player can also look at the range with the press of F on the keypad
 * when numbers are entered they are entered on the SSD. Once the number is guessed the LCD displays
 * the number was guessed and then the SSD is stopped and any button press triggers a new game.
 * Input: KeyPad Press
 * Output: SSD Countdown timer, LED player count, LCD output, SSD number entered
==============================================================================*/
/*------------------ Board system settings. PLEASE DO NOT MODIFY THIS PART ----------*/
#ifndef _SUPPRESS_PLIB_WARNING // suppress the plib warning during compiling
#define _SUPPRESS_PLIB_WARNING
#endif
#pragma config FPLLIDIV = DIV_2 // PLL Input Divider (2x Divider)
#pragma config FPLLMUL = MUL_20 // PLL Multiplier (20x Multiplier)
#pragma config FPLLODIV = DIV_1 // System PLL Output Clock Divider (PLL Divide by 1)
#pragma config FNOSC = PRIPLL   // Oscillator Selection Bits (Primary Osc w/PLL (XT+,HS+,EC+PLL))
#pragma config FSOSCEN = OFF    // Secondary Oscillator Enable (Disabled)
#pragma config POSCMOD = XT     // Primary Oscillator Configuration (XT osc mode)
#pragma config FPBDIV = DIV_2   // Peripheral Clock Divisor (Pb_Clk is Sys_Clk/8)
/*----------------------------------------------------------------------------*/

#include <xc.h>     //Microchip XC processor header which links to the PIC32MX370512L header
#include "config.h" // Basys MX3 configuration header
#include "lcd.h"
#include <stdio.h>
#include <p32xxxx.h>
#include <sys/attribs.h>
#include "led.h"
#include "btn.h"
#include "mic.h"
#include "songs.h"
#include "adc.h"

// Define Rows and Columns
#define R4 LATCbits.LATC3
#define R3 LATGbits.LATG7
#define R2 LATGbits.LATG8
#define R1 LATGbits.LATG9
#define C4 PORTCbits.RC2
#define C3 PORTCbits.RC1
#define C2 PORTCbits.RC4
#define C1 PORTGbits.RG6

// display arrays

// Global Variables
#define SYS_CLK 80000000
#define PB_CLK 40000000

#define FLASH_THRED 50000
unsigned char array[] = {18, 18, 18, 18}; // cleared, no value to show
int freq[] = {262, 294, 330, 349, 277, 311, 392, 440, 494, 523, 370, 415, 466};
int q = 0;
int note;
int display_value = 20;
int press = 0;
int LD_position = 0b0001;
int flag = 0;
int current_song = 0;
int total_songs = 3;
int buttonLockRight = 0;
int buttonLockLeft = 0;
float sinArray[] = {
    512,
    544,
    576,
    607,
    639,
    670,
    700,
    729,
    758,
    786,
    812,
    838,
    862,
    884,
    906,
    925,
    943,
    960,
    974,
    987,
    998,
    1007,
    1014,
    1019,
    1022,
    1023,
    1022,
    1019,
    1014,
    1007,
    998,
    987,
    974,
    960,
    943,
    925,
    906,
    884,
    862,
    838,
    812,
    786,
    758,
    729,
    700,
    670,
    639,
    607,
    576,
    544,
    512,
    479,
    447,
    416,
    384,
    353,
    323,
    294,
    265,
    237,
    211,
    185,
    161,
    139,
    117,
    98,
    80,
    63,
    49,
    36,
    25,
    16,
    9,
    4,
    1,
    0,
    1,
    4,
    9,
    16,
    25,
    36,
    49,
    63,
    80,
    98,
    117,
    139,
    161,
    185,
    211,
    237,
    265,
    294,
    323,
    353,
    384,
    416,
    447,
    479,
    512,
};

/* ------------------------ Forward Declarations---------------------------- */
// subroutines

void delay_ms(int ms);
void CNConfig();
void T2Setup();
void Oc1Setup();
/* -------------------------- Definitions------------------------------------ */
#define SYS_FREQ (80000000L) // 80MHz system clock
#define SECONDS_TIMEOUT 20
// states
enum state
{
    IDLE,
    SONG_PLAY,
    FREESTYLE_PIANO,
    PLAY_AND_RECORD,
    PLAYBACK_RECORDED
} mode;

int main(void)
{
    /*-------------------- Port and State Initialization -------------------------*/

    DDPCONbits.JTAGEN = 0; // Statement is required to use Pin RA0 as IO
    LED_Init();            // This function initializes the hardware involved in the LED module
                //  The pins corresponding to LEDs are initialized as digital outputs

    LCD_Init(); // Initializes the LCD module
    BTN_

        LED_SetGroupValue(0);
    // initial mode
    mode = SONG_PLAY;

    // rows as outputs
    TRISGbits.TRISG7 = 0;
    ANSELGbits.ANSG7 = 0;
    TRISGbits.TRISG8 = 0;
    ANSELGbits.ANSG8 = 0;
    TRISCbits.TRISC3 = 0;
    TRISGbits.TRISG9 = 0;
    ANSELGbits.ANSG9 = 0;

    // columns as inputs
    TRISGbits.TRISG6 = 1;
    ANSELGbits.ANSG6 = 0;
    TRISCbits.TRISC2 = 1;
    TRISCbits.TRISC4 = 1;
    TRISCbits.TRISC1 = 1;

    // Turn rows off
    R1 = 0;
    R2 = 0;
    R3 = 0;
    R4 = 0;

    Oc1Setup();
    // Calls timer2 config for interrupt
    T2Setup();
    // Calls CN config for interrupt
    CNConfig();

    TRISBbits.TRISB14 = 0;
    ANSELBbits.ANSB14 = 0;
    T2CONbits.ON = 0;
    RPB14R = 0xC;

    int j = PORTC;
    int k = PORTG;
    //
    while (1)
    {
        /*-------------------- Main logic and actions start --------------------------*/
        switch (mode)
        {
        // mode to configure players - SSD are off, LEDs are off, LCD displays initial message
        // configure the variables high and low to be 0-99, turn off timer 2
        case IDLE:

            T2CONbits.ON = 0;
            LED_SetGroupValue(0);
            LCD_WriteStringAtPos("8-Bit Music Box      ", 0, 0);
            LCD_WriteStringAtPos("Press Btn L/R/U/D", 1, 0);
            break;
        case SONG_PLAY:
            LCD_WriteStringAtPos("Clap to switch     ", 0, 0);
            LCD_WriteStringAtPos("Keypad D to end", 1, 0);
            LED_SetGroupValue(LD_position);
            break;
        case FREESTYLE_PIANO:
            LCD_WriteStringAtPos("Keypad Piano     ", 0, 0);
            LCD_WriteStringAtPos("Keypad D to end   ", 1, 0);
            LED_SetGroupValue(0);
            break;
        }
        if (mode == IDLE)
        {
            if (BTN_GetValue('L') && !buttonLockLeft)
            {
                buttonLockLeft = 1;
                mode = SONG_PLAY;
            }
            else if (BTN_GetValue('R') && !buttonLockRight)
            {
                buttonLockRight = 1;
                mode = FREESTYLE_PIANO;
            }
        }
        else if (mode == FREESTYLE_PIANO)
        {
            if (display_value == 0 || display_value == 1 || display_value == 2 || display_value == 3 || display_value == 4 || display_value == 5 || display_value == 7 || display_value == 8 || display_value == 9 || display_value == 10 || display_value == 12 || display_value == 14 || display_value == 15)
            {
                T2CONbits.ON = 1;
                OC1CONbits.ON = 1;
            }
        }

        if (buttonLockLeft && !BTN_GetValue('L'))
        {
            delay_ms(20);
            buttonLockLeft = 0;
        }
        else if (buttonLockLeft && !BTN_GetValue('R'))
        {
            delay_ms(20);
            buttonLockRight = 0;
        }
    }
    delay_ms(25);
}

/*--------------------- Action and logic end ---------------------------------*/

/* -----------------------------------------------------------------------------
**	delay_ms
**	Parameters:
**		ms - amount of milliseconds to delay (based on 80 MHz SSCLK)
**	Return Value:
**		none
**	Description:
**		Create a delay by counting up to counter variable
** -------------------------------------------------------------------------- */
void delay_ms(int ms)
{
    int i, counter;
    for (counter = 0; counter < ms; counter++)
    {
        for (i = 0; i < 1426; i++)
        {
        } // software delay 1 millisec
    }
}

/* -----------------------------------------------------------------------------
**	keyPad
**	Parameters:
**		void
**	Return Value:
**		none
**	Description:
**		An interrupt for the keyPad allows for input of the keyPad
** -------------------------------------------------------------------------- */

void __ISR(_CHANGE_NOTICE_VECTOR) keyPad(void)
{
    CNCONCbits.ON = 0;
    CNCONGbits.ON = 0;

    int i;
    for (i = 0; i < 1000; i++)
    {
    }
    unsigned char keyCurrState = (!C1 || !C2 || !C3 || !C4);

    if (!keyCurrState)
    {
        display_value = 20;
        press = 0;

        T2CONbits.ON = 0;
        OC1CONbits.ON = 0;
    }

    else if (!press)
    {
        press = 1;
        R1 = 0;
        R2 = 1;
        R3 = 1;
        R4 = 1;

        if (C1 == 0)
        {
            display_value = 1;
            note = freq[0];
            flag = 1;
        }

        else if (C2 == 0)
        {
            display_value = 2;
            note = freq[1];
            flag = 1;
        }

        else if (C3 == 0)
        {
            display_value = 3;
            note = freq[2];
            flag = 1;
        }

        else if (C4 == 0)
        {
            display_value = 10;
            note = freq[3];
            flag = 1;
        }

        R1 = 1;
        R2 = 0;

        if (C1 == 0)
        {

            // If button pressed was 4
            display_value = 4;
            note = freq[4];
            flag = 1;
        }
        if (C2 == 0)
        {

            // If button pressed was 5
            display_value = 5;
            note = freq[5];
            flag = 1;
        }
        else if (C3 == 0)
        {

            // If button pressed was 6
            display_value = 6;
            flag = 1;
        }
        else if (C4 == 0)
        {

            // If button pressed was 'B'
            display_value = 11;
            flag = 1;
        }

        R2 = 1;
        R3 = 0;

        if (C1 == 0)
        {
            // If button pressed was 7
            display_value = 7;
            note = freq[6];
            flag = 1;
        }
        else if (C2 == 0)
        {
            // If button pressed was 8
            display_value = 8;
            note = freq[7];
            flag = 1;
        }
        else if (C3 == 0)
        {
            // If button pressed was 9
            display_value = 9;
            note = freq[8];
            flag = 1;
        }
        else if (C4 == 0)
        {
            // If button pressed was 'C'
            display_value = 12;
            note = freq[9];
            flag = 1;
        }

        R3 = 1;
        R4 = 0;
        if (C1 == 0)
        {
            // If button pressed was 0
            display_value = 0;
            note = freq[10];
            flag = 1;
        }
        else if (C2 == 0)
        {
            // If button pressed was 'F'
            display_value = 15;
            note = freq[11];
            flag = 1;
        }
        else if (C3 == 0)
        {
            // If button pressed was 'E'
            display_value = 14;
            note = freq[12];
            flag = 1;
        }
        else if (C4 == 0)
        {
            // If button pressed was 'D'
            display_value = 13;
            flag = 1;
        }
        // done with checking if one key was pressed
        // reset
        R1 = 0;
        R2 = 0;
        R3 = 0;
        R4 = 0;
    }

    // PR2 = (int)(((float)(80*10000000))/(note*100)-1);
    // PR2 = (int)((float)(800000000/note/100) + 0.5) - 1;
    PR2 = PB_CLK / (1 * note * 101) - 1;
    int j = PORTC;
    int k = PORTG;
    IFS1bits.CNCIF = 0;
    IFS1bits.CNGIF = 0;
    CNCONCbits.ON = 1;
    CNCONGbits.ON = 1;
}
/* -----------------------------------------------------------------------------
**	CNConfig
**	Parameters:
**		none
**	Return Value:
**		none
**	Description:
**		Function to set up the CN interrupt
** -------------------------------------------------------------------------- */
void CNConfig()
{
    macro_disable_interrupts;
    CNCONCbits.ON = 1;
    CNCONGbits.ON = 1;

    CNENC = 0x16;
    CNPUC = 0x16;

    CNENG = 0x40;
    CNPUG = 0x40;

    IPC8bits.CNIP = 6;
    IPC8bits.CNIS = 2;

    IFS1bits.CNCIF = 0;
    IFS1bits.CNGIF = 0;

    IEC1bits.CNCIE = 1;
    IEC1bits.CNGIE = 1;

    macro_enable_interrupts();
}

/* -----------------------------------------------------------------------------
**	T2Setup
**	Parameters:
**		none
**	Return Value:
**		none
**	Description:
**		Function to set up the timer 2 interrupt
** -------------------------------------------------------------------------- */
void T2Setup()
{
    // macro_disable_interrupts;
    //  Timer2 pin/reg configuration
    PR2 = 79999;         // set period register, generates one interrupt every 1 s
    TMR2 = 0;            // initialize count to 0
    T2CONbits.TCKPS = 0; // 1:1 prescalevalue
    T2CONbits.TGATE = 0; // not gated input (the default)
    T2CONbits.TCS = 0;   // PBLK input (the default)
    // Timer2 interrupt configuration
    IPC2bits.T2IP = 7; // interrupt priority for Timer
    IPC2bits.T2IS = 3; // interrupt subpriority for Timer
    IEC0bits.T2IE = 1; // enable Timer2 interrupt for Timer
    IFS0bits.T2IF = 0; // clear Timer2 interrupt flag for Timer 2

    // macro_enable_interrupts();//enable interrupts at CPU
}
/* -----------------------------------------------------------------------------
**	TimerISR
**	Parameters:
**		none
**	Return Value:
**		none
**	Description:
**		function to countdown the timer
** -------------------------------------------------------------------------- */
void __ISR(_TIMER_2_VECTOR) TimerISR(void)
{
    // int i;

        OC1RS =(sinArray[q]*(PR2+1)))/1023;
        q++;
        q = q % 100;

        // OC1RS =(int)((float)(sinArray[i]*(PR2+1))/1023);
        IFS0bits.T2IF = 0;
}
void __ISR(_TIMER_4_VECTOR) Timer4ISR(void)
{
}

void Oc1Setup()
{
        // macro_disable_interrupts;
        //  OC1 pin/reg configuration
        // OC1CONbits.ON = 0;
        OC1CONbits.OCM = 6;
        OC1CONbits.OCTSEL = 0;
        OC1RS = PR2 / 2;
        OC1R = OC1RS;
        // macro_enable_interrupts();
}
