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
#include "nescartridge.h"

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

static const uint32_t colors[64] =
{
0x757575, 0x271B8F, 0x0000AB, 0x47009F, 0x8F0077, 0xAB0013, 0xA70000, 0x7F0B00,
0x432F00, 0x004700, 0x005100, 0x003F17, 0x1B3F5F, 0x000000, 0x000000, 0x000000,
0xBCBCBC, 0x0073EF, 0x233BEF, 0x8300F3, 0xBF00BF, 0xE7005B, 0xDB2B00, 0xCB4F0F,
0x8B7300, 0x009700, 0x00AB00, 0x00933B, 0x00838B, 0x000000, 0x000000, 0x000000,
0xFFFFFF, 0x3FBFFF, 0x5F97FF, 0xA78BFD, 0xF77BFF, 0xFF77B7, 0xFF7763, 0xFF9B3B,
0xF3BF3F, 0x83D313, 0x4FDF4B, 0x58F898, 0x00EBDB, 0x000000, 0x000000, 0x000000,
0xFFFFFF, 0xABE7FF, 0xC7D7FF, 0xD7CBFF, 0xFFC7FF, 0xFFC7DB, 0xFFBFB3, 0xFFDBAB,
0xFFE7A3, 0xE3FFA3, 0xABF3BF, 0xB3FFCF, 0x9FFFF3, 0x000000, 0x000000, 0x000000
};

extern PPURegisters ppureg;
extern uint8_t ppuMemoryMap[0x4000];
extern uint8_t oam[256];

void nesppu_init(INES* ines);
void nesppu_step(uint16_t cycles, void(*invoke_nmi)(void));
void nesppu_drawFrame();
void nesppu_drawTableText();
void nesppu_drawDebugData();
void nesppu_configurePatternLookup();
void nesppu_drawFromPatternTableDebug(uint16_t id, uint16_t bankOffset, uint8_t paletteIndex, uint16_t x, uint16_t y);
void nesppu_drawFromPatternTable(uint16_t id, uint16_t bankOffset, uint8_t paletteIndex, uint16_t x, uint16_t y);
void nesppu_drawOutlinedSquare(uint32_t color, uint8_t size, uint8_t x, uint8_t y);
uint8_t nesppu_read(uint16_t addr);
void nesppu_write(uint16_t addr, uint8_t data);

#endif
