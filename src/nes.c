/**
 * nes.c
 * 
 * @author Noah Sadir
 * @date 2023-08-01
 */

#include "include/nes.h"

INES cartridge;
uint8_t memoryMap[0x10000];
int32_t cpuCycles = 0;
uint32_t realFreq = 0;
bool resetPPUStat = false;

struct timeval t1, t2;

void nes_init(char* fsRoot) {
  cartridge = nescartridge_loadRom(fsRoot);
  
  nes_configureMemory();
  nesppu_init(&cartridge);
  mos6502_init(&nes_cpuWrite, &nes_cpuRead);

  if (cartridge.header.disassemblyMode) {
    nes_disassemble("./debug/dasm.s");
    OVERLAY_MSG = "Dissasembly successful.";
    while (true) {
      Keyboard key;
      io_pollInput(&key);
      io_render();
    }
  } else if (CONFIG_DEBUG.shouldDebugCPU) {
    nes_debugCPU();
  } else {
    if (CONFIG_DEBUG.shouldTraceInstructions) {
      fileio_writeStringToFile("./debug/trace.log", "", false);
    }
    nes_start();
  }

  io_panic("CPU halted unexpectedly.");
}

void nes_start() {
  mos6502_interrupt_reset();
  cpuCycles += 4;
  uint32_t elapsed = 0;
  uint32_t cyclesPerInterval = CONFIG_CPU.frequency / INTERVALS_PER_SEC;
  int32_t realUs = 0;
  uint32_t intervals = 0;
  char outputStr[512];

#if (SUPPRESS_TIMING)
  while (true) {
    mos6502_step(NULL, &nes_finishedInstruction);
    Keyboard key;
    io_pollInput(&key);
    io_render();
  }
#else
  gettimeofday(&t1, 0);
  while (true) {
    // perform desired number of cpu cycles per ms
    while (cpuCycles < cyclesPerInterval) {
      if (CONFIG_DEBUG.shouldTraceInstructions) {
        if (CONFIG_CPU.shouldCacheInstructions) {
          io_panic("Cannot trace with caching.");
        }
        char trace[256];
        trace[0] = '\0';
        mos6502_step(trace, &nes_finishedInstruction);
        sprintf(trace, "%s\n", trace);
        fileio_writeStringToFile("./debug/trace.log", trace, true);
      } else {
        mos6502_step(NULL, &nes_finishedInstruction);
      }
    }

    // update metrics every second
    if (intervals == INTERVALS_PER_SEC) {
      nes_generateMetrics(outputStr);
      realFreq = 0;
      intervals = 0;
    }

    // perform I/O updates every interval
    if (realUs > TIMING_INTERVAL) {
      Keyboard key;
      io_pollInput(&key);
      io_render();
      realUs -= TIMING_INTERVAL;
      intervals += 1;
    }

    // delay if necessary
    gettimeofday(&t2, 0);
    elapsed = ((t2.tv_sec - t1.tv_sec) * 1000000) + t2.tv_usec - t1.tv_usec;
    if (CONFIG_DEBUG.shouldLimitFrequency) {
      usleep(elapsed < TIMING_INTERVAL ? usleep(TIMING_INTERVAL - elapsed) : 0);
    }

    // perform time-related calculations
    gettimeofday(&t2, 0);
    elapsed = ((t2.tv_sec - t1.tv_sec) * 1000000) + t2.tv_usec - t1.tv_usec;
    realUs += elapsed;
    realFreq += cpuCycles;
    cpuCycles -= cyclesPerInterval;
    gettimeofday(&t1, 0);
  }
#endif
}

void nes_finishedInstruction(uint8_t cycles) {
  if (cycles == 0) {
    io_panic("Illegal instruction.");
  }
  cpuCycles += cycles;
  if (resetPPUStat) {
    // wait until end of instruction before resetting PPU stat
    uint16_t addr = 0x2002;
    ppureg.ppustatus = SET_ppustat_vblankstarted(ppureg.ppustatus, 0);
    ppureg.addrLatch = false;
    ppureg.scrollLatch = false;
    while (addr <= 0x3FFF) {
      memoryMap[addr] = ppureg.ppustatus;
      addr += 0x08;
    }
    resetPPUStat = false;
  }
  nesppu_step(cycles * 3, &mos6502_interrupt_nmi);
}

void nes_configureMemory() {
  if (cartridge.header.mapperNumber == 0) {
    for (int i = 0; i < 0x2000; i++) {
      ppuMemoryMap[i] = cartridge.chrRom[i];
    }
    for (int i = 0; i < 0x8000; i++) {
      int cartridgeIndex = i % (cartridge.header.prgRomSize * 0x4000);
      memoryMap[0x8000 + i] = cartridge.prgRom[cartridgeIndex];
    }
  } else {
    io_panic("Unsupported mapper.");
  }
}

uint8_t nes_cpuRead(uint16_t addr) {
  // handle special case of reading from ppustat
  if (addr >= 0x2000 && addr <= 0x3FFF) {
    addr = 0x2000 + (addr % 0x08);
    if (addr == 0x2002) {
      resetPPUStat = true;
      return ppureg.ppustatus;
    } else if (addr == 0x2004) {
      return oam[ppureg.oamaddr];
    } else if (addr == 0x2007) {
      return nesppu_read(ppureg.loadedAddr);
    }
  }
  return memoryMap[addr];
}

void nes_cpuWrite(uint16_t addr, uint8_t data) {
  if (addr <= 0x1FFF) {
    addr = addr % 0x0800;
    memoryMap[addr] = data;
    memoryMap[addr + 0x0800] = data;
    memoryMap[addr + 0x1000] = data;
    memoryMap[addr + 0x1800] = data;
  } else if (addr <= 0x3FFF) {
    addr = 0x2000 + (addr % 0x08);
    if (addr == 0x2000) {
      ppureg.ppuctrl = data;
    } else if (addr == 0x2001) {
      ppureg.ppumask = data;
    } else if (addr == 0x2002) {
      // not writeable
    } else if (addr == 0x2003) {
      ppureg.oamaddr = data;
    } else if (addr == 0x2004) {
      ppureg.oamdata = data;
      while (addr <= 0x3FFF) {
        memoryMap[addr] = ppureg.oamdata;
        addr += 0x08;
      }
    } else if (addr == 0x2005) {
      if (!ppureg.scrollLatch) {
        ppureg.scrollX = data;
      } else {
        ppureg.scrollY = data;
      }
      ppureg.scrollLatch = !ppureg.scrollLatch;
    } else if (addr == 0x2006) {
      if (!ppureg.addrLatch) {
        ppureg.loadedAddr &= 0x00FF;
        ppureg.loadedAddr |= ((uint16_t)data) << 8;
      } else {
        ppureg.loadedAddr &= 0xFF00;
        ppureg.loadedAddr |= (uint16_t)data;
      }
      ppureg.ppuaddr = data;
      ppureg.addrLatch = !ppureg.addrLatch;
    } else if (addr == 0x2007) {
      nesppu_write(ppureg.loadedAddr & 0x3FFF, data);
      ppureg.loadedAddr += (GET_ppuctrl_vraminc(ppureg.ppuctrl) ? 32 : 1);
    }
  } else if (addr <= 0x4017) {
    if (addr == 0x4014) {
      ppureg.oamdma = data;
      uint16_t cpuAddr = ((uint16_t)data) << 8;
      for (int i = 0; i < 256; i++) {
        oam[i] = memoryMap[cpuAddr + i];
      }
    } else if (addr == 0x4017) {
      memoryMap[addr] = data;
    }
  } else if (addr <= 0x401F) {
    return;
  } else if (addr <= 0xFFFF) {
    // cartridge space
  }
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
      mos6502_decode_external_wrapper(NULL, assemblyLineString, &bytecodeCount, pc);
      strcat(assemblyLineString, "\n");
      fileio_writeStringToFile(filePath, assemblyLineString, true);
      pc += bytecodeCount;
    }
  }
}

void nes_generateMetrics(char* outputStr) {
#if (!SUPPRESS_EXTIO)
  if (CONFIG_DEBUG.shouldDisplayPerformance) {
    outputStr[0] = '\0';
    char perfString[128];
    sprintf(perfString, "%.6f MHz\n\n", (double)realFreq / 1000000.0);
    char regString[256];
    sprintf(regString, 
      "CPU\n----\n A: %02X\n X: %02X\n Y: %02X\n S: %02X\n P: %02X\nPC: %04X\n\nPPU\n----\n         VPHBSINN\nPPUCTRL: %d%d%d%d%d%d%d%d\n\n         BGRsbMmG\nPPUMASK: %d%d%d%d%d%d%d%d\n\n         VSO\nPPUSTAT: %d%d%d\n\nPPUADDR: %04X\nOAMADDR: %02X\nSCRLL-X: %d\nSCRLL-Y: %d",
      reg.a, reg.x, reg.y, reg.s, reg.p, reg.pc,
      GET_bit7(ppureg.ppuctrl), GET_bit6(ppureg.ppuctrl), GET_bit5(ppureg.ppuctrl), GET_bit4(ppureg.ppuctrl), GET_bit3(ppureg.ppuctrl), GET_bit2(ppureg.ppuctrl), GET_bit1(ppureg.ppuctrl), GET_bit0(ppureg.ppuctrl),
      GET_bit7(ppureg.ppumask), GET_bit6(ppureg.ppumask), GET_bit5(ppureg.ppumask), GET_bit4(ppureg.ppumask), GET_bit3(ppureg.ppumask), GET_bit2(ppureg.ppumask), GET_bit1(ppureg.ppumask), GET_bit0(ppureg.ppumask),
      GET_bit7(ppureg.ppustatus), GET_bit6(ppureg.ppustatus), GET_bit5(ppureg.ppustatus),
      ppureg.loadedAddr, ppureg.oamaddr, ppureg.scrollX, ppureg.scrollY
    );

    if (CONFIG_DEBUG.shouldDisplayPerformance) {
      strcat(outputStr, perfString);
    }

    if (CONFIG_DEBUG.shouldDisplayDebugScreen) {
      strcat(outputStr, regString);
    }

    OVERLAY_MSG = outputStr;
  }
#endif
}

void nes_debugCPU() {
  char traceStr[256];
  char fileStr[256];
  char* correctStr;
  char* nestestLogData;
  int lineNumber = 1;

  mos6502_interrupt_reset();
  reg.pc = 0xC000;
  cpuCycles += 4;

  fileio_readFileAsString("./debug/nestest.log", &nestestLogData);
  fileio_writeStringToFile("./debug/nestest-gen.log", "", false);
  mos6502_step(traceStr, &nes_finishedInstruction);
  correctStr = strtok(nestestLogData, "\n");

#if (!SUPPRESS_EXTIO)
  printf("%05d: %s, CYC:%d\n", lineNumber, traceStr, cpuCycles);
  sprintf(fileStr, "%05d: %s, CYC:%d\n", lineNumber, traceStr, cpuCycles);
  fileio_writeStringToFile("./debug/nestest-gen.log", fileStr, true);
#endif

  while (strncmp(traceStr, correctStr, 73) == 0) {
    mos6502_step(traceStr, &nes_finishedInstruction);
    correctStr = strtok(NULL, "\n");
    lineNumber += 1;

#if (!SUPPRESS_EXTIO)
    printf("%05d: %s, CYC:%d\n", lineNumber, traceStr, cpuCycles);
    sprintf(fileStr, "%05d: %s, CYC:%d\n", lineNumber, traceStr, cpuCycles);
    fileio_writeStringToFile("./debug/nestest-gen.log", fileStr, true);
#endif

  }

#if (!SUPPRESS_EXTIO)
  printf("\nMISMATCH AT LINE %d\n", lineNumber);
  printf("EXPECTED: `%.73s`\n", correctStr);
  printf("RECEIVED: `%.73s`\n", traceStr);
#endif

  io_panic("CPU fault.");

}
