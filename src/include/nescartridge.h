/**
 * nescartridge.h
 * 
 * Handle cartridge loading for NES emulation.
 * 
 * @author Noah Sadir
 * @date 2023-08-02
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

#ifndef NESCARTRIDGE_H
#define NESCARTRIDGE_H

#include "global.h"
#include "config.h"
#include "io.h"

#include <stdint.h>

typedef enum {
  MIRRORING_HORIZONTAL  = 0,
  MIRRORING_VERTICAL    = 1
} MirroringType;

typedef enum {
  TV_NTSC  = 0,
  TV_PAL   = 1
} TVSystem;

typedef struct {
  bool disassemblyMode;
  uint8_t prgRomSize;
  uint8_t chrRomSize;
  MirroringType mirroringType;
  bool containsPrgRam;
  bool containsTrainer;
  bool ignoreMirroringControl;
  bool isVSUnisystem;
  bool isPlayChoice10;
  uint8_t mapperNumber;
  uint8_t prgRamSize;
  TVSystem tvSystem;
} HeaderINES;

typedef struct {
  HeaderINES header;
  uint8_t* trainer;
  uint8_t* prgRom;
  uint8_t* chrRom;
  uint8_t* instRom;
  uint8_t* pRom;
} INES;

typedef struct {
  uint8_t* data;
  uint32_t bytes;
} FileBinary;


INES nescartridge_loadRom(char* romPath);
bool nescartridge_isRomFile(char* fileName);
bool nescartridge_selectRom(char selectedRomPath[FILEIO_MAX_PATH_SIZE]);
INES nescartridge_parseRom(FileBinary* bin);

#endif
