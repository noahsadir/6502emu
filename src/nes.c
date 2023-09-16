/**
 * nes.c
 * 
 * @author Noah Sadir
 * @date 2023-08-01
 */

#include "include/nes.h"

INES cartridge;
uint8_t memoryMap[0xFFFF];

void nes_init(char* fsRoot) {
  cartridge = nescartridge_loadRom(fsRoot);
  nes_configureMemory();
  mos6502_init(&nes_cpuWrite, &nes_cpuRead);

  if (cartridge.header.disassemblyMode) {
    nes_disassemble("./debug/dasm.s");
    OVERLAY_MSG = "Dissasembly successful.";
    while (true) {
      SDL_KeyCode key;
      io_pollInput(&key);
      io_render();
    }
  } else {
    mos6502_interrupt_reset();
    while (true) {
      mos6502_step(NULL, &nes_finishedInstruction);
    }
  }

  io_panic("CPU halted unexpectedly.");
}

void nes_finishedInstruction(uint8_t cycles) {
  if (CONFIG_DEBUG.shouldDisplayDebugScreen) {
    char regString[128];
    sprintf(regString, 
      " A: %02X\n X: %02X\n Y: %02X\n S: %02X\n P: %02X\nPC: %04X",
      reg.a, reg.x, reg.y, reg.s, reg.p, reg.pc);
    OVERLAY_MSG = regString;
  }
  SDL_KeyCode key;
  io_pollInput(&key);
  io_render();
}

void nes_disassemble(char* filePath) {
  if (cartridge.header.mapperNumber == 0) {
    char assemblyLineString[64];
    uint8_t bytecodeCount = 0;
    uint32_t pc = 0x8000;
    uint32_t prgEnd = pc + (0x4000 * cartridge.header.prgRomSize);
    fileio_writeStringToFile(filePath, "", false);
    while (pc < prgEnd) {
      assemblyLineString[0] = '\0';
      mos6502_decode(NULL, assemblyLineString, &bytecodeCount, pc);
      strcat(assemblyLineString, "\n");
      fileio_writeStringToFile(filePath, assemblyLineString, true);
      pc += bytecodeCount;
    }
  }
}

void nes_configureMemory() {
  if (cartridge.header.mapperNumber == 0) {
    for (int i = 0; i < 0x8000; i++) {
      int cartridgeIndex = i % (cartridge.header.prgRomSize * 0x4000);
      memoryMap[0x8000 + i] = cartridge.prgRom[cartridgeIndex];
    }
  } else {
    io_panic("Unsupported mapper.");
  }
}

uint8_t nes_cpuRead(uint16_t addr) {
  return memoryMap[addr];
}

void nes_cpuWrite(uint16_t addr, uint8_t data) {

}
