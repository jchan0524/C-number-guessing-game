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
#pragma config FPBDIV = DIV_8   // Peripheral Clock Divisor (Pb_Clk is Sys_Clk/8)
/*----------------------------------------------------------------------------*/

#include <xc.h>     //Microchip XC processor header which links to the PIC32MX370512L header
#include "config.h" // Basys MX3 configuration header
#include "lcd.h"
#include <stdio.h>
#include <p32xxxx.h>
#include <sys/attribs.h>
#include <time.h>
#include "ssd.h"
#include "btn.h"
#include "led.h"
#include "acl.h"

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
#define FLASH_THRED 50000
unsigned char array[] = {18, 18, 18, 18}; // cleared, no value to show
int display_value;
int press = 0;
int total_sec = 20;
int input = 0, input1 = 0;
int LD_position = 0b0001;
int flag = 0;
char strMsg[80];
int new_calculate = 0;
int num_players = 0, current_player = 0;
int showing_hint = 0;
int secret_num, hardcoded_secret_num = 33;
int low = 0, high = 99;

int count = 0;
/* ------------------------ Forward Declarations---------------------------- */
// subroutines

void delay_ms(int ms);
void CNConfig();
void GuessingGame(int number);
void SSDShowRange();
void T2Setup();

/* -------------------------- Definitions------------------------------------ */

#define SYS_FREQ (80000000L) // 80MHz system clock
#define SECONDS_TIMEOUT 20
// states
enum state
{
    CONFIG_PLAYERS,
    CONFIG_SECRET,
    GUESSING,
    CONFIG_RANGE,
    GAMEOVER
} mode;

/* ----------------------------- Main --------------------------------------- */

int main(void)
{
    /*-------------------- Port and State Initialization -------------------------*/

    DDPCONbits.JTAGEN = 0; // Statement is required to use Pin RA0 as IO
    LED_Init();            // This function initializes the hardware involved in the LED module
                //  The pins corresponding to LEDs are initialized as digital outputs
    SSD_Init(); // Initializes the SSD module
    LCD_Init(); // Initializes the LCD module
    ACL_Init(); // Initializes the Accelerometer module

    // Seeds a random number
    float rgACLGVals[3];
    ACL_ReadGValues(rgACLGVals);
    int seed = rgACLGVals[0] * 10000;
    srand((unsigned)seed);
    // Turns SSD and LEDs off to start the program
    SSD_WriteDigits(array[0], array[1], array[2], array[3], 0, 0, 0, 0);
    LED_SetGroupValue(0);
    // initial mode
    mode = CONFIG_PLAYERS;

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

    // Calls CN config for interrupt
    CNConfig();
    // Calls timer2 config for interrupt
    T2Setup();

    int j = PORTC;
    int k = PORTG;

    while (1)
    {
        /*-------------------- Main logic and actions start --------------------------*/
        switch (mode)
        {
        // mode to configure players - SSD are off, LEDs are off, LCD displays initial message
        // configure the variables high and low to be 0-99, turn off timer 2
        case CONFIG_PLAYERS:

            T2CONbits.ON = 0;
            SSD_WriteDigits(array[0], array[1], array[2], array[3], 0, 0, 0, 0);
            LED_SetGroupValue(0);
            LCD_WriteStringAtPos("Num Guessing      ", 0, 0);
            LCD_WriteStringAtPos("# Players? [1-4]", 1, 0);
            high = 99;
            low = 0;

            break;

            // this mode displays the number of players and asks which type of game to be played
        case CONFIG_SECRET:
            sprintf(strMsg, "Num Guessing-%d", num_players);
            LCD_WriteStringAtPos(strMsg, 0, 0);
            LCD_WriteStringAtPos("Rand(E),Det(D)?      ", 1, 0);

            break;

            // this mode turns on the timer 2 and turns on the LED based on which player's turn it is
            // Calls the delay function to keep the SSD from flickering.
            // The SSD displays the numbers input in array[0] and array[1] and the timer counting down in total_sec%10 and total_sec/10
        case GUESSING:
            T2CONbits.ON = 1;
            LED_SetGroupValue(LD_position);
            delay_ms(50);
            SSD_WriteDigits(array[0], array[1], total_sec % 10, total_sec / 10, 0, 0, 0, 0);

            break;

        case CONFIG_RANGE:
            // this mode is to show the range of the game - it delays 50ms then it calls SSDShowRange to display the range
            // it also displays showing range on the SSD
            delay_ms(50);
            SSDShowRange();
            LED_SetGroupValue(LD_position);
            LCD_WriteStringAtPos("Showing Range....     ", 1, 0);

            break;

        case GAMEOVER:
            // in mode GAMEOVER - the timer is turned off and the position of the LED is set back to LED0;
            T2CONbits.ON = 0;
            LD_position = 0b0001;

            break;
        }

        if (flag == 1)
        {
            if (mode == CONFIG_PLAYERS)
            {
                if (display_value == 1 || display_value == 2 || display_value == 3 || display_value == 4)
                {
                    num_players = display_value;
                    // flag =0;
                    mode = CONFIG_SECRET;
                }
            }

            else if (mode == CONFIG_SECRET)
            {
                if (display_value == 13)
                {
                    LCD_WriteStringAtPos("Deterministic     ", 1, 0);
                    secret_num = hardcoded_secret_num;
                    display_value = 0;
                    array[0] = 0;
                    flag = 0;
                    mode = GUESSING;
                }

                else if (display_value == 14)
                {
                    LCD_WriteStringAtPos("Random Secret     ", 1, 0);
                    secret_num = rand() % 100;
                    display_value = 0;
                    array[0] = 0;
                    flag = 0;
                    mode = GUESSING;
                }
            }

            else if (mode == GUESSING)
            {
                if (display_value == 14 && high >= new_calculate && low <= new_calculate && total_sec > 0)
                {
                    GuessingGame(secret_num);
                    current_player++;
                    total_sec = 20;
                    T2CONbits.ON = 0;
                    flag = 0;
                    if (current_player == num_players)
                    {
                        current_player = 0;
                        LD_position = 0b0001;
                    }
                    else
                    {
                        LD_position <<= 1;
                    }
                }

                else if (display_value == 14 && (high > new_calculate || low < new_calculate))
                {
                    LCD_WriteStringAtPos("Out Of Range!     ", 1, 0);
                    input = 0;
                    input1 = 0;
                    array[0] = 0;
                    array[1] = 18;
                    display_value = 20;
                }

                else if (array[0] == 0 && display_value >= 0 && display_value <= 9)
                {
                    input = 0;
                    input = display_value;
                    array[0] = display_value;
                    flag = 0;
                }

                else if (array[1] == 18 && array[3] != 0 && display_value >= 0 && display_value <= 9)
                {
                    input1 = 0;
                    input1 = input * 10;
                    array[1] = input;
                    input = display_value;
                    array[0] = display_value;
                    flag = 0;
                }

                else if (array[1] != 18 && display_value >= 0 && display_value <= 9)
                {
                    input1 = 0;
                    input = 0;
                    input = display_value;
                    array[1] = array[0];
                    array[0] = display_value;
                    input1 = array[1] * 10;
                    flag = 0;
                }
                else if (display_value == 13)
                {
                    if (array[1] != 18)
                    {
                        array[0] = array[1];
                        array[1] = 18;
                        input = input1 / 10;
                        input1 = 0;
                        flag = 0;
                    }

                    else if (array[1] == 18)
                    {
                        array[0] = 0;
                        input = 0;
                        flag = 0;
                    }
                }

                else if (display_value == 15)
                {
                    flag = 0;
                    mode = CONFIG_RANGE;
                }
                else if (display_value == 12)
                {
                    array[0] = 0;
                    input = 0;
                    input1 = 0;
                    array[1] = 18;
                }
                new_calculate = input + input1;
            }

            else if (mode == CONFIG_RANGE)
            {
                if (display_value == 15)
                {
                    flag = 0;
                    mode = GUESSING;
                }
            }
            if (mode == GAMEOVER)
            {
                if (display_value != 18)
                {
                    flag = 0;
                    input = 0;
                    input1 = 0;
                    array[0] = 18;
                    array[1] = 18;
                    mode = CONFIG_PLAYERS;
                }
            }
        }

        if (total_sec == 0)
        {
            input = 0;
            input1 = 0;
            array[1] = 18;
            array[0] = 0;
            current_player++;
            total_sec = 20;
            if (current_player == num_players)
            {
                current_player = 0;
                LD_position = 0b0001;
            }
            else
            {
                LD_position <<= 1;
            }
            mode = GUESSING;
        }
    }

    /*--------------------- Action and logic end ---------------------------------*/
}
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
        press = 0;
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
            flag = 1;
        }

        else if (C2 == 0)
        {
            display_value = 2;
            count++;
            flag = 1;
        }

        else if (C3 == 0)
        {
            display_value = 3;
            flag = 1;
        }

        else if (C4 == 0)
        {
            display_value = 10;
            flag = 1;
        }

        R1 = 1;
        R2 = 0;

        if (C1 == 0)
        {

            // If button pressed was 4
            display_value = 4;
            flag = 1;
        }
        if (C2 == 0)
        {

            // If button pressed was 5
            display_value = 5;
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
            flag = 1;
        }
        else if (C2 == 0)
        {
            // If button pressed was 8
            display_value = 8;
            flag = 1;
        }
        else if (C3 == 0)
        {
            // If button pressed was 9
            display_value = 9;
            flag = 1;
        }
        else if (C4 == 0)
        {
            // If button pressed was 'C'
            display_value = 12;
            flag = 1;
        }

        R3 = 1;
        R4 = 0;
        if (C1 == 0)
        {
            // If button pressed was 0
            display_value = 0;
            flag = 1;
        }
        else if (C2 == 0)
        {
            // If button pressed was 'F'
            display_value = 15;
            flag = 1;
        }
        else if (C3 == 0)
        {
            // If button pressed was 'E'
            display_value = 14;
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

    int j = PORTC;
    int k = PORTG;
    IFS1bits.CNCIF = 0;
    IFS1bits.CNGIF = 0;
    CNCONCbits.ON = 1;
    CNCONGbits.ON = 1;
}

/* -----------------------------------------------------------------------------
**	GussingGame
**	Parameters:
**		number - the secret number to be guessed
**	Return Value:
**		none
**	Description:
**		Helper function to control the logic of calculating what happends when the guessed number is to high, low or equal to the number to be guessed
** -------------------------------------------------------------------------- */
void GuessingGame(int number)
{

    if (number < new_calculate)
    {
        LCD_WriteStringAtPos("Too High!          ", 1, 0);
        high = new_calculate - 1;

        array[0] = 0;
        array[1] = 18;
    }
    else if (number > new_calculate)
    {
        LCD_WriteStringAtPos("Too Low!            ", 1, 0);
        low = new_calculate + 1;
        array[0] = 0;
        array[1] = 18;
    }
    else if (number == new_calculate)
    {
        LCD_WriteStringAtPos("You Got It!!!             ", 1, 0);
        display_value = 18;
        mode = GAMEOVER;
    }
}
/* -----------------------------------------------------------------------------
**	SSDShowRange
**	Parameters:
**		none
**	Return Value:
**		none
**	Description:
**		Sets up the logic of how the range is supposed to behave with only 2 digits appearing if there is a ones and tens place are filled and one digit if just the ones place is filled
** -------------------------------------------------------------------------- */
void SSDShowRange()
{
    if (high / 10 == 0 && low / 10 == 0)
    {
        SSD_WriteDigits(high % 10, 18, low % 10, 18, 0, 0, 0, 0);
    }
    else if (high / 10 == 0)
    {
        SSD_WriteDigits(high % 10, 18, low % 10, low / 10, 0, 0, 0, 0);
    }
    else if (low / 10 == 0)
    {
        SSD_WriteDigits(high % 10, high / 10, low % 10, 18, 0, 0, 0, 0);
    }
    else
        SSD_WriteDigits(high % 10, high / 10, low % 10, low / 10, 0, 0, 0, 0);
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

    IPC8bits.CNIP = 5;
    IPC8bits.CNIS = 2;

    IFS1bits.CNCIF = 0;
    IFS1bits.CNGIF = 0;

    IEC1bits.CNCIE = 1;
    IEC1bits.CNGIE = 1;

    macro_enable_interrupts();
}

// void CNConfig(){
//     macro_disable_interrupts;
//     CNCONDbits.ON = 1;
//
//     CNEND = 0xf00;
//     CNPUD = 0xf00;
//
//     IPC8bits.CNIP=5;
//     IPC8bits.CNIS = 2;
//
//     IFS1bits.CNDIF =0;
//     IEC1bits.CNDIE =1;
//     macro_enable_interrupts();
// }

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
    PR2 = 39216; // set period register, generates one interrupt every 1 s
    TMR2 = 0;    // initialize count to 0
    T2CON = 0x8030;

    T2CONbits.TCKPS = 7; // 1:256 prescale value
    T2CONbits.TGATE = 0; // not gated input (default)
    T2CONbits.TCS = 0;   // PCBLK input (default)
    T2CONbits.ON = 1;    // turn on Timer2
    IPC2bits.T2IP = 7;   // priority
    IPC2bits.T2IS = 3;   // subpriority
    IFS0bits.T2IF = 0;   // clear interrupt flag
    IEC0bits.T2IE = 1;   // enable interrupt

    macro_enable_interrupts(); // enable interrupts at CPU
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
    total_sec--;
    IFS0bits.T2IF = 0;
}