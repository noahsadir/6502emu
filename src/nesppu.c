/**
 * nesppu.c
 * 
 * @author Noah Sadir
 * @date 2023-09-28
 */

#include "include/nesppu.h"

PPURegisters ppureg;

uint8_t ppuMemoryMap[0x4000];
uint8_t patternTable[512][64];
uint8_t oam[256];

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

uint8_t nesppu_read(uint16_t addr) {
  return ppuMemoryMap[addr];
}

void nesppu_write(uint16_t addr, uint8_t data) {
  if (addr <= 0x0FFF) {

  } else if (addr <= 0x1FFF) {

  } else if (addr <= 0x23FF) {

  } else if (addr <= 0x27FF) {

  } else if (addr <= 0x2BFF) {

  } else if (addr <= 0x2FFF) {

  } else if (addr <= 0x3EFF) {
    
  } else if (addr <= 0x3F1F) {

  } else if (addr <= 0x3FFF) {

  }
}
