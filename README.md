# triton
A Transam Triton emulator

Emulates the 8080-based home-build computer designed by Mike Hughes and
released in the UK by Transam Microsystems Ltd. in partnership with
Electronics Today International in 1978.

This code is developed for Linux and displays to screen using the SFML
library.

Also required are a set of L7.2 ROM images, which are currently available at
https://sites.google.com/site/patrickbwarren/electronics/transam-triton

These need to be renamed as below and placed in working directory:

mon1  > MONA72.ROM

mon2  > MONB72.ROM

basic > BASIC72.ROM

Compile with the following command:

g++ -o triton ./triton.cpp 8080.cpp -lsfml-graphics -lsfml-window -lsfml-audio -lsfml-system

Keyboard Input:

Keyboard input currently assumes host has a UK keyboard.

Keyboard input is mapped to UK PC keys, so you get ampusand (&) with shift+7
rather than shift+6 on original keyboard. The exception to the rule is hash
(#) which can be found at shift+3. Most, but not all, keys are currently
supported. Control keys work as expected, but shift lock is not available.

Other keyboard funcions provided are:

* F1 - Hard reset (Reset signal sent directly to CPU)
* F2 - Clear screen (Interrupt 1)
* F3 - Initialise (Interrupt 2)
* F4 - Pause/Unpause
* F9 - Exit emulator

More information about the history of this machine available from Happy Little
Diodes at:

https://www.youtube.com/watch?v=0cSRgJ68_tM

This emulator is very much a work in progress...

Things which currently work:
* Emulation of 8080
* Output to screen using port 5 and correct fonts
* Output to LEDs using port 3
* Push buttons PB1-PB4 mapped to F1-F4
* Tape I/O to static file "TAPE"
* Writing/Running/Saving/Loading BASIC programs
* Audio 'Beep' on error

Things which kind of work:
* Keyboard emulation has only been tested with UK keyboard
* Accuracy is based on documentation rather than physical machine
* Fixed memory map based on L7.1 ROM documentation

Things which don't work (or haven't been tested) yet:
* Printer emulation
* Requires a Makefile

Also included are the following tapes:

MANUAL_MC_TAPE - All of the machine code program samples from the manual
MANUAL_BASIC_TAPE - All of the BASIC program samples from the manual

TRAIN_TAPE - Train graphics demonstration program

    "Although it is quite easy to use the VDU function
    within TRITON'S BASIC to produce moving graphics
    on the display screen you are limited to the speed with
    which movements can be made. This is due to the
    inefficiency of an interpreter program. Much better
    use of TRITON'S memory mapped VDU can be made
    with programs written in machine code."
    
    "This one gives a picture of a simple railway engine
    followed by a couple of trucks which move across the
    screen from right to left. As they leave the screen on
    the left they re-enter again from the right but one line
    up the screen. The process continues until the train
    reaches the left-hand side of the fifth line from the top
    of the screen and then the program repeats itself." 
    
    Hughes, M. (1979) From 'Softspot', Computing Today, Vol 1 No 4 (February 1979) pp. 38-41

CROSSHATCH_TAPE - Crosshatch Generator

    "This produces four of the most common crosshatch patterns
    on a TV set and allows easy selection of each type. The
    actual adjustment of a TV is fairly easy once the concergence
    controls have been found and a full explanation of the methods
    was given in ETI September 1978. The program runs
    in 1K on TRITON."
    
ETCH_A_SKETCH - Etch-A-Sketch

    "This program emulates the children's toy of the same name.
    The first character or graphic key pressed will cause the selected
    symbol to appear near the centre of the VDU screen.
    On pressing one of the keys U,D,L or R the symbol will
    move in the selected direction leaving a trail behind it. It
    should be noted that if the drawing moves off the screen
    you run the risk of corrupting the monitor. This program is
    for TRITON and runs in 1K."
    
CONFUSE_A_CAT - Confuse-A-Cat

    "This program is a modified version of the one in the TRITON
    manual. It alternates between filling and emptying the screen
    with characters. It is written in tiny BASIC and will run in
    the standard kit memory."
    
    Davidson, I. (1979) From 'Softspot Special', Computing Today, Vol 1 No (May 1979) pp. 33-35
    
Program versions on these "tapes" have been adapted to operate with L7.2

Any feedback welcome - just email me:

rstuart114-at-gmail.com
