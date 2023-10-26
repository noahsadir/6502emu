/**
 * io.h
 * 
 * Handle host-specific I/O functionality.
 * 
 * @author Noah Sadir
 * @date 2023-07-30
 * 
 * Copyright (c) 2023 Noah Sadir
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef IO_H
#define IO_H

#include "global.h"
#include "config.h"
#include "font.h"

extern uint32_t* BITMAP0;
extern uint32_t* BITMAP1;
extern uint32_t* BITMAP2;
extern uint32_t* BITMAP3;
extern char* OVERLAY_MSG;
extern char* PANIC_MSG;
extern bool PANIC_MODE;

void io_init(void);
int io_pollInput(Keyboard* key);
void io_render(void);
void io_clear(void);
void io_drawString(char* str, int screen);
void io_drawChar(char chr, int num, uint32_t* bmp);
void io_drawScreen(int screen,  uint32_t* pixels);
void io_drawPixel(int x, int y, int screen, uint32_t color,  uint32_t* pixels);
void io_panic(char* str);
void io_kill(void);

#endif
