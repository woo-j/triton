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

g++ -o triton ./triton.cpp 8080.cpp -lsfml-graphics -lsfml-window -lsfml-system

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
* Most of the functions of the Monitor
* Entering programs using the Monitor and running them (options P and G)
* Push buttons PB1-PB4 mapped to F1-F4
* Tape I/O to static file "TAPE"

Things which kind of work:
* Keyboard emulation has only been tested with UK keyboard
* Not all VDU control codes work
* Accuracy is based on documentation rather than physical machine
* Fixed memory map based on L7.1 ROM documentation

Things which don't work yet:
* BASIC gets stuck in an endless loop
* Printer emulation
* No Transam documentation included
* Requires a Makefile

Any feedback welcome - just email me:

rstuart114-at-gmail.com
