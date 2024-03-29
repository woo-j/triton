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
 * VDU is based on Thomson-CSD chip (SFC96364)
 * Most of the rest of the machine is 74 logic ICs
 * 
 * Only version 7.2 ROM (default) is currently tested, but experimental mechanism is in place for using alternative ROMs
 */

#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include "8080.hpp"
#include <iostream>
#include <fstream>
#include <cmath>
#include <cstring>
using namespace std;

class IOState {
public:
    int  key_buffer;
    int  led_buffer;
    int  vdu_buffer;
    int  port6;
    bool oscillator;
    bool tape_relay;
    int  cursor_position;
    int  tape_status;
    int  uart_status;
    int  vdu_startrow;
    
    void vdu_strobe(State8080* state);
  void key_press(sf::Event::EventType event, int key, bool shifted, bool ctrl);
};

void IOState::vdu_strobe(State8080* state) {
    // Takes input from port 5 buffer (IC 51) and attempts to duplicate
    // Thomson-CSF VDU controller (IC 61) interface with video RAM
    // Updated with information from the manufacturer's datasheet thanks to Gerald Sommariva

    int i;
    int input = vdu_buffer & 0x7f;
    int vdu_page = 0x1000;
    
    switch(input) {
        case 0x00:
            // NUL
            break;
        case 0x04:
            // EOT (End of Text)
            break;
        case 0x08:
            // Cursor left / Retour d'une position à gauche
            cursor_position--;
            if (cursor_position < 0) {
                cursor_position += 1024;
            }
            break;
        case 0x09:
            // Cursor right / Retour d'une position à droite
            cursor_position++;
            if (cursor_position >= 1024) {
                cursor_position -= 1024;
            }
            break;
        case 0x0a:
            // Cursor down (erased next line) / Descente d'une position (Ligne suivante du texte effacée)
            cursor_position += 64;
            if (cursor_position >= 1024) {
                cursor_position -= 64;
                vdu_startrow++;
                if (vdu_startrow > 15) {
                    vdu_startrow = 0;
                }
                
                for (i = 0; i < 64; i++) {
                    state->memory[vdu_page + (((64 * vdu_startrow) + cursor_position + i) % 1024)] = 32;
                }
            }
            break;
        case 0x0b:
            // Cursor up / Montée d'une position
            cursor_position -=64;
            if (cursor_position < 0) {
                cursor_position += 1024;
            }
            break;
        case 0x0c:
            // Page clear and home position / Effacement de la page et retour en haut à gauche
            for (i = 0; i < 1024; i++) {
                state->memory[vdu_page + i] = 32;
            }
            cursor_position = 0;
            vdu_startrow = 0;
            break;
        case 0x0d:
            // Carriage return and end of line erasure / Effacement de la fin de ligne et retour en début de linge
            if (cursor_position % 64 != 0) {
                while(cursor_position % 64 != 0) {
                    state->memory[vdu_page + (((64 * vdu_startrow) + cursor_position) % 1024)] = 32;
                    cursor_position++;
                }
                cursor_position -= 64;
            }
            break;
        case 0x18:
            // +1 page (next page) / +1 page (page suivante du texte)
        case 0x19:
            // -1 page (former page) / -1 page (page précédente du texte)
            // Only one page of RAM is used, so these commands have no effect
            break;
        case 0x1a:
            // Erasure of current line / Effacement de la linge courante du curseur
            for (i = 0; i < 64; i++) {
                state->memory[vdu_page + (((64 * vdu_startrow) + (cursor_position - (cursor_position % 64)) + i) % 1024)] = 32;
            }
            break;
        case 0x1b:
            // Line feed (Displayed next line) / Descente d'une position (Ligne suivante du texte visualisée)
            vdu_startrow++;
            if (vdu_startrow > 15) {
                vdu_startrow = 0;
            }
            cursor_position -= 64;
            if (cursor_position < 0) {
                cursor_position += 1024;
            }
            break;
        case 0x1c:
            // Home cursor / Retour du curseur en haut à gauche
            cursor_position = 0;
            break;
        case 0x1d:
            // Carriage return / Retour du curseur au début de la ligne
            cursor_position -= (cursor_position % 64);
            break;
        default:
            state->memory[vdu_page + (((64 * vdu_startrow) + cursor_position) % 1024)] = input;
            cursor_position++;
            
            if (cursor_position >= 1024) {
                cursor_position -= 64;
                vdu_startrow++;
                if (vdu_startrow > 15) {
                    vdu_startrow = 0;
                }
                
                for (i = 0; i < 64; i++) {
                    state->memory[vdu_page + (((64 * vdu_startrow) + cursor_position + i) % 1024)] = 32;
                }
            }
            break;
    }
}

void IOState::key_press(sf::Event::EventType event, int key, bool shifted, bool ctrl) {
    // Handles keyboard input, placing data in port 0 (IC 49)
    // Assumes PC has UK keyboard - because that's all I have to test it with!

    int temp = 0xFF;

    if (ctrl == false) {
        
        switch(key) {
            case sf::Keyboard::Escape: temp = 0x1B; break; // escape
            case sf::Keyboard::Space: temp = 0x20; break; // space
            case sf::Keyboard::Enter: temp = 0x0D; break; // carriage return
            case sf::Keyboard::Backspace: temp = 0x08; break; // backspace
            case sf::Keyboard::Left: temp = 0x08; break; // Ctrl+H
            case sf::Keyboard::Right: temp = 0x09; break; // Ctrl+I
            case sf::Keyboard::Down: temp = 0x0A; break; // Ctrl+J
            case sf::Keyboard::Up: temp = 0x0B; break; // Ctrl+K
        }
        
        if (shifted == false) { // No shift
            
            if ((key >= sf::Keyboard::A) && (key <= sf::Keyboard::Z)) {
                temp = key + 0x61; // Letters A - Z
            }
        
            if ((key >= sf::Keyboard::Num0) && (key <= sf::Keyboard::Num9)) {
                temp = key + 0x16; // Numbers 0 to 9
            }
            
            switch (key) {
                case sf::Keyboard::LBracket: temp = 0x5B; break; // left bracket
                case sf::Keyboard::RBracket: temp = 0x5D; break; // right bracket
                case sf::Keyboard::Semicolon: temp = 0x3B; break; // semicolon
                case sf::Keyboard::Comma: temp = 0x2C; break; // comma
                case sf::Keyboard::Period: temp = 0x2E; break; // stop
                case sf::Keyboard::Quote: temp = 0x27; break; // quote
                case sf::Keyboard::Slash: temp = 0x2F; break; // slash
                case sf::Keyboard::Backslash: temp = 0x5C; break; // backslash
                case sf::Keyboard::Equal: temp = 0x3D; break; // equal
                case sf::Keyboard::Hyphen: temp = 0x2D; break; // hyphen
            }
        } else { // Shift key pressed
            
            if ((key >= sf::Keyboard::A) && (key <= sf::Keyboard::Z)) {
                temp = key + 0x41; // Graphic 34-59
            }
            
            switch (key) {
                case sf::Keyboard::Num0: temp = 0x29; break; // close brace
                case sf::Keyboard::Num1: temp = 0x21; break; // exclamation
                case sf::Keyboard::Num2: temp = 0x22; break; // double quote
                case sf::Keyboard::Num3: temp = 0x23; break; // hash
                case sf::Keyboard::Num4: temp = 0x24; break; // dollar
                case sf::Keyboard::Num5: temp = 0x25; break; // percent
                case sf::Keyboard::Num6: temp = 0x5E; break; // carrat
                case sf::Keyboard::Num7: temp = 0x26; break; // ampusand
                case sf::Keyboard::Num8: temp = 0x2A; break; // asterisk
                case sf::Keyboard::Num9: temp = 0x28; break; // open brace
                case sf::Keyboard::LBracket: temp = 0x7B; break; // graphic 60 - arrow up
                case sf::Keyboard::RBracket: temp = 0x7D; break; // graphic 62 - arrow left
                case sf::Keyboard::Semicolon: temp = 0x3A; break; // colon
                case sf::Keyboard::Comma: temp = 0x3C; break; // less than
                case sf::Keyboard::Period: temp = 0x3E; break; // greater than
                case sf::Keyboard::Quote: temp = 0x40; break; // at
                case sf::Keyboard::Slash: temp = 0x3F; break; // question
                case sf::Keyboard::Backslash: temp = 0x7C; break; // graphic 61 - arrow down
                case sf::Keyboard::Equal: temp = 0x2B; break; // plus
                case sf::Keyboard::Hyphen: temp = 0x5F; break; // underscore
            }
        }
        
    } else { // Control characters
      
        if ((key >= sf::Keyboard::A) && (key <= sf::Keyboard::Z)) {
            temp = key + 0x01; // CONT A - Z
        }
        
        switch (key) {
            case sf::Keyboard::Quote: temp = 0x00; break; // control + at
            case sf::Keyboard::Backslash: temp = 0x1C; break; // control + backslash
            case sf::Keyboard::LBracket: temp = 0x1B; break; // control + left bracket
            case sf::Keyboard::RBracket: temp = 0x1D; break; // control + right bracket
        }
    }

    if (temp != 0xFF) { // check if the key press was recognised
        key_buffer = temp; // set the key buffer
        if (event == sf::Event::KeyPressed) key_buffer |= 0x80; // set the strobe bit
    }
}

void MachineIN(State8080* state, int port, IOState* io, fstream &tape) {
    // Handles port input (CPU IN)
    switch(port) {
        case 0:
            // Keyboard buffer (IC 49)
            state->a = io->key_buffer;
            // printf("%02X\n", io->key_buffer); // for debugging
            break;
        case 1:
            // Get UART status
            state->a = io->uart_status;
            break;
        case 4:
            // Input data from tape
            if (io->tape_relay) {
                if (io->tape_status == ' ') {
                    tape.open ("TAPE", ios::in | ios::binary);
                    io->tape_status = 'r';
                }
                if ((io->tape_status == 'r') && (tape.eof() == false)) {
                    char c;
                    tape.get(c);
                    state->a = (unsigned char) c;
                } else {
                    state->a = 0x00;
                }
            }
            break;
    }
}

void MachineOUT(State8080* state, int port, IOState* io, fstream &tape) {
    // Handles port output (CPU OUT)
    switch(port) {
        case 2:
            // Output data to tape
            if (io->tape_relay) {
                if (io->tape_status == ' ') {
                    tape.open ("TAPE", ios::out | ios::app | ios::binary);
                    io->tape_status = 'w';
                }
                if (io->tape_status == 'w') {
                    char c;
                    c = (char) state->a;
                    tape.put(c);
                }
            }
            break;
        case 3:
            // LED buffer (IC 50)
            io->led_buffer = state->a;
            break;
        case 5:
            // VDU buffer (IC 51)
            if (io->vdu_buffer != state->a) {
                io->vdu_buffer = state->a;
                if (state->a >= 0x80) {
                    io->vdu_strobe(state);
                }
            }
            break;
        case 6:
            // Port 6 latches (IC 52)
            io->port6 = state->a >> 6;
            break;
        case 7:
            // Port 7 latches (IC 52) and tape power switch (RLY 1)
            io->oscillator = ((state->a & 0x40) != 0);
            
            if (((state->a & 0x80) != 0) && (io->tape_relay == false)) {
                io->tape_relay = true;
            }
            
            if (((state->a & 0x80) == 0) && io->tape_relay) {
                if ((io->tape_status == 'w') || (io->tape_status == 'r')) {
                    tape.close();
                    io->tape_status = ' ';
                }
                io->tape_relay = false;
            }
            break;
    }
}

bool load_rom(const char filename[24], int start_address, const char size[5], unsigned char main_memory[]) {
    ifstream rom;
    int rom_length = 0;
    
    if (strcmp(size, "1k") == 0) {
        rom_length = 0x400;
    }
    if (strcmp(size, "2k") == 0) {
        rom_length = 0x800;
    }
    if (strcmp(size, "4k") == 0) {
        rom_length = 0x1000;
    }
    if (strcmp(size, "8k") == 0) {
        rom_length = 0x2000;
    }
    
    if (rom_length == 0) {
        return false;
    }
    
    rom.open(filename, ios::in | ios::binary);
    if (rom.is_open()) {
        rom.read ((char *) &main_memory[start_address], rom_length);
        rom.close();
    } else {
        std::cout << "Unable to load ROM\n";
        rom.close();
        return false;
    }
    
    return true;
}

int main(int argc, char **argv) {
    unsigned char main_memory[0xffff];

    int time;
    int running_time;
    int cursor_count = 0;
    int i;
    IOState io;
    int xpos, ypos;
    
    int opcode;
    int port;
    int framerate = 25;
    int operations_per_frame;
    int glyph;
    int vdu_rolloffset;
    
    bool inFocus = true;
    bool shifted = false;
    bool ctrl = false;
    bool pause = false;
    bool cursor_on = true;
    
    // One microcycle is 1.25uS = effective clock rate of 800kHz
    operations_per_frame = 800000 / framerate;
    
    State8080 state;
    fstream tape;
    
    sf::Int16 wave[11025]; // Quarter of a second at 44.1kHz
    const double increment = 1000./44100;
    double x = 0;
    
    for (i = 0; i < 11025; i++) {
        wave[i] = 10000 * sin(x * 6.28318);
        x += increment;
    }
    
    sf::SoundBuffer Buffer;
    Buffer.loadFromSamples(wave, 11025, 1, 44100);
    
    sf::Sound beep;
    beep.setBuffer(Buffer);
    beep.setLoop(true);
    
    io.uart_status = 0x11;
    io.tape_status = ' ';
    io.vdu_startrow = 0;
    
    if (argc == 1) {
        if (load_rom("MONA72.ROM", 0x00, "1k", main_memory) == false) {
            exit(1);
        }
        if (load_rom("MONB72.ROM", 0xc00, "1k", main_memory) == false) {
            exit(1);
        }
        if (load_rom("BASIC72.ROM", 0xe000, "8k", main_memory) == false) {
            exit(1);
        }
    } else if (strcmp(argv[1], "4.1") == 0) {
        if (load_rom("roms/L4.1 MONITOR.BIN", 0x00, "1k", main_memory) == false) {
            exit(1);
        }
        if (load_rom("roms/L4.1A BASIC.BIN", 0x400, "1k", main_memory) == false) {
            exit(1);
        }
        if (load_rom("roms/L4.1B BASIC.BIN", 0x800, "1k", main_memory) == false) {
            exit(1);
        }
    } else if (strcmp(argv[1], "5.1") == 0) {
        if (load_rom("roms/ROM_5.1A.BIN", 0x00, "1k", main_memory) == false) {
            exit(1);
        }
        if (load_rom("roms/ROM_5.1A BASIC.BIN", 0x400, "1k", main_memory) == false) {
            exit(1);
        }
        if (load_rom("roms/ROM_5.1B BASIC.BIN", 0x800, "1k", main_memory) == false) {
            exit(1);
        }
        if (load_rom("roms/ROM_5.1B.BIN", 0xc00, "1k", main_memory) == false) {
            exit(1);
        }
    } else if (strcmp(argv[1], "5.2") == 0) {
        if (load_rom("roms/ROM_5.2A.BIN", 0x00, "1k", main_memory) == false) {
            exit(1);
        }
        if (load_rom("roms/ROM_5.1A BASIC.BIN", 0x400, "1k", main_memory) == false) {
            exit(1);
        }
        if (load_rom("roms/ROM_5.1B BASIC.BIN", 0x800, "1k", main_memory) == false) {
            exit(1);
        }
        if (load_rom("roms/ROM_5.2B.BIN", 0xc00, "1k", main_memory) == false) {
            exit(1);
        }
    } else if (strcmp(argv[1], "7.2") == 0) {
        if (load_rom("roms/ROM_7.2A.BIN", 0x00, "1k", main_memory) == false) {
            exit(1);
        }
        if (load_rom("roms/ROM_7.2B.BIN", 0xc00, "1k", main_memory) == false) {
            exit(1);
        }
        if (load_rom("roms/L7.2A BASIC.BIN", 0xe000, "1k", main_memory) == false) {
            exit(1);
        }
        if (load_rom("roms/L7.2B BASIC.BIN", 0xe400, "1k", main_memory) == false) {
            exit(1);
        }
        if (load_rom("roms/L7.2C BASIC.BIN", 0xe800, "1k", main_memory) == false) {
            exit(1);
        }
        if (load_rom("roms/L7.2D BASIC.BIN", 0xec00, "1k", main_memory) == false) {
            exit(1);
        }
        if (load_rom("roms/L7.2E BASIC.BIN", 0xf000, "1k", main_memory) == false) {
            exit(1);
        }
        if (load_rom("roms/L7.2F BASIC.BIN", 0xf400, "1k", main_memory) == false) {
            exit(1);
        }
        if (load_rom("roms/L7.2G BASIC.BIN", 0xf800, "1k", main_memory) == false) {
            exit(1);
        }
        if (load_rom("roms/L7.2H BASIC.BIN", 0xfc00, "1k", main_memory) == false) {
            exit(1);
        }
    } else if (strcmp(argv[1], "7.2DEC") == 0) {
        if (load_rom("roms/ROM_7.2A.BIN", 0x00, "1k", main_memory) == false) {
            exit(1);
        }
        if (load_rom("roms/ROM_7.2B.BIN", 0xc00, "1k", main_memory) == false) {
            exit(1);
        }
        if (load_rom("roms/L7.2A BASIC 31DECEMBER2020.BIN", 0xe000, "1k", main_memory) == false) {
            exit(1);
        }
        if (load_rom("roms/L7.2B BASIC 31DECEMBER2020.BIN", 0xe400, "1k", main_memory) == false) {
            exit(1);
        }
        if (load_rom("roms/L7.2C BASIC 31DECEMBER2020.BIN", 0xe800, "1k", main_memory) == false) {
            exit(1);
        }
        if (load_rom("roms/L7.2D BASIC 31DECEMBER2020.BIN", 0xec00, "1k", main_memory) == false) {
            exit(1);
        }
        if (load_rom("roms/L7.2E BASIC 31DECEMBER2020.BIN", 0xf000, "1k", main_memory) == false) {
            exit(1);
        }
        if (load_rom("roms/L7.2F BASIC 31DECEMBER2020.BIN", 0xf400, "1k", main_memory) == false) {
            exit(1);
        }
        if (load_rom("roms/L7.2G BASIC 31DECEMBER2020.BIN", 0xf800, "1k", main_memory) == false) {
            exit(1);
        }
        if (load_rom("roms/L7.2H BASIC 31DECEMBER2020.BIN", 0xfc00, "1k", main_memory) == false) {
            exit(1);
        }
    } else {
        std::cout << "Invalid CLI argument\n";
        exit(1);
    }
    
    state.a = 0;
    state.pc = 0x00;
    state.memory = main_memory;
    
    // Initialise window
    sf::RenderWindow window(sf::VideoMode(512, 414), "Transam Triton");
    window.setFramerateLimit(framerate);
    
    sf::Texture fontmap;
    if (!fontmap.loadFromFile("font.png")) {
        std::cout << "Error loading font file";
        exit(1);
    }
    
    sf::Texture tapemap;
    if (!tapemap.loadFromFile("tape.png")) {
        std::cout << "Error loading tape image";
        exit(1);
    }
    
    sf::Sprite sprite[1024];
    sf::CircleShape led[8];
    sf::Color ledoff = sf::Color(50,0,0);
    sf::Color ledon = sf::Color(250,0,0);
    sf::Sprite tape_indicator;
    sf::RectangleShape cursor(sf::Vector2f(8.0f, 2.0f));
    
    for (i = 0; i < 1024; i++) {
        sprite[i].setTexture(fontmap);
        ypos = ((i - (i % 8)) / 64) * 24;
        xpos = (i % 64) * 8;
        sprite[i].setPosition(sf::Vector2f((float) xpos,(float) ypos));
    }
    
    for (i = 0; i < 8; i++) {
        led[i].setRadius(7.0f);
        led[i].setPosition(15.0f + (i * 15), 396.0f);
    }
    tape_indicator.setTexture(tapemap);
    tape_indicator.setPosition(sf::Vector2f(462.0f, 386.0f));

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
                inFocus = false;
            
            if (event.type == sf::Event::GainedFocus)
                inFocus = true;
            
            // Keep track of shift and control keys
            if (event.type == sf::Event::KeyPressed) {
                if ((event.key.code == sf::Keyboard::LShift) || (event.key.code == sf::Keyboard::RShift))
                    shifted = true;
                
                if ((event.key.code == sf::Keyboard::LControl) || (event.key.code == sf::Keyboard::RControl))
                    ctrl = true;
            }
            
            if (event.type == sf::Event::KeyReleased) {
                if ((event.key.code == sf::Keyboard::LShift) || (event.key.code == sf::Keyboard::RShift))
                    shifted = false;
                
                if ((event.key.code == sf::Keyboard::LControl) || (event.key.code == sf::Keyboard::RControl))
                    ctrl = false;
            }
                
            if (inFocus) {
                // Respond to key presses
                if (event.type == sf::Event::KeyPressed) {
                    switch(event.key.code) {
                        case sf::Keyboard::F1:
                            // Reset button (PB 1)
                            state.pc = 0x0000;
                            state.int_enable = false;
                            break;
                        case sf::Keyboard::F2:
                            // Clear Screen button (PB 2)
                            if (state.int_enable) {
                                state.int_enable = false;
                                main_memory[state.sp - 2] = state.pc & 0xff;
                                main_memory[state.sp - 1] = state.pc >> 8;
                                state.sp -= 2;
                                state.pc = 0x0008; // RST1
                            }
                            break;
                        case sf::Keyboard::F3:
                            // Initialise button (PB 3)
                            if (state.int_enable) {
                                state.int_enable = false;
                                main_memory[state.sp - 2] = state.pc & 0xff;
                                main_memory[state.sp - 1] = state.pc >> 8;
                                state.sp -= 2;
                                state.pc = 0x0010; // RST2
                            }
                            break;
                        case sf::Keyboard::F4:
                            // Pause button (PB 4)
                            if (pause) {
                                pause = false;
                            } else {
                                pause = true;
                            }
                            break;
                        case sf::Keyboard::F9:
                            window.close();
                            break;
                        default:
                            io.key_press(event.type, event.key.code, shifted, ctrl);
                            break;
                    }
                }
                
                if (event.type == sf::Event::KeyReleased) {
                    io.key_press(event.type, event.key.code, shifted, ctrl);
                }
            }
        }
        
        if (pause == false) {
            // Send as many clock pulses to the CPU as would happen
            // between screen frames
            running_time = 0;
            while (running_time < operations_per_frame) {
                opcode = get_memory(&state, state.pc);
                port = get_memory(&state, state.pc + 1);

                if (opcode == 0xdb) { // IN
                    MachineIN(&state, port, &io, tape);
                    state.pc += 2;
                    time = 10;
                } else if (opcode == 0xd3) { // OUT
                    MachineOUT(&state, port, &io, tape);
                    state.pc += 2;
                    time = 10;
                } else { // All other operands
                    time = Emulate8080Op(&state);
                }
                
                running_time += time;
            }
            
            cursor_count++;
            
            // Draw screen from VDU memory
            // Font texture acts as ROMs (IC 69 and 70)
            window.clear();
            vdu_rolloffset = 64 * io.vdu_startrow;
            for (i = 0; i < 1024; i++) {
                glyph = main_memory[0x1000 + ((vdu_rolloffset + i) % 1024)] & 0x7f;
                xpos = (glyph % 16) * 8;
                ypos = ((glyph / 16) * 24);
                sprite[i].setTextureRect(sf::IntRect(xpos, ypos, 8, 24));
                window.draw(sprite[i]);
            }
            for (i = 0; i < 8; i++) {
                if ((io.led_buffer & (0x80 >> i)) == 0) {
                    // note LEDs are on for "0"
                    led[i].setFillColor(ledon);
                } else {
                    led[i].setFillColor(ledoff);
                }
                window.draw(led[i]);
            }
            
            if (io.tape_relay == false) {
                tape_indicator.setTextureRect(sf::IntRect(0, 0, 45, 30));
            } else {
                switch (io.tape_status) {
                    case ' ':
                        tape_indicator.setTextureRect(sf::IntRect(45, 0, 45, 30));
                        break;
                    case 'r':
                        tape_indicator.setTextureRect(sf::IntRect(90, 0, 45, 30));
                        break;
                    case 'w':
                        tape_indicator.setTextureRect(sf::IntRect(135, 0, 45, 30));
                        break;
                }
            }
            window.draw(tape_indicator);
            
            if (cursor_count > (framerate / 4)) {
                // Cursor has 2Hz "winking" frequency
                if (cursor_on) {
                    cursor.setFillColor(sf::Color(0, 0, 0));
                    cursor_on = false;
                } else {
                    cursor.setFillColor(sf::Color(255, 255, 255));
                    cursor_on = true;
                }
                cursor_count = 0;
            }
            i = io.cursor_position;
            ypos = (((i - (i % 8)) / 64) * 24) + 18;
            xpos = (i % 64) * 8;
            cursor.setPosition(sf::Vector2f((float) xpos,(float) ypos));
            window.draw(cursor);
            
            window.display();
            
            if (io.oscillator) {
                beep.play();
            } else {
                beep.pause();
            }
            
        } else {
            beep.pause();
        }
    }

    return 0;
}
