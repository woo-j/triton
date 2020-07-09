/*
    triton - a Transam Triton emulator
    Copyright (C) 2020 Robin Stuart <rstuart114@gmail.com>

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:

    1. Redistributions of source code must retain the above copyright
       notice, this list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright
       notice, this list of conditions and the following disclaimer in the
       documentation and/or other materials provided with the distribution.
    3. Neither the name of the project nor the names of its contributors
       may be used to endorse or promote products derived from this software
       without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
    ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
    ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
    FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
    DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
    OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
    HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
    LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
    OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
    SUCH DAMAGE.
 */

/* Emulator for the Transam Triton
 * Created using only information available online - so expect bugs!
 * Central processor is Intel 8080A
 * VDU is based on Thomson-CSD chip (SFC96364) for which I can find no documentation
 * Most of the rest of the machine is 74 logic ICs
 * 
 * Only available ROMS (version 7.2) are currently hard-wired
 * ROM images thanks to Patrick B Warren at
 * https://sites.google.com/site/patrickbwarren/electronics/transam-triton
 * 
 * Developed for Linux using SFML library for I/O
 * 
 * Compile with:
 *      g++ -o triton ./triton.cpp 8080.cpp -lsfml-graphics -lsfml-window -lsfml-system
 */

#include <SFML/Graphics.hpp>
#include "8080.hpp"
#include <iostream>

class IOState {
public:
    int key_buffer;
    int led_buffer;
    int vdu_buffer;
    int port6;
    int port7;
    int tape_relay;
    int cursor_position;
    
    void vdu_strobe(State8080* state);
    void key_press(int key, int shifted, int ctrl);
};

void IOState::vdu_strobe(State8080* state) {
    // Takes input from port 5 buffer (IC 51) and attempts to duplicate
    // Thomson-CSF VDU controller (IC 61) interface with video RAM

    int i;
    int input = vdu_buffer & 0x7f;
    
    //std::cout << "VDU " << input << " cursor " << cursor_position << "\n";
    
    if (input == 0x08) {
        // Backspace
        if ((cursor_position % 64) != 0) {
            cursor_position--;
            state->memory[0x1000 + cursor_position] = 32;
        }
    }
    
    if (input == 0x09) {
        // Step cursor RIGHT
        if ((cursor_position % 64) < 63)
            cursor_position++;
    }
    
    if (input == 0x0a) {
        // Line feed - not sure what this does! (possibly just for printer support?)
    }
    
    if (input == 0x0b) {
        // Step cursor UP
        if (cursor_position >= 64) {
            cursor_position -= 64;
        }
    }
    
    if (input == 0x0c) {
        // Clear screen/reset cursor
        for (i = 0; i < 1024; i++) {
            state->memory[0x1000 + i] = 32;
        }
        cursor_position = 0;
    }
    
    if (input == 0x0d) {
        // Carriage return/clear line
        if (cursor_position % 64 == 0) {
            state->memory[0x1000 + cursor_position] = 32;
            cursor_position++;
        }
        
        while(cursor_position % 64 != 0) {
            state->memory[0x1000 + cursor_position] = 32;
            cursor_position++;
        }
        
        if (cursor_position >= 1024) {
            for (i = 0; i < 960; i++) {
                state->memory[0x1000 + i] = state->memory[0x1000 + i + 64];
            }
            for (i = 960; i < 1024; i++) {
                state->memory[0x1000 + i] = 32;
            }
            
            cursor_position -= 64;
        }
    }
    
    if (input == 0x1c) {
        // Reset cursor
        cursor_position = 0;
    }
    
    if (input == 0x1d) {
        // Carriage return (no clear)
        cursor_position += 64;
        cursor_position -= (cursor_position % 64);
        
        if (cursor_position >= 1024) {
            for (i = 0; i < 960; i++) {
                state->memory[0x1000 + i] = state->memory[0x1000 + i + 64];
            }
            for (i = 960; i < 1024; i++) {
                state->memory[0x1000 + i] = 32;
            }
            
            cursor_position -= 64;
        }
    }
    
    if ((input >= 0x20) && (input <= 0x5f)) {
        // Alphanumeric characters
        cursor_position++;
        
        if (cursor_position >= 1024) {
            for (i = 0; i < 960; i++) {
                state->memory[0x1000 + i] = state->memory[0x1000 + i + 64];
            }
            for (i = 960; i < 1024; i++) {
                state->memory[0x1000 + i] = 32;
            }
            
            cursor_position -= 64;
        }
        
        state->memory[0x1000 + cursor_position - 1] = input;
    }
    
}

void IOState::key_press(int key, int shifted, int ctrl) {
    // Handles keyboard input, placing data in port 0 (IC 49)
    // Assumes PC has UK keyboard - because that's all I have to test it with!
    
    //std::cout << "Key " << key << "\n";
    
    if (ctrl == 0) {
    
        if ((key >= 0) && (key <= 25)) {
            // Letters A - Z
            key_buffer = key + 225;
        }
        
        switch(key) {
            case 36: key_buffer = 155; break; // escape
            case 46: key_buffer = 219; break; // left bracket
            case 47: key_buffer = 221; break; // right bracket
            case 57: key_buffer = 160; break; // space
            case 58: key_buffer = 141; break; // carriage return
            case 59: key_buffer = 136; break; // backspace
        }
        
        if (shifted == 0) {
            // No shift
            if ((key >= 26) && (key <= 35)) {
                // Numbers 0 to 9
                key_buffer = key + 150;
            }
            
            switch (key) {
                case 48: key_buffer = 187; break; // semicolon
                case 49: key_buffer = 172; break; // comma
                case 50: key_buffer = 174; break; // stop
                case 51: key_buffer = 167; break; // quote
                case 52: key_buffer = 175; break; // slash
                case 53: key_buffer = 220; break; // backslash
                case 55: key_buffer = 189; break; // equal
                case 56: key_buffer = 173; break; // hyphen
            }
        } else {
            // Shift key pressed
            switch (key) {
                case 26: key_buffer = 169; break; // close brace
                case 27: key_buffer = 161; break; // exclamation
                case 28: key_buffer = 162; break; // double quote
                case 29: key_buffer = 163; break; // hash
                case 30: key_buffer = 164; break; // dollar
                case 31: key_buffer = 165; break; // percent
                case 32: key_buffer = 222; break; // carrat
                case 33: key_buffer = 166; break; // ampusand
                case 34: key_buffer = 170; break; // asterisk
                case 35: key_buffer = 168; break; // open brace
                case 48: key_buffer = 186; break; // colon
                case 49: key_buffer = 188; break; // less than
                case 50: key_buffer = 190; break; // greater than
                case 51: key_buffer = 192; break; // at
                case 52: key_buffer = 191; break; // question
                case 53: key_buffer = 252; break; // bar
                case 55: key_buffer = 171; break; // plus
                case 56: key_buffer = 223; break; // underscore
            }
        }
        
    } else {
        // Control characters
        if ((key >= 0) && (key <= 25)) {
            // CONT A - Z
            key_buffer = key + 129;
        }
        
        if (key == 51) key_buffer = 128; // control + at
        if (key == 53) key_buffer = 156; // control + backslash
        if (key == 46) key_buffer = 155; // control + left bracket
        if (key == 47) key_buffer = 157; // control + right bracket
    }
}

void MachineIN(State8080* state, int port, IOState* io) {
    // Handles port input (CPU IN)
    switch(port) {
        case 0:
            // Keyboard buffer (IC 49)
            state->a = io->key_buffer;
            io->key_buffer = 0;
            break;
    }
}

void MachineOUT(State8080* state, int port, IOState* io) {
    // Handles port output (CPU OUT)
    switch(port) {
        case 3:
            // LED buffer (IC 50)
            io->led_buffer = state->a;
            break;
        case 5:
            // VDU buffer (IC 51)
            io->vdu_buffer = state->a;
            if (state->a >= 0x80) {
                io->vdu_strobe(state);
            }
            break;
        case 6:
            // Port 6 latches (IC 52)
            io->port6 = state->a >> 6;
            break;
        case 7:
            // Port 7 latches (IC 52) and tape power switch (RLY 1)
            io->port7 = ((state->a & 0x40) != 0);
            io->tape_relay = ((state->a & 0x80) != 0);
            break;
    }
}

int main() {
    int main_memory[0xffff];
    
    FILE *rom;
    int time;
    int running_time;
    int i;
    IOState io;
    int xpos, ypos;
    
    int opcode;
    int port;
    int framerate = 25;
    int operations_per_frame;
    int opcount;
    int glyph;
    int inFocus = 1;
    int shifted = 0;
    int ctrl = 0;
    int pause = 0;
    
    // One microcycle is 1.25uS = effective clock rate of 800kHz
    operations_per_frame = 800000 / framerate;
    
    State8080 state;
    
    // Load Monitor 'A'
    rom = fopen("MONA72.ROM", "r");
    
    for (i = 0; i < 0x400; i++) {
        main_memory[i] = getc(rom);
    }
    
    fclose(rom);
    
    // Load Monitor 'B'
    rom = fopen("MONB72.ROM", "r");
    
    for (i = 0; i < 0x400; i++) {
        main_memory[i + 0xc00] = getc(rom);
    }
    
    fclose(rom);
    
    // Load BASIC 7.2
    rom = fopen("BASIC72.ROM", "r");
    
    for(i = 0; i < 0x2000; i++) {
        main_memory[i + 0xe000] = getc(rom);
    }
    
    fclose(rom);
    
    state.a = 0;
    state.pc = 0x00;
    state.memory = main_memory;
    
    // Initialise window
    sf::RenderWindow window(sf::VideoMode(512, 414), "Transam Triton");
    window.setFramerateLimit(framerate);
    
    sf::Texture fontmap;
    if (!fontmap.loadFromFile("font.png")) {
        std::cout << "Error loading font file";
    }
    
    sf::Sprite sprite[1024];
    sf::CircleShape led[8];
    sf::Color ledoff = sf::Color(50,0,0);
    sf::Color ledon = sf::Color(250,0,0);
    
    for (i = 0; i < 1024; i++) {
        sprite[i].setTexture(fontmap);
        ypos = ((i - (i % 8)) / 64) * 24;
        xpos = (i % 64) * 8;
        sprite[i].setPosition(sf::Vector2f((float) xpos,(float) ypos));
    }
    
    for (i = 0; i < 8; i++) {
        led[i].setRadius(7.0f);
        led[i].setPosition(15.0f + (i * 15), 386.0f);
    }

    while (window.isOpen())
    {
        sf::Event event;
        
        while (window.pollEvent(event))
        {
            // Close application on request
            if (event.type == sf::Event::Closed) {
                window.close();
            }
            
            // Don't react to keyboard input when not in focus
            if (event.type == sf::Event::LostFocus)
                inFocus = 0;
            
            if (event.type == sf::Event::GainedFocus)
                inFocus = 1;
            
            // Keep track of shift and control keys
            if (event.type == sf::Event::KeyPressed) {
                if ((event.key.code == sf::Keyboard::LShift) || (event.key.code == sf::Keyboard::RShift))
                    shifted = 1;
                
                if ((event.key.code == sf::Keyboard::LControl) || (event.key.code == sf::Keyboard::RControl))
                    ctrl = 1;
            }
            
            if (event.type == sf::Event::KeyReleased) {
                if ((event.key.code == sf::Keyboard::LShift) || (event.key.code == sf::Keyboard::RShift))
                    shifted = 0;
                
                if ((event.key.code == sf::Keyboard::LControl) || (event.key.code == sf::Keyboard::RControl))
                    ctrl = 0;
            }
                
            if (inFocus != 0) {
                // Respond to key presses
                if (event.type == sf::Event::KeyPressed)
                {
                    switch(event.key.code) {
                        case sf::Keyboard::F1:
                            // Reset button (PB 1)
                            state.pc = 0x0000;
                            state.int_enable = 0;
                            break;
                        case sf::Keyboard::F2:
                            // Clear Screen button (PB 2)
                            if (state.int_enable == 1) {
                                state.int_enable = 0;
                                main_memory[state.sp - 2] = state.pc & 0xff;
                                main_memory[state.sp - 1] = state.pc >> 8;
                                state.sp -= 2;
                                state.pc = 0x0008; // RST1
                            }
                            break;
                        case sf::Keyboard::F3:
                            // Initialise button (PB 3)
                            if (state.int_enable == 1) {
                                state.int_enable = 0;
                                main_memory[state.sp - 2] = state.pc & 0xff;
                                main_memory[state.sp - 1] = state.pc >> 8;
                                state.sp -= 2;
                                state.pc = 0x0010; // RST2
                            }
                            break;
                        case sf::Keyboard::F4:
                            // Pause button (PB 4)
                            if (pause == 1) {
                                pause = 0;
                            } else {
                                pause = 1;
                            }
                            break;
                        case sf::Keyboard::F9:
                            window.close();
                            break;
                        default:
                            io.key_press(event.key.code, shifted, ctrl);
                            break;
                    }
                }
            }
        }
        
        if (pause == 0) {
            // Send as many clock pulses to the CPU as would happen
            // between screen frames
            running_time = 0;
            while (running_time < operations_per_frame) {
                opcode = get_memory(&state, state.pc);
                port = get_memory(&state, state.pc + 1);

                if (opcode == 0xdb) { // IN
                    MachineIN(&state, port, &io);
                    state.pc += 2;
                    time = 10;
                } else if (opcode == 0xd3) { // OUT
                    MachineOUT(&state, port, &io);
                    state.pc += 2;
                    time = 10;
                } else { // All other operands
                    time = Emulate8080Op(&state);
                }
                
                running_time += time;
            }
            
            // Draw screen from VDU memory
            // Font texture acts as ROMs (IC 69 and 70)
            window.clear();
            for (i = 0; i < 1024; i++) {
                glyph = main_memory[0x1000 + i];
                xpos = (glyph % 16) * 8;
                ypos = ((glyph / 16) * 24);
                sprite[i].setTextureRect(sf::IntRect(xpos, ypos, 8, 24));
                window.draw(sprite[i]);
                //std::cout << std::hex << glyph << " ";
            }
            for (i = 0; i < 8; i++) {
                if ((io.led_buffer & (0xf0 >> i)) == 0) {
                    // note LEDs are on for "0"
                    led[i].setFillColor(ledon);
                } else {
                    led[i].setFillColor(ledoff);
                }
                window.draw(led[i]);
            }
            window.display();
            //std::cout << "\n";
        }
    }

    return 0;
}
