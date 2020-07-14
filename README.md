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
* Monitor options P J E R G A D H L I O W M V N X
* Push buttons PB1-PB4 mapped to F1-F4
* Tape I/O to static file "TAPE"
* Writing/Running/Saving/Loading BASIC programs
* Audio 'Beep' on error

Things which kind of work:
* Keyboard emulation has only been tested with UK keyboard
* Not all VDU control codes work
* Accuracy is based on documentation rather than physical machine
* Fixed memory map based on L7.1 ROM documentation

Things which don't work (or haven't been tested) yet:
* Monitor options C U F T Z K untested
* Printer emulation
* No Transam documentation included
* Requires a Makefile

Also included are the following tapes:

MANUAL_MC_TAPE - All of the machine code program samples from the manual
MANUAL_BASIC_TAPE - All of the BASIC program samples from the manual

Program versions on these "tapes" have been adapted to operate with L7.2

Any feedback welcome - just email me:

rstuart114-at-gmail.com
