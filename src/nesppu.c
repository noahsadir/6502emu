/**
 * nesppu.c
 * 
 * @author Noah Sadir
 * @date 2023-09-28
 */
/*
TODO:
[*] Reading/writing PPU memory
[*] Setting registers
[*] Load all 4 nametables
[*] Load attribute table
[*] Load palettes
[*] Render background
[*] Render sprites
[*] Sprite 0 hit
[~] Scrolling
*/
#include "include/nesppu.h"

PPURegisters ppureg;

uint8_t ppuMemoryMap[0x4000];
uint8_t patternTable[512][64];
uint8_t nametable[4][960];
uint8_t visibleBackground[61440];
uint8_t attrTable[4][64];
uint8_t* paletteTable = ppuMemoryMap + 0x3F00;
uint8_t oam[256];

uint8_t scanlineScrollX[262];
uint8_t scanlineScrollY[262];
PPURegisters scanlineReg[262];

uint32_t cycleCount = 0;
INES cartridge;
bool didGenerateNmi = false;
int lastScanline = 0;
bool shouldEdit[4];
bool states[4];

void nesppu_init(INES* ines) {
  ppureg.ppuctrl = 0x00;
  ppureg.ppumask = 0x00;
  ppureg.ppustatus &= 0xBF;
  ppureg.oamaddr = 0x00;
  ppureg.ppuscroll = 0x00;
  ppureg.ppuaddr = 0x00;
  ppureg.ppudata = 0x00;
  states[0] = 1;
  states[1] = (cartridge.header.mirroringType == MIRRORING_HORIZONTAL);
  states[2] = (cartridge.header.mirroringType == MIRRORING_VERTICAL);
  states[3] = 0;

  cartridge = *ines;
  nesppu_configurePatternLookup();
}

void nesppu_step(uint16_t cycles, void(*invoke_nmi)(void)) {
  for (int i = 0; i < cycles; i++) {
    // increment cycle count or reset from beginning
    cycleCount = (cycleCount > 89342) ? 0 : cycleCount + 1;
    uint16_t scanline = cycleCount / 341;
    uint8_t spriteZeroY = oam[0];
    if (lastScanline == scanline) continue; // only run code below once per scanline
    scanlineReg[scanline] = ppureg;
    //printf("%d: %d\n", scanline, GET_ppuctrl_nametable(ppureg.ppuctrl));

    if (scanline == 0) {
      didGenerateNmi = false;
      nesppu_drawBackground();
      nesppu_drawSprites(true);
    }
    
    if (scanline < 241) {
      // probably check for sprite zero, render sprites maybe?
      if (spriteZeroY == scanline) {
        ppureg.ppustatus = SET_ppustat_spritezerohit(ppureg.ppustatus, 1);
      }
    }
    
    if (scanline == 241) {
      ppureg.ppustatus = SET_ppustat_vblankstarted(ppureg.ppustatus, 1);
    }
    
    if (scanline >= 241) {
      // invoke NMI any time during vblank when both Vblank & NMI flags are enabled
      if (GET_ppustat_vblankstarted(ppureg.ppustatus) && GET_ppuctrl_generatenmi(ppureg.ppuctrl) && !didGenerateNmi) {
        invoke_nmi();
        didGenerateNmi = true;
      }
    }

    if (scanline == 261) {
      ppureg.ppustatus = SET_ppustat_spritezerohit(ppureg.ppustatus, 0);
    }

    lastScanline = scanline;
  }
}

void nesppu_drawSprites(bool hasPriority) {
  for (int i = 0; i < 64; i++) {
    uint8_t byte0 = oam[i * 4];
    uint8_t byte1 = oam[(i * 4) + 1];
    uint8_t byte2 = oam[(i * 4) + 2];
    uint8_t byte3 = oam[(i * 4) + 3];
    
    uint8_t x = byte3;
    uint8_t y = byte0;
    uint16_t bankOffset = GET_ppuctrl_spritepattern(ppureg.ppuctrl) ? 256 : 0;
    uint8_t tileId = byte1;
    uint8_t paletteIndex = byte2 & 0x3;
    bool flipVertical = byte2 & BIT_MASK_8;
    bool flipHorizontal = byte2 & BIT_MASK_7;
    bool priority = !(byte2 & BIT_MASK_6);

    if (y == 0 || y >= 0xEF) continue;

    for (int i = 0; i < 64; i++) {
      int row = flipVertical ? 7 - (i / 8) : (i / 8);
      int col = flipHorizontal ? 7 - (i % 8) : (i % 8);
      uint32_t pos = (y * 256) + (row * 256) + x + col;
      if ((x + col) >= 248 || (y + row) >= 232) continue;
      uint8_t color = patternTable[tileId + bankOffset][i];

      // only draw sprite pixel if:
      // - pixel is not transparent (0)
      // - in front of background ~OR~ behind but background pixel is 0
      uint8_t nametablePixelVal = visibleBackground[pos];
      if (color != 0 && (priority || nametablePixelVal == 0)) {
        BITMAP0[pos] = colors[paletteTable[(paletteIndex * 4) + color + 0x10]];
      }
    }
  }
}

void nesppu_drawBackground(void) {
  uint16_t bankOffset = GET_ppuctrl_backgroundpattern(ppureg.ppuctrl) ? 256 : 0;
  for (int r = 0; r < 31; r++) {
    for (int c = 0; c < 33; c++) {
      // Account for scroll when picking nametable tile
      uint16_t baseNametable = GET_ppuctrl_nametable(scanlineReg[r * 8].ppuctrl);
      // use scanlineReg since registers may have changed mid-render
      int coarseScrollX = scanlineReg[r * 8].scrollX / 8;
      int coarseScrollY = scanlineReg[r * 8].scrollY / 8;
      int fineScrollX = scanlineReg[r * 8].scrollX % 8;
      int fineScrollY = scanlineReg[r * 8].scrollY % 8;

      int row = r + coarseScrollY;
      int col = c + coarseScrollX;

      int ntId = baseNametable;

      // if scroll-adjusted row or col goes "off the screen",
      // overflow to next appropriate nametable
      if (row >= 30) {
        ntId = (ntId + 2) % 4;
        row -= 30;
      }
      if (col >= 32) {
        ntId = (ntId + 1) % 4;
        col -= 32;
      }

      // determine palette value for tile
      uint8_t attrByte = attrTable[ntId][((row / 4) * 8) + (col / 4)];
      uint8_t attrRow = (row % 4) / 2;
      uint8_t attrCol = (col % 4) / 2;
      uint8_t attrShift = ((attrRow * 2) + attrCol) * 2;
      attrByte = (attrByte >> attrShift) & 3;
      nesppu_drawBkgTileFromPatternTable(nametable[ntId][(row * 32) + col], bankOffset, attrByte, (c * 8) - fineScrollX, (r * 8) - fineScrollY);
    }
  }

  if (CONFIG_DEBUG.shouldDisplayDebugScreen) {
    nesppu_drawDebugData();
  }
}

void nesppu_drawOutlinedSquare(uint32_t color, uint8_t size, uint8_t x, uint8_t y) {
  // draw a color square with a dotted outline
  // used to display palette table for debugging purposes
  for (int row = 0; row < size; row++) {
    for (int col = 0; col < size; col++) {
      if (row == 0 || col == 0 || row == size - 1 || col == size - 1) {
        BITMAP2[((y + row) * 256) + x + col] = ((row + col) % 2 == 0) ? 0x000000 : 0xFFFFFF;
      } else {
        BITMAP2[((y + row) * 256) + x + col] = color;
      }
      
    }
  }
}

uint8_t nesppu_read(uint16_t addr) {
  return ppuMemoryMap[addr];
}

void nesppu_drawBkgTileFromPatternTable(uint16_t id, uint16_t bankOffset, uint8_t paletteIndex, int16_t x, int16_t y) {
  // draw a full-size tile in BITMAP0
  for (int i = 0; i < 64; i++) {
    // ignore pixel if OOB
    if (x + (i % 8) < 0 || y + (i / 8) < 0 || x + (i % 8) >= 256 || y + (i / 8) >= 240) continue;

    uint32_t pos = (y * 256) + ((i / 8) * 256) + (x + (i % 8));
    uint8_t color = patternTable[id + bankOffset][i];
    BITMAP0[pos] = (color == 0) ? colors[paletteTable[0]] : colors[paletteTable[(paletteIndex * 4) + color]];
    visibleBackground[pos] = color;
  }
}

void nesppu_drawFromPatternTableDebug(uint16_t id, uint16_t bankOffset, uint8_t paletteIndex, uint16_t x, uint16_t y) {
  // draw a shrunken tile in BITMAP3
  for (int i = 0; i < 16; i++) {
    uint32_t pos = (y * 256) + ((i / 4) * 256) + x + (i % 4);
    uint8_t color = patternTable[id + bankOffset][i * 2];
    BITMAP3[pos] = (color == 0) ? colors[paletteTable[0]] : colors[paletteTable[(paletteIndex * 4) + color]];
    BITMAP3[pos] = ~BITMAP3[pos];
  }
}

void nesppu_write(uint16_t addr, uint8_t data) {
  if (addr <= 0x1FFF) { // Pattern table
    // read only
  } else if (addr <= 0x3EFF) { // Nametable
    // determine which of the 4 nametables should be edited
    // not sure if state table is the best way, but it does the job
    if (addr <= 0x2400) {
      shouldEdit[0] = states[0]; // see nesppu_init() for info on state elements
      shouldEdit[1] = states[1];
      shouldEdit[2] = states[2];
      shouldEdit[3] = states[3];
    } else if (addr <= 0x2800) {
      shouldEdit[0] = states[1];
      shouldEdit[1] = states[0];
      shouldEdit[2] = states[3];
      shouldEdit[3] = states[2];
    } else if (addr <= 0x2C00) {
      shouldEdit[0] = states[2];
      shouldEdit[1] = states[3];
      shouldEdit[2] = states[0];
      shouldEdit[3] = states[1];
    } else {
      shouldEdit[0] = states[3];
      shouldEdit[1] = states[2];
      shouldEdit[2] = states[1];
      shouldEdit[3] = states[0];
    }

    addr = 0x2000 + (addr % 0x400);
    for (int i = 0; i < 4; i++) {
      if (shouldEdit[i]) {
        ppuMemoryMap[addr] = data;
        // determine if nametable or attribute table should be updated
        if (addr < 0x23C0) nametable[i][addr - 0x2000] = data;
        else attrTable[i][addr - 0x23C0] = data;
      }
    }
  } else if (addr <= 0x3FFF) { // Palette
    addr = 0x3F00 + (addr % 0x20);
    if (addr == 0x3F00 || addr == 0x3F10) {
      // special case for univ. background color
      addr = 0x3F00;
      while (addr < 0x3FFF) {
        ppuMemoryMap[addr] = data;
        ppuMemoryMap[addr + 0x10] = data;
        addr += 0x20;
      }
    } else {
      // mirror palette entries up to 0x3FFF
      while (addr < 0x3FFF) {
        ppuMemoryMap[addr] = data;
        addr += 0x20;
      }
    }
  }
}

void nesppu_drawDebugData(void) {
  uint16_t bankOffset = GET_ppuctrl_backgroundpattern(ppureg.ppuctrl) ? 256 : 0;

  // render all 4 nametables in separate screen
  for (int row = 0; row < 30; row++) {
    for (int col = 0; col < 32; col++) {
      for (int i = 0; i < 4; i++) {
        uint8_t attrByte = attrTable[i][((row / 4) * 8) + (col / 4)];
        uint8_t attrRow = (row % 4) / 2;
        uint8_t attrCol = (col % 4) / 2;
        uint8_t attrShift = ((attrRow * 2) + attrCol) * 2;
        attrByte = (attrByte >> attrShift) & 3;
        nesppu_drawFromPatternTableDebug(nametable[i][(row * 32) + col], bankOffset, attrByte, (col * 4) + (i % 2 ? 128 : 0), (row * 4) + (i < 2 ? 0 : 120));
      }
    }
  }

  // nametable colors are inverted by default on debug screen
  // if pixel is currently visible, show as un-inverted
  for (int i = 0; i < 120; i++) {
    uint8_t baseNametable = GET_ppuctrl_nametable(scanlineReg[i * 2].ppuctrl);
    uint8_t scrollX = scanlineReg[i * 2].scrollX / 2;
    uint8_t scrollY = scanlineReg[i * 2].scrollY / 2;
    uint8_t scrollXPos = scrollX + ((baseNametable == 1 || baseNametable == 3) ? 128 : 0);
    uint8_t scrollYPos = scrollY + ((baseNametable == 2 || baseNametable == 3) ? 128 : 0);
    for (int j = 0; j < 128; j++) {
      uint16_t pos = ((scrollXPos + j) % 256) + (((i + scrollYPos) % 240) * 256);
      BITMAP3[pos] = ~BITMAP3[pos];
    }
  }

  // print palette tables
  for (int paletteIndex = 0; paletteIndex < 4; paletteIndex++) {
    nesppu_drawOutlinedSquare(colors[paletteTable[(paletteIndex * 4) + 0]], 8, 8 + (paletteIndex * 40), 152);
    nesppu_drawOutlinedSquare(colors[paletteTable[(paletteIndex * 4) + 1]], 8, 16 + (paletteIndex * 40), 152);
    nesppu_drawOutlinedSquare(colors[paletteTable[(paletteIndex * 4) + 2]], 8, 24 + (paletteIndex * 40), 152);
    nesppu_drawOutlinedSquare(colors[paletteTable[(paletteIndex * 4) + 3]], 8, 32 + (paletteIndex * 40), 152);
  }
  for (int paletteIndex = 4; paletteIndex < 8; paletteIndex++) {
    nesppu_drawOutlinedSquare(colors[paletteTable[(paletteIndex * 4) + 0]], 8, 8 + ((paletteIndex - 4) * 40), 176);
    nesppu_drawOutlinedSquare(colors[paletteTable[(paletteIndex * 4) + 1]], 8, 16 + ((paletteIndex - 4) * 40), 176);
    nesppu_drawOutlinedSquare(colors[paletteTable[(paletteIndex * 4) + 2]], 8, 24 + ((paletteIndex - 4) * 40), 176);
    nesppu_drawOutlinedSquare(colors[paletteTable[(paletteIndex * 4) + 3]], 8, 32 + ((paletteIndex - 4) * 40), 176);
  }
}

void nesppu_configurePatternLookup(void) {
  // CHR ROM stores pattern table in a way which is difficult to parse
  // So let's store each character as a 64-element array of 2-bit values
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
    nesppu_drawTableText();
    uint16_t offset = 8 * 256;

    // Draw first pattern table in BITMAP2 at (0, 8) as a 16x16 grid
    for (int chr = 0; chr < 256; chr++) {
      uint8_t tileRow = chr / 16;
      uint8_t tileCol = chr % 16;
      for (int i = 0; i < 64; i++) {
        uint32_t pos = (tileRow * 256 * 8) + ((i / 8) * 256) + (tileCol * 8) + (i % 8) + offset;
        switch (patternTable[chr][i]) {
          case 0: BITMAP2[pos] = 0x000000; break;
          case 1: BITMAP2[pos] = 0xFF0000; break;
          case 2: BITMAP2[pos] = 0x00FF00; break;
          case 3: BITMAP2[pos] = 0x0000FF; break;
        }
      }
    }

    // Draw second pattern table in BITMAP2 at (128, 8) as a 16x16 grid
    for (int chr = 0; chr < 256; chr++) {
      uint8_t tileRow = chr / 16;
      uint8_t tileCol = chr % 16;
      for (int i = 0; i < 64; i++) {
        uint32_t pos = (tileRow * 256 * 8) + ((i / 8) * 256) + (tileCol * 8) + (i % 8) + offset;
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

void nesppu_drawTableText(void) {
  // draw descriptor text for pattern & palette tables in BITMAP2
  io_drawString(" Pattern Tbl 1   Pattern Tbl 2\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n Bkgrnd Palette\n\n\n Sprite Palette", 2);
}
