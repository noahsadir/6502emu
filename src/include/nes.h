/**
 * nes.h
 * 
 * Main controller for NES emulation.
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

#ifndef NES_H
#define NES_H

#include "global.h"
#include "config.h"
#include "nescartridge.h"
#include "mos6502.h"
#include "io.h"
#include "nesppu.h"
#include "nesjoypad.h"

void nes_init(char* fsRoot);
void nes_start(void);
void nes_disassemble(char* filePath);
void nes_configureMemory(void);
uint8_t nes_cpuRead(uint16_t addr);
void nes_cpuWrite(uint16_t addr, uint8_t data);
void nes_finishedInstruction(uint8_t cycles);
void nes_generateMetrics(char* outputStr);
void nes_toggleJoypad(Keyboard key, int status);
void nes_debugCPU(void);

#endif
