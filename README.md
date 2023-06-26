# Project3.c

**Authors:** Justin Chan
**Date:** 11/2020

## Description

`Project3.c` is a guessing game that gives the player 20 secondes to guess a number from 0-99. The user is first asked to choose from 1-4 players and then deterministifc or random. The players then have 20 seconds to guess the number. If the number is outside the range, it will not be accepted. If the timer reaches 0, the next player goes. The player can also look at the range with the press of F on the keypad. When numbers are entered, they are displayed on the SSD. Once the number is guessed, the LCD displays the number that was guessed, and then the SSD is stopped. Any button press triggers a new game.

## Input

KeyPad Press

## Output

- SSD Countdown timer
- LED player count
- LCD output
- SSD number entered

## Dependencies

- Microchip XC32 compiler
- Basys MX3 configuration header (`config.h`)
- LCD library (`lcd.h`)
- SSD library (`ssd.h`)
- Button library (`btn.h`)
- LED library (`led.h`)
- Accelerometer library (`acl.h`)

# Instructions for Running the Program on Basys MX3 Board

## Prerequisites

- MPLAB X IDE installed on your system.
- Microchip XC32 compiler installed.
- Basys MX3 board from Digilent.
- USB cables for connecting the Basys MX# board to the computer.

## Steps

1. Connect your Basys MX3 board to your computer using the USB cable.
2. Launch MPLAB X IDE.
3. Create a new project in MPLAB X IDE.
4. Set the project configuration to target the Basys MX3 board.
   - Go to "Project" > "Properties".
   - Select the "Conf:[default]" configuration
   - Under "Categories" on the left, select "PIC32 Compiler" > "Optimizations".
   - Set the "Optimization Level" to "None"
   - Under "Categories on the left, select "PIC32 Assembler" > "General"
   - Set the "Assembler Options" to "-defsym \_HARDWARE_BOARD="PIC32MX"", including the quotes.
   - Click "OK" to save the configuration.
5. Add the necessary source files to the project:
   - Right-click on your project in the "Projects" pane.
   - Select "Add Existing items to Project".
   - Browse to the location of the source files(`Project3.c`, `config.h`, `lcd.h`, `ssd.h`, `btn.h`, `led.h`, `acl.h`), and select them.
   - Click "Add" to add the files to your project.
6. Add the necessary source files to the project:
   - Right-click on your project in the "Projects" pane.
   - Select "Build".
   - Wait for the build process to complete.
7. Configure the Basys MX3 board in MPLAB X IDE:
   - Go to "Tools" > "Embedded" > "Hardware Tool".
   - Select "MPLAB REAL ICE POWER" as the tool.
   - Click "Connect".
   - Select the appropriate device and interface.
   - Click "OK" to close the dialog.
8. Upload the compiled code to the Basys MX3 board:
   - Right-click on your project in the "Projects" pane.
   - Select "Set as Main Project".
   - Wait for the programming process to complete
9. Disconnect the Basys MX3 board from your computer.
10. Power on the Basys MX3 board using an external power source (if not already powered).
11. The program should start running on the Basys MX3 board, and you can interact with it using the keypad, SSD, LCD, LED, and accelerometer.
