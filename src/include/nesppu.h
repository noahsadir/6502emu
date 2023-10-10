/**
 * nesppu.h
 * 
 * Implementation of NES PPU.
 * 
 * @author Noah Sadir
 * @date 2023-09-28
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

#ifndef NESPPU_H
#define NESPPU_H

#include "global.h"
#include "config.h"
#include "io.h"

#define GET_ppuctrl_nametable(val) (val & 3)
#define GET_ppuctrl_vraminc GET_bit2
#define GET_ppuctrl_spritepattern GET_bit3
#define GET_ppuctrl_backgroundpattern GET_bit4
#define GET_ppuctrl_spritesize GET_bit5
#define GET_ppuctrl_ppuselect GET_bit6
#define GET_ppuctrl_generatenmi GET_bit7

#define GET_ppumask_grayscale GET_bit0
#define GET_ppumask_showbackgroundleft GET_bit1
#define GET_ppumask_showspirtesleft GET_bit2
#define GET_ppumask_showbackground GET_bit3
#define GET_ppumask_showsprites GET_bit4
#define GET_ppumask_emphasizered GET_bit5
#define GET_ppumask_emphasizegreen GET_bit6
#define GET_ppumask_emphasizeblue GET_bit7

#define GET_ppustat_spriteoverflow GET_bit5
#define GET_ppustat_spritezerohit GET_bit6
#define GET_ppustat_vblankstarted GET_bit7

#define SET_ppuctrl_nametable(val, f) (val & (f & 3))
#define SET_ppuctrl_vraminc SET_bit2
#define SET_ppuctrl_spritepattern SET_bit3
#define SET_ppuctrl_backgroundpattern SET_bit4
#define SET_ppuctrl_spritesize SET_bit5
#define SET_ppuctrl_ppuselect SET_bit6
#define SET_ppuctrl_generatenmi SET_bit7

#define SET_ppumask_grayscale SET_bit0
#define SET_ppumask_showbackgroundleft SET_bit1
#define SET_ppumask_showspirtesleft SET_bit2
#define SET_ppumask_showbackground SET_bit3
#define SET_ppumask_showsprites SET_bit4
#define SET_ppumask_emphasizered SET_bit5
#define SET_ppumask_emphasizegreen SET_bit6
#define SET_ppumask_emphasizeblue SET_bit7

#define SET_ppustat_spriteoverflow SET_bit5
#define SET_ppustat_spritezerohit SET_bit6
#define SET_ppustat_vblankstarted SET_bit7

typedef struct {
  uint8_t ppuctrl;
  uint8_t ppumask;
  uint8_t ppustatus;
  uint8_t oamaddr;
  uint8_t oamdata;
  uint8_t ppuscroll;
  uint8_t ppuaddr;
  uint8_t ppudata;
  uint8_t oamdma;
  bool scrollLatch;
  bool addrLatch;
  uint8_t scrollX;
  uint8_t scrollY;
  uint16_t loadedAddr;
} PPURegisters;

extern PPURegisters ppureg;
extern uint8_t ppuMemoryMap[0x4000];
extern uint8_t oam[256];

void nesppu_init();
void nesppu_step(uint16_t cycles, void(*invoke_nmi)(void));
void nesppu_drawFrame();
void nesppu_configurePatternLookup();
uint8_t nesppu_read(uint16_t addr);
void nesppu_write(uint16_t addr, uint8_t data);

#endif
