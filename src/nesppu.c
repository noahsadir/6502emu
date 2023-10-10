/**
 * nesppu.c
 * 
 * @author Noah Sadir
 * @date 2023-09-28
 */
/*
TODO:
[ ] Reading/writing PPU memory
[ ] Setting registers
*/
#include "include/nesppu.h"

PPURegisters ppureg;

uint8_t ppuMemoryMap[0x4000];
uint8_t patternTable[512][64];
uint8_t nametable[4][1024];
uint8_t oam[256];

uint32_t cycleCount = 0;

void nesppu_init() {
  ppureg.ppuctrl = 0x00;
  ppureg.ppumask = 0x00;
  ppureg.ppustatus &= 0xBF;
  ppureg.oamaddr = 0x00;
  ppureg.ppuscroll = 0x00;
  ppureg.ppuaddr = 0x00;
  ppureg.ppudata = 0x00;

  nesppu_configurePatternLookup();
}

void nesppu_step(uint16_t cycles, void(*invoke_nmi)(void)) {
  for (int i = 0; i < cycles; i++) {
    // increment cycle count or reset from beginning
    cycleCount = (cycleCount > 89342) ? 0 : cycleCount + 1;
    uint16_t scanline = cycleCount / 341;
    if (scanline < 241) {
      // probably check for sprite zero
    } else if (scanline == 241) {
      nesppu_drawFrame();
      ppureg.ppustatus = SET_ppustat_vblankstarted(ppureg.ppustatus, 1);
    }
    
    if (scanline >= 241) {
      // invoke NMI any time during vblank when both Vblank & NMI flags are enabled
      if (GET_ppustat_vblankstarted(ppureg.ppustatus) && GET_ppuctrl_generatenmi(ppureg.ppuctrl)) {
        invoke_nmi();
      }
    }
  }
}

void nesppu_drawFrame() {

}

uint8_t nesppu_read(uint16_t addr) {
  return ppuMemoryMap[addr];
}

void nesppu_write(uint16_t addr, uint8_t data) {
  if (addr <= 0x1FFF) { // Pattern table
    // read only
  } else if (addr <= 0x3EFF) { // Nametable
    // complex mirrror stuff
    addr = 0x2000 + (addr % 0x400);
    ppuMemoryMap[addr] = data;
    ppuMemoryMap[addr + 0x400] = data;
    ppuMemoryMap[addr + 0x800] = data;
    ppuMemoryMap[addr + 0xC00] = data;
  } else if (addr <= 0x3FFF) { // Palette
    addr = 0x3F00 + (addr % 0x1F);
    if (addr == 0x3F00 || addr == 0x3F10) {
      // special case for background palette
      addr = 0x3F00;
      while (addr < 0x3FFF) {
        ppuMemoryMap[addr] = data;
        ppuMemoryMap[addr + 0x10] = data;
        addr += 0x20;
      }
    } else {
      while (addr < 0x3FFF) {
        ppuMemoryMap[addr] = data;
        addr += 0x20;
      }
    }
  }
}

void nesppu_configurePatternLookup() {
  for (int i = 0; i < 512; i++) {
    uint16_t chrAddrLow = i * 16;
    uint16_t chrAddrHigh = chrAddrLow + 8;
    uint16_t patternIndex = 0;
    for (int row = 0; row < 8; row++) {
      for (int col = 0; col < 8; col++) {
        uint8_t lowBit = (ppuMemoryMap[chrAddrLow + row] >> (7 - col)) & 1;
        uint8_t highBit = (ppuMemoryMap[chrAddrHigh + row] >> (7 - col)) & 1;
        patternTable[i][patternIndex] = (highBit << 1) | lowBit;
        patternIndex += 1;
      }
    }
  }

  if (CONFIG_DEBUG.shouldDisplayDebugScreen) {
    for (int chr = 0; chr < 256; chr++) {
      uint8_t tileRow = chr / 16;
      uint8_t tileCol = chr % 16;
      for (int i = 0; i < 64; i++) {
        uint32_t pos = (tileRow * 256 * 8) + ((i / 8) * 256) + (tileCol * 8) + (i % 8);
        switch (patternTable[chr][i]) {
          case 0: BITMAP2[pos] = 0x000000; break;
          case 1: BITMAP2[pos] = 0xFF0000; break;
          case 2: BITMAP2[pos] = 0x00FF00; break;
          case 3: BITMAP2[pos] = 0x0000FF; break;
        }
      }
    }
    for (int chr = 0; chr < 256; chr++) {
      uint8_t tileRow = chr / 16;
      uint8_t tileCol = chr % 16;
      for (int i = 0; i < 64; i++) {
        uint32_t pos = (tileRow * 256 * 8) + ((i / 8) * 256) + (tileCol * 8) + (i % 8);
        switch (patternTable[chr + 256][i]) {
          case 0: BITMAP2[pos + 128] = 0x000000; break;
          case 1: BITMAP2[pos + 128] = 0xFF0000; break;
          case 2: BITMAP2[pos + 128] = 0x00FF00; break;
          case 3: BITMAP2[pos + 128] = 0x0000FF; break;
        }
      }
    }
  }
}
