/**
 * nescartridge.c
 * 
 * @author Noah Sadir
 * @date 2023-08-01
 */

#include "include/nescartridge.h"

INES nescartridge_loadRom(char* fsRoot) {
  char selectedRomPath[FILEIO_MAX_NAME_SIZE];
  selectedRomPath[0] = '\0';
  strcat(selectedRomPath, fsRoot);
  for (int i = 0; i < (CONFIG_DISPLAY.width * CONFIG_DISPLAY.height); i += 1) {
    BITMAP0[i] = i;
  }
  bool dasmMode = nescartridge_selectRom(selectedRomPath);
  io_clear();

  FileBinary bin;
  uint8_t* romBinary;
  if ((bin.bytes = fileio_readFileAsBinary(selectedRomPath, &romBinary)) == -1) {
    io_panic("Unable to read ROM file.");
  }
  bin.data = romBinary;
  INES cartridge = nescartridge_parseRom(&bin);
  cartridge.header.disassemblyMode = dasmMode;
  free(romBinary);
  return cartridge;
}

INES nescartridge_parseRom(FileBinary* bin) {
  INES cartridge;
  uint32_t pos = 0;
  if (bin->bytes < 16) io_panic("Unable to parse ROM file.");

  // parse header
  HeaderINES header;
  header.prgRomSize = bin->data[4];
  header.chrRomSize = bin->data[5];
  uint8_t flags6 = bin->data[6];
  header.mirroringType = (flags6 & BIT_FILL_1) ? MIRRORING_VERTICAL : MIRRORING_HORIZONTAL;
  header.containsPrgRam = ((flags6 >> 1) & BIT_FILL_1);
  header.containsTrainer = ((flags6 >> 2) & BIT_FILL_1);
  header.ignoreMirroringControl = ((flags6 >> 3) & BIT_FILL_1);
  header.mapperNumber = (flags6 >> 4) & BIT_FILL_4;
  uint8_t flags7 = bin->data[7];
  header.isVSUnisystem = (flags7 & BIT_FILL_1);
  header.isPlayChoice10 = ((flags7 >> 1) & BIT_FILL_1);
  header.mapperNumber |= (flags7 & 0xF0);
  header.prgRamSize = bin->data[8];
  header.tvSystem = (bin->data[9] & BIT_FILL_1) ? TV_PAL : TV_NTSC;
  cartridge.header = header;
  pos = 16;

  // load trainer, if present
  if (header.containsTrainer) {
    cartridge.trainer = malloc(sizeof(uint8_t) * 512);
    for (int i = 0; i < 512; i++) {
      cartridge.trainer[i] = bin->data[pos];
      pos += 1;
    }
  }

  // load prg rom (or fail if size mismatch)
  if ((pos + (header.prgRomSize * 16384)) > bin->bytes) io_panic("Unable to parse ROM file.");
  cartridge.prgRom = malloc(sizeof(uint8_t) * (header.prgRomSize * 16384));
  for (int i = 0; i < (header.prgRomSize * 16384); i++) {
    cartridge.prgRom[i] = bin->data[pos];
    pos += 1;
  }

  // load chr rom (or fail if size mismatch)
  if ((pos + (header.chrRomSize * 8192)) > bin->bytes) io_panic("Unable to parse ROM file.");
  cartridge.chrRom = malloc(sizeof(uint8_t) * (header.chrRomSize * 8192));
  for (int i = 0; i < (header.chrRomSize * 8192); i++) {
    cartridge.chrRom[i] = bin->data[pos];
    pos += 1;
  }

  if (header.containsPrgRam) {
    io_panic("PRG RAM unsupported.");
    //prgRAM = malloc(sizeof(uint8_t) * (header.prgRamSize * 8192));
  }

  return cartridge;
}

bool nescartridge_selectRom(char selectedRomPath[FILEIO_MAX_PATH_SIZE]) {
  char outputMessage[1024];
  char* files[FILEIO_MAX_COUNT];
  char* romFiles[FILEIO_MAX_COUNT];
  int romCount = 0;

  if (fileio_listFilesInPath(selectedRomPath, files) != -1) {
    for (int i  = 0; files[i] != NULL; i++) {
      if (nescartridge_isRomFile(files[i])) {
        romFiles[romCount] = files[i];
        romFiles[romCount + 1] = NULL;
        romCount += 1;
      }
    }
  }

  SDL_KeyCode key = SDLK_0;
  int selectedIndex = 0;
  bool keyPressed = false;
  bool disassembleActivated = false;
  while (key != SDLK_RETURN) {
    if (io_pollInput(&key) == 1) {
      keyPressed = true;
    } else if (io_pollInput(&key) == 0) {
      keyPressed = false;
    }

    if (keyPressed && key == SDLK_UP && selectedIndex > 0) {
      selectedIndex -= 1;
    } else if (keyPressed && key == SDLK_DOWN && selectedIndex < romCount - 1) {
      selectedIndex += 1;
    } else if (keyPressed && key == SDLK_d) {
      disassembleActivated = !disassembleActivated;
    }

    outputMessage[0] = '\0';
    strcat(outputMessage, "\n\tSelect a ROM:\n");

    if (disassembleActivated) {
      strcat(outputMessage, "\tCPU Mode: DASM\n");
    } else {
      strcat(outputMessage, "\tCPU Mode: EXEC\n");
    }

    for (int i = 0; i < romCount; i++) {
      strcat(outputMessage, "\n\t[");
      strcat(outputMessage, (selectedIndex == i) ? "*] " : " ] ");
      strcat(outputMessage, romFiles[i]);
    }
    OVERLAY_MSG = outputMessage;
    io_render();
  }

  strncat(selectedRomPath, romFiles[selectedIndex], FILEIO_MAX_NAME_SIZE);

  return disassembleActivated;
}

bool nescartridge_isRomFile(char* fileName) {
  int len = strlen(fileName);
  if (len >= 4) {
    return (
      strcmp(fileName + len - 4, ".nes") == 0
      || strcmp(fileName + len - 4, ".NES") == 0
    );
  }
  return false;
} 
