/**
 * mos6502.c
 * 
 * @author Noah Sadir
 * @date 2023-08-03
 */

#include "include/mos6502.h"

CPURegisters reg;
BytecodeProgram prgBytecode;

void(*memWrite)(uint16_t, uint8_t);
uint8_t(*memRead)(uint16_t);

CPUMnemonic mnemonicTable[0x100];
CPUAddressingMode addrModeTable[0x100];
uint8_t cycleTable[0x100];
char* mnemonicStringTable[80];

void mos6502_init(void(*w)(uint16_t, uint8_t), uint8_t(*r)(uint16_t)) {
  mos6502_configureTables();

  if (CONFIG_CPU.shouldCacheInstructions && !MINIMIZE_MEMORY) {
    for (int i = 0; i < 65536; i++) {
      prgBytecode.cacheMap[i] = false;
    }
  }

  memWrite = w;
  memRead = r;
  memWrite = w;
  memRead = r;

  reg.a = 0x00;
  reg.x = 0x00;
  reg.y = 0x00;
  reg.s = 0x00;
  reg.p = 0x24;
  reg.pc = 0x00;
}

void mos6502_step(char* traceStr, void(*c)(uint8_t)) {
  Bytecode* bytecode = NULL;
  if (CONFIG_CPU.shouldCacheInstructions && !MINIMIZE_MEMORY) {
    // ~20% performance savings observed w/ caching
    if (!prgBytecode.cacheMap[reg.pc]) {
      static Bytecode bc;
      bytecode = &bc;
      mos6502_decode(bytecode, NULL, NULL, reg.pc);
      prgBytecode.bytecodeCount += 1;
      prgBytecode.bytecodes[prgBytecode.bytecodeCount - 1] = bc;
      prgBytecode.addrMap[reg.pc] = prgBytecode.bytecodeCount - 1;
      prgBytecode.cacheMap[reg.pc] = true;
    }
    bytecode = prgBytecode.bytecodes + prgBytecode.addrMap[reg.pc];
  } else {
    Bytecode bc;
    bytecode = &bc;
    char asmStr[33];
    mos6502_decode(bytecode, asmStr, NULL, reg.pc);
    if (traceStr != NULL) {
      mos6502_generateTrace(traceStr, asmStr, bytecode);
    }
  }

  c(mos6502_execute(bytecode));
}

void mos6502_generateTrace(char* traceStr, char* asmStr, Bytecode* bytecode) {
#if (!SUPPRESS_EXTIO)
  char dataStr[9];
  char operandConvertStr[32];
  operandConvertStr[0] = '\0';

  if (bytecode->count == 1) {
    sprintf(dataStr, "%02X",
      bytecode->data[0]);
  } else if (bytecode->count == 2) {
    sprintf(dataStr, "%02X %02X",
      bytecode->data[0], bytecode->data[1]);
  } else if (bytecode->count == 3) {
    sprintf(dataStr, "%02X %02X %02X",
      bytecode->data[0], bytecode->data[1], bytecode->data[2]);
  }

  if (bytecode->addressingMode == AM_ZERO_PAGE) {
    sprintf(operandConvertStr, " = %02X", memRead(mos6502_fetchValue(bytecode)));
  } else if (bytecode->addressingMode == AM_ABSOLUTE) {
    if (bytecode->mnemonic != I_JMP && bytecode->mnemonic != I_JSR) {
      sprintf(operandConvertStr, " = %02X", memRead(mos6502_fetchValue(bytecode)));
    }
  } else if (bytecode->addressingMode == AM_ABS_X || bytecode->addressingMode == AM_ABS_Y) {
    if (bytecode->mnemonic != I_JMP && bytecode->mnemonic != I_JSR) {
      sprintf(operandConvertStr, " @ %04X = %02X", mos6502_fetchValue(bytecode), memRead(mos6502_fetchValue(bytecode)));
    }
  } else if (bytecode->addressingMode == AM_ZP_X || bytecode->addressingMode == AM_ZP_Y) {
    if (bytecode->mnemonic != I_JMP && bytecode->mnemonic != I_JSR) {
      sprintf(operandConvertStr, " @ %02X = %02X", mos6502_fetchValue(bytecode), memRead(mos6502_fetchValue(bytecode)));
    }
  } else if (bytecode->addressingMode == AM_ABS_INDIRECT) {
    sprintf(operandConvertStr, " = %04X", mos6502_fetchValue(bytecode));
  } else if (bytecode->addressingMode == AM_ZP_X_INDIRECT) {
    if (bytecode->mnemonic != I_JMP && bytecode->mnemonic != I_JSR) {
      sprintf(operandConvertStr, " @ %02X = %04X = %02X",
        (uint8_t)(bytecode->data[1] + reg.x),
        mos6502_fetchValue(bytecode),
        memRead(mos6502_fetchValue(bytecode)));
    }
  } else if (bytecode->addressingMode == AM_ZP_INDIRECT_Y) {
    if (bytecode->mnemonic != I_JMP && bytecode->mnemonic != I_JSR) {
      uint8_t addr = bytecode->data[1];
      uint8_t addrInc = addr + 1;
      uint16_t indaddr = ((uint16_t)memRead(addrInc) << 8) | (uint16_t)memRead(addr);
      sprintf(operandConvertStr, " = %04X @ %04X = %02X",
        indaddr,
        mos6502_fetchValue(bytecode),
        memRead(mos6502_fetchValue(bytecode)));
    }
  }

  strcat(asmStr, operandConvertStr);
  sprintf(traceStr, "%04X  %-8s %-32s A:%02X X:%02X Y:%02X P:%02X SP:%02X",
    reg.pc, dataStr, asmStr, reg.a, reg.x, reg.y, reg.p, reg.s);
#endif
}

force_inline uint16_t mos6502_read16(uint16_t addr) {
  return (((uint16_t)memRead(addr + 1)) << 8) | (uint16_t)memRead(addr);
}

force_inline void mos6502_stack_push(uint8_t data) {
  memWrite(0x0100 + (uint16_t)reg.s, data);
  reg.s -= 1;
}

force_inline uint8_t mos6502_stack_pop(void) {
  reg.s += 1;
  uint8_t val = memRead(0x0100 + (uint16_t)reg.s);
  return val;
}

void mos6502_interrupt_nmi(void) {
  mos6502_stack_push((uint8_t)(reg.pc >> 8));
  mos6502_stack_push((uint8_t)(reg.pc));
  mos6502_stack_push(reg.p & ~CPUSTAT_BREAK);
  reg.pc = mos6502_read16(0xFFFA);
}

void mos6502_interrupt_reset(void) {
  reg.s = 0xFD;
  reg.pc = mos6502_read16(0xFFFC);
}

void mos6502_interrupt_irq(void) {
  mos6502_stack_push((uint8_t)(reg.pc >> 8));
  mos6502_stack_push((uint8_t)(reg.pc));
  mos6502_stack_push(reg.p & ~CPUSTAT_BREAK);
  reg.pc = mos6502_read16(0xFFFE);
}

force_inline void mos6502_setflag(CPUStatusFlag flag, uint8_t value) {
  if (value == 0) {
    reg.p &= ~flag;
  } else {
    reg.p |= flag;
  }
  reg.p &= ~CPUSTAT_BREAK2;
}

force_inline uint8_t mos6502_getflag(CPUStatusFlag flag) {
  return (reg.p & flag) > 0;
}

force_inline uint16_t mos6502_fetchValue(Bytecode* bytecode) {
  switch (bytecode->addressingMode) {
    case AM_ACCUMULATOR: {
      return reg.a;
      break;
    }
    case AM_IMPLIED: {
      return 0;
      break;
    }
    case AM_IMMEDIATE: {
      return reg.pc + 1;
      break;
    }
    case AM_ABSOLUTE: {
      uint16_t addr = ((uint16_t)bytecode->data[2] << 8) | (uint16_t)bytecode->data[1];
      return addr;
      break;
    }
    case AM_ZERO_PAGE: {
      uint8_t addr = bytecode->data[1];
      return addr;
      break;
    }
    case AM_RELATIVE: {
      int8_t offset = bytecode->data[1];
      return (reg.pc + 2) + offset;
      break;
    }
    case AM_ABS_INDIRECT: {
      uint16_t addr = ((uint16_t)bytecode->data[2] << 8) | (uint16_t)bytecode->data[1];
      uint16_t low = memRead(addr);
      uint16_t high = ((addr & 0x00FF) == 0xFF) ? memRead(addr & 0xFF00) : memRead(addr + 1);
      uint16_t indaddr = (high << 8) | low;
      return indaddr;
      break;
    }
    case AM_ABS_X: {
      uint16_t addr = ((uint16_t)bytecode->data[2] << 8) | (uint16_t)bytecode->data[1];
      addr += reg.x;
      return addr;
      break;
    }
    case AM_ABS_Y: {
      uint16_t addr = ((uint16_t)bytecode->data[2] << 8) | (uint16_t)bytecode->data[1];
      addr += reg.y;
      return addr;
      break;
    }
    case AM_ZP_X: {
      uint8_t addr = bytecode->data[1];
      addr += reg.x;
      return addr;
      break;
    }
    case AM_ZP_Y: {
      uint8_t addr = bytecode->data[1];
      addr += reg.y;
      return addr;
      break;
    }
    case AM_ZP_X_INDIRECT: {
      uint8_t addr = bytecode->data[1];
      addr += reg.x;
      uint8_t addrInc = addr + 1;
      uint16_t indaddr = ((uint16_t)memRead(addrInc) << 8) | (uint16_t)memRead(addr);
      return indaddr;
      break;
    }
    case AM_ZP_INDIRECT_Y: {
      uint8_t addr = bytecode->data[1];
      uint8_t addrInc = addr + 1;
      uint16_t indaddr = ((uint16_t)memRead(addrInc) << 8) | (uint16_t)memRead(addr);
      indaddr += reg.y;
      return indaddr;
      break;
    }
    default: break;
  }
  return 0;
}

void mos6502_decode_external_wrapper(Bytecode* bytecode, char* assemblyResult, uint8_t* byteCount, uint16_t pc) {
  mos6502_decode(bytecode, assemblyResult, byteCount, pc);
}

force_inline void mos6502_decode(Bytecode* bytecode, char* assemblyResult, uint8_t* byteCount, uint16_t pc) {
  uint8_t opcode = memRead(pc);
  CPUAddressingMode addrMode = addrModeTable[opcode];
  CPUMnemonic mnemonic = mnemonicTable[opcode];
  uint8_t bytes = 0;

  switch (addrMode) {
    case AM_UNSET: bytes = 1; break;
    case AM_ACCUMULATOR: bytes = 1; break;
    case AM_IMPLIED: bytes = 1; break;
    case AM_IMMEDIATE: bytes = 2; break;
    case AM_ABSOLUTE: bytes = 3; break;
    case AM_ZERO_PAGE: bytes = 2; break;
    case AM_RELATIVE: bytes = 2; break;
    case AM_ABS_INDIRECT: bytes = 3; break;
    case AM_ABS_X: bytes = 3; break;
    case AM_ABS_Y: bytes = 3; break;
    case AM_ZP_X: bytes = 2; break;
    case AM_ZP_Y: bytes = 2; break;
    case AM_ZP_INDIRECT_Y: bytes = 2; break;
    case AM_ZP_X_INDIRECT: bytes = 2; break;
    default: bytes = 1; break;
  }

  if (bytecode != NULL) {
    bytecode->addressingMode = addrMode;
    bytecode->mnemonic = mnemonic;
    bytecode->count = bytes;
    bytecode->data[0] = opcode;
    for (int i = 1; i < bytes; i++) {
      bytecode->data[i] = memRead(pc + i);
    }
  }

  if (byteCount != NULL) {
    *byteCount = bytes;
  }

#if (!SUPPRESS_EXTIO)
  if (assemblyResult != NULL && CONFIG_DEBUG.shouldTraceInstructions) {
    char operandString[64];

    switch (addrMode) {
      case AM_UNSET: sprintf(operandString, ""); break;
      case AM_ACCUMULATOR: sprintf(operandString, "A"); break;
      case AM_IMPLIED: sprintf(operandString, ""); break;
      case AM_IMMEDIATE: sprintf(operandString, "#$%02X", memRead(pc + 1)); break;
      case AM_ABSOLUTE: sprintf(operandString, "$%02X%02X", memRead(pc + 2), memRead(pc + 1));  break;
      case AM_ZERO_PAGE: sprintf(operandString, "$%02X", memRead(pc + 1)); break;
      case AM_RELATIVE: sprintf(operandString, "$%02X", (pc + 2) + (int8_t)memRead(pc + 1)); break;
      case AM_ABS_INDIRECT: sprintf(operandString, "($%02X%02X)", memRead(pc + 2), memRead(pc + 1)); break;
      case AM_ABS_X: sprintf(operandString, "$%02X%02X,X", memRead(pc + 2), memRead(pc + 1)); break;
      case AM_ABS_Y: sprintf(operandString, "$%02X%02X,Y", memRead(pc + 2), memRead(pc + 1)); break;
      case AM_ZP_X: sprintf(operandString, "$%02X,X", memRead(pc + 1)); break;
      case AM_ZP_Y: sprintf(operandString, "$%02X,Y", memRead(pc + 1)); break;
      case AM_ZP_INDIRECT_Y: sprintf(operandString, "($%02X),Y", memRead(pc + 1)); break;
      case AM_ZP_X_INDIRECT: sprintf(operandString, "($%02X,X)", memRead(pc + 1)); break;
      default: sprintf(operandString, ""); break;
    }

    sprintf(assemblyResult, "%*s %s", 4, mnemonicStringTable[mnemonic], operandString);
  }
#endif
}

force_inline uint8_t mos6502_execute(Bytecode* bytecode) {
  uint16_t operand = mos6502_fetchValue(bytecode);
  uint8_t cycles = cycleTable[bytecode->data[0]];

  switch (bytecode->mnemonic) {
    case I_ADC: {
      uint16_t m = memRead(operand);
      uint16_t sum = reg.a + m + mos6502_getflag(CPUSTAT_CARRY);
      mos6502_setflag(CPUSTAT_CARRY, sum > 0xFF);
      mos6502_setflag(CPUSTAT_OVERFLOW, (reg.a ^ sum) & (m ^ sum) & 0x80);
      reg.a = sum;
      mos6502_setflag(CPUSTAT_ZERO, reg.a == 0);
      mos6502_setflag(CPUSTAT_NEGATIVE, (reg.a & BIT_MASK_7) != 0);
      reg.pc += bytecode->count;
      break;
    }
    case I_AND: {
      uint16_t m = memRead(operand);
      reg.a = reg.a & m;
      mos6502_setflag(CPUSTAT_ZERO, reg.a == 0);
      mos6502_setflag(CPUSTAT_NEGATIVE, (reg.a & BIT_MASK_7) != 0);
      reg.pc += bytecode->count;
      break;
    }
    case I_ASL: {
      uint8_t val = (bytecode->addressingMode == AM_ACCUMULATOR) ? reg.a : memRead(operand);
      mos6502_setflag(CPUSTAT_CARRY, (val & BIT_MASK_7) != 0);
      val <<= 1;
      mos6502_setflag(CPUSTAT_ZERO, val == 0);
      mos6502_setflag(CPUSTAT_NEGATIVE, (val & BIT_MASK_7) != 0);
      if (bytecode->addressingMode == AM_ACCUMULATOR) {
        reg.a = val;
      } else {
        memWrite(operand, val);
      }
      reg.pc += bytecode->count;
      break;
    }
    case I_BCC: {
      if (mos6502_getflag(CPUSTAT_CARRY) == 0) {
        reg.pc = operand;
        cycles += 1;
      } else {
        reg.pc += bytecode->count;
      }
      break;
    }
    case I_BCS: {
      if (mos6502_getflag(CPUSTAT_CARRY) == 1) {
        reg.pc = operand;
        cycles += 1;
      } else {
        reg.pc += bytecode->count;
      }
      break;
    }
    case I_BEQ: {
      if (mos6502_getflag(CPUSTAT_ZERO) == 1) {
        reg.pc = operand;
        cycles += 1;
      } else {
        reg.pc += bytecode->count;
      }
      break;
    }
    case I_BIT: {
      uint16_t m = memRead(operand);
      mos6502_setflag(CPUSTAT_ZERO, (reg.a & m) == 0);
      mos6502_setflag(CPUSTAT_NEGATIVE, (m & BIT_MASK_7) > 0);
      mos6502_setflag(CPUSTAT_OVERFLOW, (m & BIT_MASK_6) > 0);
      reg.pc += bytecode->count;
      break;
    }
    case I_BMI: {
      if (mos6502_getflag(CPUSTAT_NEGATIVE) == 1) {
        reg.pc = operand;
        cycles += 1;
      } else {
        reg.pc += bytecode->count;
      }
      break;
    }
    case I_BNE: {
      if (mos6502_getflag(CPUSTAT_ZERO) == 0) {
        reg.pc = operand;
        cycles += 1;
      } else {
        reg.pc += bytecode->count;
      }
      break;
    }
    case I_BPL: {
      if (mos6502_getflag(CPUSTAT_NEGATIVE) == 0) {
        reg.pc = operand;
        cycles += 1;
      } else {
        reg.pc += bytecode->count;
      }
      break;
    }
    case I_BRK: {
      reg.pc += 2;
      mos6502_stack_push((uint8_t)(reg.pc >> 8));
      mos6502_stack_push((uint8_t)(reg.pc));
      mos6502_stack_push(reg.p | CPUSTAT_BREAK);
      reg.pc = mos6502_read16(0xFFFE);
      mos6502_setflag(CPUSTAT_NO_INTRPT, 1);
      break;
    }
    case I_BVC: {
      if (mos6502_getflag(CPUSTAT_OVERFLOW) == 0) {
        reg.pc = operand;
        cycles += 1;
      } else {
        reg.pc += bytecode->count;
      }
      break;
    }
    case I_BVS: {
      if (mos6502_getflag(CPUSTAT_OVERFLOW) == 1) {
        reg.pc = operand;
        cycles += 1;
      } else {
        reg.pc += bytecode->count;
      }
      break;
    }
    case I_CLC: {
      mos6502_setflag(CPUSTAT_CARRY, 0);
      reg.pc += bytecode->count;
      break;
    }
    case I_CLD: {
      mos6502_setflag(CPUSTAT_DECIMAL, 0);
      reg.pc += bytecode->count;
      break;
    }
    case I_CLI: {
      mos6502_setflag(CPUSTAT_NO_INTRPT, 0);
      reg.pc += bytecode->count;
      break;
    }
    case I_CLV: {
      mos6502_setflag(CPUSTAT_OVERFLOW, 0);
      reg.pc += bytecode->count;
      break;
    }
    case I_CMP: {
      uint8_t m = memRead(operand);
      int8_t val = (int8_t)reg.a - (int8_t)m;

      if (reg.a < m) {
        mos6502_setflag(CPUSTAT_ZERO, 0);
        mos6502_setflag(CPUSTAT_CARRY, 0);
      } else if (reg.a == m) {
        mos6502_setflag(CPUSTAT_ZERO, 1);
        mos6502_setflag(CPUSTAT_CARRY, 1);
      } else if (reg.a > m) {
        mos6502_setflag(CPUSTAT_ZERO, 0);
        mos6502_setflag(CPUSTAT_CARRY, 1);
      }

      mos6502_setflag(CPUSTAT_NEGATIVE, val < 0);
      reg.pc += bytecode->count;
      break;
    }
    case I_CPX: {
      uint8_t m = memRead(operand);
      int8_t val = (int8_t)reg.x - (int8_t)m;

      if (reg.x < m) {
        mos6502_setflag(CPUSTAT_ZERO, 0);
        mos6502_setflag(CPUSTAT_CARRY, 0);
      } else if (reg.x == m) {
        mos6502_setflag(CPUSTAT_ZERO, 1);
        mos6502_setflag(CPUSTAT_CARRY, 1);
      } else if (reg.x > m) {
        mos6502_setflag(CPUSTAT_ZERO, 0);
        mos6502_setflag(CPUSTAT_CARRY, 1);
      }

      mos6502_setflag(CPUSTAT_NEGATIVE, val < 0);
      reg.pc += bytecode->count;
      break;
    }
    case I_CPY: {
      uint8_t m = memRead(operand);
      int8_t val = (int8_t)reg.y - (int8_t)m;

      if (reg.y < m) {
        mos6502_setflag(CPUSTAT_ZERO, 0);
        mos6502_setflag(CPUSTAT_CARRY, 0);
      } else if (reg.y == m) {
        mos6502_setflag(CPUSTAT_ZERO, 1);
        mos6502_setflag(CPUSTAT_CARRY, 1);
      } else if (reg.y > m) {
        mos6502_setflag(CPUSTAT_ZERO, 0);
        mos6502_setflag(CPUSTAT_CARRY, 1);
      }

      mos6502_setflag(CPUSTAT_NEGATIVE, val < 0);
      reg.pc += bytecode->count;
      break;
    }
    case I_DEC: {
      uint8_t m = memRead(operand);
      m -= 1;
      memWrite(operand, m);
      mos6502_setflag(CPUSTAT_ZERO, m == 0);
      mos6502_setflag(CPUSTAT_NEGATIVE, (m & BIT_MASK_7) > 0);
      reg.pc += bytecode->count;
      break;
    }
    case I_DEX: {
      reg.x -= 1;
      mos6502_setflag(CPUSTAT_ZERO, reg.x == 0);
      mos6502_setflag(CPUSTAT_NEGATIVE, (reg.x & BIT_MASK_7) > 0);
      reg.pc += bytecode->count;
      break;
    }
    case I_DEY: {
      reg.y -= 1;
      mos6502_setflag(CPUSTAT_ZERO, reg.y == 0);
      mos6502_setflag(CPUSTAT_NEGATIVE, (reg.y & BIT_MASK_7) > 0);
      reg.pc += bytecode->count;
      break;
    }
    case I_EOR: {
      uint16_t m = memRead(operand);
      reg.a = reg.a ^ m;
      mos6502_setflag(CPUSTAT_ZERO, reg.a == 0);
      mos6502_setflag(CPUSTAT_NEGATIVE, (reg.a & BIT_MASK_7) != 0);
      reg.pc += bytecode->count;
      break;
    }
    case I_INC: {
      uint8_t m = memRead(operand);
      m += 1;
      memWrite(operand, m);
      mos6502_setflag(CPUSTAT_ZERO, m == 0);
      mos6502_setflag(CPUSTAT_NEGATIVE, (m & BIT_MASK_7) > 0);
      reg.pc += bytecode->count;
      break;
    }
    case I_INX: {
      reg.x += 1;
      mos6502_setflag(CPUSTAT_ZERO, reg.x == 0);
      mos6502_setflag(CPUSTAT_NEGATIVE, (reg.x & BIT_MASK_7) > 0);
      reg.pc += bytecode->count;
      break;
    }
    case I_INY: {
      reg.y += 1;
      mos6502_setflag(CPUSTAT_ZERO, reg.y == 0);
      mos6502_setflag(CPUSTAT_NEGATIVE, (reg.y & BIT_MASK_7) > 0);
      reg.pc += bytecode->count;
      break;
    }
    case I_JMP: {
      reg.pc = operand;
      break;
    }
    case I_JSR: {
      reg.pc += 2;
      mos6502_stack_push((uint8_t)(reg.pc >> 8));
      mos6502_stack_push((uint8_t)(reg.pc));
      reg.pc = operand;
      break;
    }
    case I_LDA: {
      uint8_t m = memRead(operand);
      reg.a = m;
      mos6502_setflag(CPUSTAT_ZERO, m == 0);
      mos6502_setflag(CPUSTAT_NEGATIVE, (m & BIT_MASK_7) > 0);
      reg.pc += bytecode->count;
      break;
    }
    case I_LDX: {
      uint8_t m = memRead(operand);
      reg.x = m;
      mos6502_setflag(CPUSTAT_ZERO, m == 0);
      mos6502_setflag(CPUSTAT_NEGATIVE, (m & BIT_MASK_7) > 0);
      reg.pc += bytecode->count;
      break;
    }
    case I_LDY: {
      uint8_t m = memRead(operand);
      reg.y = m;
      mos6502_setflag(CPUSTAT_ZERO, m == 0);
      mos6502_setflag(CPUSTAT_NEGATIVE, (m & BIT_MASK_7) > 0);
      reg.pc += bytecode->count;
      break;
    }
    case I_LSR: {
      uint8_t val = (bytecode->addressingMode == AM_ACCUMULATOR) ? reg.a : memRead(operand);
      mos6502_setflag(CPUSTAT_CARRY, (val & BIT_MASK_0) != 0);
      val >>= 1;
      mos6502_setflag(CPUSTAT_ZERO, val == 0);
      mos6502_setflag(CPUSTAT_NEGATIVE, (val & BIT_MASK_7) != 0);
      if (bytecode->addressingMode == AM_ACCUMULATOR) {
        reg.a = val;
      } else {
        memWrite(operand, val);
      }
      reg.pc += bytecode->count;
      break;
    }
    case I_NOP: {
      reg.pc += bytecode->count;
      break;
    }
    case I_ORA: {
      uint16_t m = memRead(operand);
      reg.a = reg.a | m;
      mos6502_setflag(CPUSTAT_ZERO, reg.a == 0);
      mos6502_setflag(CPUSTAT_NEGATIVE, (reg.a & BIT_MASK_7) != 0);
      reg.pc += bytecode->count;
      break;
    }
    case I_PHA: {
      mos6502_stack_push(reg.a);
      reg.pc += bytecode->count;
      break;
    }
    case I_PHP: {
      mos6502_stack_push(reg.p | CPUSTAT_BREAK | CPUSTAT_BREAK2);
      reg.pc += bytecode->count;
      break;
    }
    case I_PLA: {
      reg.a = mos6502_stack_pop();
      mos6502_setflag(CPUSTAT_ZERO, reg.a == 0);
      mos6502_setflag(CPUSTAT_NEGATIVE, (reg.a & BIT_MASK_7) != 0);
      reg.pc += bytecode->count;
      break;
    }
    case I_PLP: {
      uint8_t newP = mos6502_stack_pop() & ~(CPUSTAT_BREAK | CPUSTAT_BREAK2);
      reg.p &= (CPUSTAT_BREAK | CPUSTAT_BREAK2);
      reg.p |= newP;
      reg.pc += bytecode->count;
      break;
    }
    case I_ROL: {
      uint8_t val = (bytecode->addressingMode == AM_ACCUMULATOR) ? reg.a : memRead(operand);
      uint8_t oldCarry = mos6502_getflag(CPUSTAT_CARRY);
      mos6502_setflag(CPUSTAT_CARRY, (val & BIT_MASK_7) != 0);
      val <<= 1;
      val |= oldCarry;
      mos6502_setflag(CPUSTAT_ZERO, val == 0);
      mos6502_setflag(CPUSTAT_NEGATIVE, (val & BIT_MASK_7) != 0);
      if (bytecode->addressingMode == AM_ACCUMULATOR) {
        reg.a = val;
      } else {
        memWrite(operand, val);
      }
      reg.pc += bytecode->count;
      break;
    }
    case I_ROR: {
      uint8_t val = (bytecode->addressingMode == AM_ACCUMULATOR) ? reg.a : memRead(operand);
      uint8_t oldCarry = mos6502_getflag(CPUSTAT_CARRY);
      mos6502_setflag(CPUSTAT_CARRY, (val & BIT_MASK_0) != 0);
      val >>= 1;
      val |= (oldCarry << 7);
      mos6502_setflag(CPUSTAT_ZERO, val == 0);
      mos6502_setflag(CPUSTAT_NEGATIVE, (val & BIT_MASK_7) != 0);
      if (bytecode->addressingMode == AM_ACCUMULATOR) {
        reg.a = val;
      } else {
        memWrite(operand, val);
      }
      reg.pc += bytecode->count;
      break;
    }
    case I_RTI: {
      uint8_t newP = mos6502_stack_pop() & ~(CPUSTAT_BREAK | CPUSTAT_BREAK2);
      reg.p &= (CPUSTAT_BREAK | CPUSTAT_BREAK2);
      reg.p |= newP;
      uint16_t low = mos6502_stack_pop();
      uint16_t high = mos6502_stack_pop();
      reg.pc = (high << 8) | low;
      break;
    }
    case I_RTS: {
      uint16_t low = mos6502_stack_pop();
      uint16_t high = mos6502_stack_pop();
      reg.pc = (high << 8) | low;
      reg.pc += 1;
      break;
    }
    case I_SBC: {
      uint8_t m = ~memRead(operand);
      uint16_t sum = reg.a + m + mos6502_getflag(CPUSTAT_CARRY);
      mos6502_setflag(CPUSTAT_CARRY, sum > 0xFF);
      mos6502_setflag(CPUSTAT_OVERFLOW, (reg.a ^ sum) & (m ^ sum) & 0x80);
      reg.a = sum;
      mos6502_setflag(CPUSTAT_ZERO, reg.a == 0);
      mos6502_setflag(CPUSTAT_NEGATIVE, (reg.a & BIT_MASK_7) != 0);
      reg.pc += bytecode->count;
      break;
    }
    case I_SEC: {
      mos6502_setflag(CPUSTAT_CARRY, 1);
      reg.pc += bytecode->count;
      break;
    }
    case I_SED: {
      mos6502_setflag(CPUSTAT_DECIMAL, 1);
      reg.pc += bytecode->count;
      break;
    }
    case I_SEI: {
      mos6502_setflag(CPUSTAT_NO_INTRPT, 1);
      reg.pc += bytecode->count;
      break;
    }
    case I_STA: {
      memWrite(operand, reg.a);
      reg.pc += bytecode->count;
      break;
    }
    case I_STX: {
      memWrite(operand, reg.x);
      reg.pc += bytecode->count;
      break;
    }
    case I_STY: {
      memWrite(operand, reg.y);
      reg.pc += bytecode->count;
      break;
    }
    case I_TAX: {
      reg.x = reg.a;
      mos6502_setflag(CPUSTAT_ZERO, reg.a == 0);
      mos6502_setflag(CPUSTAT_NEGATIVE, (reg.a & BIT_MASK_7) != 0);
      reg.pc += bytecode->count;
      break;
    }
    case I_TAY: {
      reg.y = reg.a;
      mos6502_setflag(CPUSTAT_ZERO, reg.a == 0);
      mos6502_setflag(CPUSTAT_NEGATIVE, (reg.a & BIT_MASK_7) != 0);
      reg.pc += bytecode->count;
      break;
    }
    case I_TSX: {
      reg.x = reg.s;
      mos6502_setflag(CPUSTAT_ZERO, reg.s == 0);
      mos6502_setflag(CPUSTAT_NEGATIVE, (reg.s & BIT_MASK_7) != 0);
      reg.pc += bytecode->count;
      break;
    }
    case I_TXA: {
      reg.a = reg.x;
      mos6502_setflag(CPUSTAT_ZERO, reg.x == 0);
      mos6502_setflag(CPUSTAT_NEGATIVE, (reg.x & BIT_MASK_7) != 0);
      reg.pc += bytecode->count;
      break;
    }
    case I_TXS: {
      reg.s = reg.x;
      reg.pc += bytecode->count;
      break;
    }
    case I_TYA: {
      reg.a = reg.y;
      mos6502_setflag(CPUSTAT_ZERO, reg.y == 0);
      mos6502_setflag(CPUSTAT_NEGATIVE, (reg.y & BIT_MASK_7) != 0);
      reg.pc += bytecode->count;
      break;
    }
    case I_ILL_ALR: {
      uint16_t m = memRead(operand);
      reg.a = reg.a & m;
      mos6502_setflag(CPUSTAT_ZERO, reg.a == 0);
      mos6502_setflag(CPUSTAT_NEGATIVE, (reg.a & BIT_MASK_7) != 0);
      uint8_t val = (bytecode->addressingMode == AM_ACCUMULATOR) ? reg.a : memRead(operand);
      mos6502_setflag(CPUSTAT_CARRY, (val & BIT_MASK_0) != 0);
      val >>= 1;
      mos6502_setflag(CPUSTAT_ZERO, val == 0);
      mos6502_setflag(CPUSTAT_NEGATIVE, (val & BIT_MASK_7) != 0);
      if (bytecode->addressingMode == AM_ACCUMULATOR) {
        reg.a = val;
      } else {
        memWrite(operand, val);
      }
      reg.pc += bytecode->count;
      break;
    }
    case I_ILL_ANC: {
      uint16_t m = memRead(operand);
      reg.a = reg.a & m;
      mos6502_setflag(CPUSTAT_ZERO, reg.a == 0);
      mos6502_setflag(CPUSTAT_NEGATIVE, (reg.a & BIT_MASK_7) != 0);
      mos6502_setflag(CPUSTAT_CARRY, (reg.a & BIT_MASK_7) != 0);
      break;
    }
    case I_ILL_ANC2: {
      uint16_t m = memRead(operand);
      reg.a = reg.a & m;
      mos6502_setflag(CPUSTAT_ZERO, reg.a == 0);
      mos6502_setflag(CPUSTAT_NEGATIVE, (reg.a & BIT_MASK_7) != 0);
      mos6502_setflag(CPUSTAT_CARRY, (reg.a & BIT_MASK_7) != 0);
      break;
    }
    case I_ILL_ANE: {
      return 0;
      break;
    }
    case I_ILL_ARR: {
      uint16_t m = memRead(operand);
      reg.a = reg.a & m;
      mos6502_setflag(CPUSTAT_ZERO, reg.a == 0);
      mos6502_setflag(CPUSTAT_NEGATIVE, (reg.a & BIT_MASK_7) != 0);
      uint8_t val = (bytecode->addressingMode == AM_ACCUMULATOR) ? reg.a : memRead(operand);
      uint8_t oldCarry = mos6502_getflag(CPUSTAT_CARRY);
      mos6502_setflag(CPUSTAT_CARRY, (val & BIT_MASK_0) != 0);
      val >>= 1;
      val |= (oldCarry << 7);
      mos6502_setflag(CPUSTAT_ZERO, val == 0);
      mos6502_setflag(CPUSTAT_NEGATIVE, (val & BIT_MASK_7) != 0);
      if (bytecode->addressingMode == AM_ACCUMULATOR) {
        reg.a = val;
      } else {
        memWrite(operand, val);
      }
      reg.pc += bytecode->count;
      break;
    }
    case I_ILL_DCP: {
      uint8_t m = memRead(operand);
      m -= 1;
      memWrite(operand, m);
      mos6502_setflag(CPUSTAT_ZERO, m == 0);
      mos6502_setflag(CPUSTAT_NEGATIVE, (m & BIT_MASK_7) > 0);
      m = memRead(operand);
      int8_t val = (int8_t)reg.a - (int8_t)m;

      if (reg.a < m) {
        mos6502_setflag(CPUSTAT_ZERO, 0);
        mos6502_setflag(CPUSTAT_CARRY, 0);
      } else if (reg.a == m) {
        mos6502_setflag(CPUSTAT_ZERO, 1);
        mos6502_setflag(CPUSTAT_CARRY, 1);
      } else if (reg.a > m) {
        mos6502_setflag(CPUSTAT_ZERO, 0);
        mos6502_setflag(CPUSTAT_CARRY, 1);
      }

      mos6502_setflag(CPUSTAT_NEGATIVE, val < 0);
      reg.pc += bytecode->count;
      break;
    }
    case I_ILL_ISC: {
      uint8_t m = memRead(operand);
      m += 1;
      memWrite(operand, m);
      mos6502_setflag(CPUSTAT_ZERO, m == 0);
      mos6502_setflag(CPUSTAT_NEGATIVE, (m & BIT_MASK_7) > 0);
      m = ~memRead(operand);
      uint16_t sum = reg.a + m + mos6502_getflag(CPUSTAT_CARRY);
      mos6502_setflag(CPUSTAT_CARRY, sum > 0xFF);
      mos6502_setflag(CPUSTAT_OVERFLOW, (reg.a ^ sum) & (m ^ sum) & 0x80);
      reg.a = sum;
      mos6502_setflag(CPUSTAT_ZERO, reg.a == 0);
      mos6502_setflag(CPUSTAT_NEGATIVE, (reg.a & BIT_MASK_7) != 0);
      reg.pc += bytecode->count;
      break;
    }
    case I_ILL_LAS: {
      uint8_t m = memRead(operand);
      reg.a = m;
      mos6502_setflag(CPUSTAT_ZERO, m == 0);
      mos6502_setflag(CPUSTAT_NEGATIVE, (m & BIT_MASK_7) > 0);
      reg.x = reg.s;
      mos6502_setflag(CPUSTAT_ZERO, reg.s == 0);
      mos6502_setflag(CPUSTAT_NEGATIVE, (reg.s & BIT_MASK_7) != 0);
      reg.pc += bytecode->count;
      break;
    }
    case I_ILL_LAX: {
      uint8_t m = memRead(operand);
      reg.a = m;
      mos6502_setflag(CPUSTAT_ZERO, m == 0);
      mos6502_setflag(CPUSTAT_NEGATIVE, (m & BIT_MASK_7) > 0);
      m = memRead(operand);
      reg.x = m;
      mos6502_setflag(CPUSTAT_ZERO, m == 0);
      mos6502_setflag(CPUSTAT_NEGATIVE, (m & BIT_MASK_7) > 0);
      reg.pc += bytecode->count;
      break;
    }
    case I_ILL_LXA: {

      break;
    }
    case I_ILL_RLA: {
      uint8_t val = (bytecode->addressingMode == AM_ACCUMULATOR) ? reg.a : memRead(operand);
      uint8_t oldCarry = mos6502_getflag(CPUSTAT_CARRY);
      mos6502_setflag(CPUSTAT_CARRY, (val & BIT_MASK_7) != 0);
      val <<= 1;
      val |= oldCarry;
      mos6502_setflag(CPUSTAT_ZERO, val == 0);
      mos6502_setflag(CPUSTAT_NEGATIVE, (val & BIT_MASK_7) != 0);
      if (bytecode->addressingMode == AM_ACCUMULATOR) {
        reg.a = val;
      } else {
        memWrite(operand, val);
      }
      uint16_t m = memRead(operand);
      reg.a = reg.a & m;
      mos6502_setflag(CPUSTAT_ZERO, reg.a == 0);
      mos6502_setflag(CPUSTAT_NEGATIVE, (reg.a & BIT_MASK_7) != 0);
      reg.pc += bytecode->count;
      break;
    }
    case I_ILL_RRA: {
      uint8_t val = (bytecode->addressingMode == AM_ACCUMULATOR) ? reg.a : memRead(operand);
      uint8_t oldCarry = mos6502_getflag(CPUSTAT_CARRY);
      mos6502_setflag(CPUSTAT_CARRY, (val & BIT_MASK_0) != 0);
      val >>= 1;
      val |= (oldCarry << 7);
      mos6502_setflag(CPUSTAT_ZERO, val == 0);
      mos6502_setflag(CPUSTAT_NEGATIVE, (val & BIT_MASK_7) != 0);
      if (bytecode->addressingMode == AM_ACCUMULATOR) {
        reg.a = val;
      } else {
        memWrite(operand, val);
      }
      uint16_t m = memRead(operand);
      uint16_t sum = reg.a + m + mos6502_getflag(CPUSTAT_CARRY);
      mos6502_setflag(CPUSTAT_CARRY, sum > 0xFF);
      mos6502_setflag(CPUSTAT_OVERFLOW, (reg.a ^ sum) & (m ^ sum) & 0x80);
      reg.a = sum;
      mos6502_setflag(CPUSTAT_ZERO, reg.a == 0);
      mos6502_setflag(CPUSTAT_NEGATIVE, (reg.a & BIT_MASK_7) != 0);
      reg.pc += bytecode->count;
      break;
    }
    case I_ILL_SAX: {
      memWrite(operand, reg.a & reg.x);
      reg.pc += bytecode->count;
      break;
    }
    case I_ILL_SBX: {
      uint8_t m = memRead(operand);
      int8_t val = (int8_t)reg.a - (int8_t)m;
      reg.x -= 1;

      if (reg.a < m) {
        mos6502_setflag(CPUSTAT_ZERO, 0);
        mos6502_setflag(CPUSTAT_CARRY, 0);
      } else if (reg.a == m) {
        mos6502_setflag(CPUSTAT_ZERO, 1);
        mos6502_setflag(CPUSTAT_CARRY, 1);
      } else if (reg.a > m) {
        mos6502_setflag(CPUSTAT_ZERO, 0);
        mos6502_setflag(CPUSTAT_CARRY, 1);
      }

      mos6502_setflag(CPUSTAT_NEGATIVE, val < 0);
      reg.pc += bytecode->count;
      break;
    }
    case I_ILL_SHA: {
      return 0;
      break;
    }
    case I_ILL_SHX: {
      return 0;
      break;
    }
    case I_ILL_SHY: {
      return 0;
      break;
    }
    case I_ILL_SLO: {
      uint8_t val = (bytecode->addressingMode == AM_ACCUMULATOR) ? reg.a : memRead(operand);
      mos6502_setflag(CPUSTAT_CARRY, (val & BIT_MASK_7) != 0);
      val <<= 1;
      mos6502_setflag(CPUSTAT_ZERO, val == 0);
      mos6502_setflag(CPUSTAT_NEGATIVE, (val & BIT_MASK_7) != 0);
      if (bytecode->addressingMode == AM_ACCUMULATOR) {
        reg.a = val;
      } else {
        memWrite(operand, val);
      }
      uint16_t m = memRead(operand);
      reg.a = reg.a | m;
      mos6502_setflag(CPUSTAT_ZERO, reg.a == 0);
      mos6502_setflag(CPUSTAT_NEGATIVE, (reg.a & BIT_MASK_7) != 0);
      reg.pc += bytecode->count;
      break;
    }
    case I_ILL_SRE: {
      uint8_t val = (bytecode->addressingMode == AM_ACCUMULATOR) ? reg.a : memRead(operand);
      mos6502_setflag(CPUSTAT_CARRY, (val & BIT_MASK_0) != 0);
      val >>= 1;
      mos6502_setflag(CPUSTAT_ZERO, val == 0);
      mos6502_setflag(CPUSTAT_NEGATIVE, (val & BIT_MASK_7) != 0);
      if (bytecode->addressingMode == AM_ACCUMULATOR) {
        reg.a = val;
      } else {
        memWrite(operand, val);
      }
      uint16_t m = memRead(operand);
      reg.a = reg.a ^ m;
      mos6502_setflag(CPUSTAT_ZERO, reg.a == 0);
      mos6502_setflag(CPUSTAT_NEGATIVE, (reg.a & BIT_MASK_7) != 0);
      reg.pc += bytecode->count;
      break;
    }
    case I_ILL_TAS: {
      return 0;
      break;
    }
    case I_ILL_USBC: {
      uint8_t m = ~memRead(operand);
      uint16_t sum = reg.a + m + mos6502_getflag(CPUSTAT_CARRY);
      mos6502_setflag(CPUSTAT_CARRY, sum > 0xFF);
      mos6502_setflag(CPUSTAT_OVERFLOW, (reg.a ^ sum) & (m ^ sum) & 0x80);
      reg.a = sum;
      mos6502_setflag(CPUSTAT_ZERO, reg.a == 0);
      mos6502_setflag(CPUSTAT_NEGATIVE, (reg.a & BIT_MASK_7) != 0);
      reg.pc += bytecode->count;
      break;
    }
    case I_ILL_NOP: {
      reg.pc += bytecode->count;
      break;
    }
    case I_ILL_JAM: {
      return 0;
      break;
    }
    default: break;
  }
  return cycles;
}

void mos6502_configureTables(void) {
  mnemonicTable[0x00] = I_BRK;
  mnemonicTable[0x01] = I_ORA;
  mnemonicTable[0x02] = I_ILL_JAM;
  mnemonicTable[0x03] = I_ILL_SLO;
  mnemonicTable[0x04] = I_ILL_NOP;
  mnemonicTable[0x05] = I_ORA;
  mnemonicTable[0x06] = I_ASL;
  mnemonicTable[0x07] = I_ILL_SLO;
  mnemonicTable[0x08] = I_PHP;
  mnemonicTable[0x09] = I_ORA;
  mnemonicTable[0x0A] = I_ASL;
  mnemonicTable[0x0B] = I_ILL_ANC;
  mnemonicTable[0x0C] = I_ILL_NOP;
  mnemonicTable[0x0D] = I_ORA;
  mnemonicTable[0x0E] = I_ASL;
  mnemonicTable[0x0F] = I_ILL_SLO;
  mnemonicTable[0x10] = I_BPL;
  mnemonicTable[0x11] = I_ORA;
  mnemonicTable[0x12] = I_ILL_JAM;
  mnemonicTable[0x13] = I_ILL_SLO;
  mnemonicTable[0x14] = I_ILL_NOP;
  mnemonicTable[0x15] = I_ORA;
  mnemonicTable[0x16] = I_ASL;
  mnemonicTable[0x17] = I_ILL_SLO;
  mnemonicTable[0x18] = I_CLC;
  mnemonicTable[0x19] = I_ORA;
  mnemonicTable[0x1A] = I_ILL_NOP;
  mnemonicTable[0x1B] = I_ILL_SLO;
  mnemonicTable[0x1C] = I_ILL_NOP;
  mnemonicTable[0x1D] = I_ORA;
  mnemonicTable[0x1E] = I_ASL;
  mnemonicTable[0x1F] = I_ILL_SLO;
  mnemonicTable[0x20] = I_JSR;
  mnemonicTable[0x21] = I_AND;
  mnemonicTable[0x22] = I_ILL_JAM;
  mnemonicTable[0x23] = I_ILL_RLA;
  mnemonicTable[0x24] = I_BIT;
  mnemonicTable[0x25] = I_AND;
  mnemonicTable[0x26] = I_ROL;
  mnemonicTable[0x27] = I_ILL_RLA;
  mnemonicTable[0x28] = I_PLP;
  mnemonicTable[0x29] = I_AND;
  mnemonicTable[0x2A] = I_ROL;
  mnemonicTable[0x2B] = I_ILL_ANC;
  mnemonicTable[0x2C] = I_BIT;
  mnemonicTable[0x2D] = I_AND;
  mnemonicTable[0x2E] = I_ROL;
  mnemonicTable[0x2F] = I_ILL_RLA;
  mnemonicTable[0x30] = I_BMI;
  mnemonicTable[0x31] = I_AND;
  mnemonicTable[0x32] = I_ILL_JAM;
  mnemonicTable[0x33] = I_ILL_RLA;
  mnemonicTable[0x34] = I_ILL_NOP;
  mnemonicTable[0x35] = I_AND;
  mnemonicTable[0x36] = I_ROL;
  mnemonicTable[0x37] = I_ILL_RLA;
  mnemonicTable[0x38] = I_SEC;
  mnemonicTable[0x39] = I_AND;
  mnemonicTable[0x3A] = I_ILL_NOP;
  mnemonicTable[0x3B] = I_ILL_RLA;
  mnemonicTable[0x3C] = I_ILL_NOP;
  mnemonicTable[0x3D] = I_AND;
  mnemonicTable[0x3E] = I_ROL;
  mnemonicTable[0x3F] = I_ILL_RLA;
  mnemonicTable[0x40] = I_RTI;
  mnemonicTable[0x41] = I_EOR;
  mnemonicTable[0x42] = I_ILL_JAM;
  mnemonicTable[0x43] = I_ILL_SRE;
  mnemonicTable[0x44] = I_ILL_NOP;
  mnemonicTable[0x45] = I_EOR;
  mnemonicTable[0x46] = I_LSR;
  mnemonicTable[0x47] = I_ILL_SRE;
  mnemonicTable[0x48] = I_PHA;
  mnemonicTable[0x49] = I_EOR;
  mnemonicTable[0x4A] = I_LSR;
  mnemonicTable[0x4B] = I_ILL_ALR;
  mnemonicTable[0x4C] = I_JMP;
  mnemonicTable[0x4D] = I_EOR;
  mnemonicTable[0x4E] = I_LSR;
  mnemonicTable[0x4F] = I_ILL_SRE;
  mnemonicTable[0x50] = I_BVC;
  mnemonicTable[0x51] = I_EOR;
  mnemonicTable[0x52] = I_ILL_JAM;
  mnemonicTable[0x53] = I_ILL_SRE;
  mnemonicTable[0x54] = I_ILL_NOP;
  mnemonicTable[0x55] = I_EOR;
  mnemonicTable[0x56] = I_LSR;
  mnemonicTable[0x57] = I_ILL_SRE;
  mnemonicTable[0x58] = I_CLI;
  mnemonicTable[0x59] = I_EOR;
  mnemonicTable[0x5A] = I_ILL_NOP;
  mnemonicTable[0x5B] = I_ILL_SRE;
  mnemonicTable[0x5C] = I_ILL_NOP;
  mnemonicTable[0x5D] = I_EOR;
  mnemonicTable[0x5E] = I_LSR;
  mnemonicTable[0x5F] = I_ILL_SRE;
  mnemonicTable[0x60] = I_RTS;
  mnemonicTable[0x61] = I_ADC;
  mnemonicTable[0x62] = I_ILL_JAM;
  mnemonicTable[0x63] = I_ILL_RRA;
  mnemonicTable[0x64] = I_ILL_NOP;
  mnemonicTable[0x65] = I_ADC;
  mnemonicTable[0x66] = I_ROR;
  mnemonicTable[0x67] = I_ILL_RRA;
  mnemonicTable[0x68] = I_PLA;
  mnemonicTable[0x69] = I_ADC;
  mnemonicTable[0x6A] = I_ROR;
  mnemonicTable[0x6B] = I_ILL_ARR;
  mnemonicTable[0x6C] = I_JMP;
  mnemonicTable[0x6D] = I_ADC;
  mnemonicTable[0x6E] = I_ROR;
  mnemonicTable[0x6F] = I_ILL_RRA;
  mnemonicTable[0x70] = I_BVS;
  mnemonicTable[0x71] = I_ADC;
  mnemonicTable[0x72] = I_ILL_JAM;
  mnemonicTable[0x73] = I_ILL_RRA;
  mnemonicTable[0x74] = I_ILL_NOP;
  mnemonicTable[0x75] = I_ADC;
  mnemonicTable[0x76] = I_ROR;
  mnemonicTable[0x77] = I_ILL_RRA;
  mnemonicTable[0x78] = I_SEI;
  mnemonicTable[0x79] = I_ADC;
  mnemonicTable[0x7A] = I_ILL_NOP;
  mnemonicTable[0x7B] = I_ILL_RRA;
  mnemonicTable[0x7C] = I_ILL_NOP;
  mnemonicTable[0x7D] = I_ADC;
  mnemonicTable[0x7E] = I_ROR;
  mnemonicTable[0x7F] = I_ILL_RRA;
  mnemonicTable[0x80] = I_ILL_NOP;
  mnemonicTable[0x81] = I_STA;
  mnemonicTable[0x82] = I_ILL_NOP;
  mnemonicTable[0x83] = I_ILL_SAX;
  mnemonicTable[0x84] = I_STY;
  mnemonicTable[0x85] = I_STA;
  mnemonicTable[0x86] = I_STX;
  mnemonicTable[0x87] = I_ILL_SAX;
  mnemonicTable[0x88] = I_DEY;
  mnemonicTable[0x89] = I_ILL_NOP;
  mnemonicTable[0x8A] = I_TXA;
  mnemonicTable[0x8B] = I_ILL_ANE;
  mnemonicTable[0x8C] = I_STY;
  mnemonicTable[0x8D] = I_STA;
  mnemonicTable[0x8E] = I_STX;
  mnemonicTable[0x8F] = I_ILL_SAX;
  mnemonicTable[0x90] = I_BCC;
  mnemonicTable[0x91] = I_STA;
  mnemonicTable[0x92] = I_ILL_JAM;
  mnemonicTable[0x93] = I_ILL_SHA;
  mnemonicTable[0x94] = I_STY;
  mnemonicTable[0x95] = I_STA;
  mnemonicTable[0x96] = I_STX;
  mnemonicTable[0x97] = I_ILL_SAX;
  mnemonicTable[0x98] = I_TYA;
  mnemonicTable[0x99] = I_STA;
  mnemonicTable[0x9A] = I_TXS;
  mnemonicTable[0x9B] = I_ILL_TAS;
  mnemonicTable[0x9C] = I_ILL_SHY;
  mnemonicTable[0x9D] = I_STA;
  mnemonicTable[0x9E] = I_ILL_SHX;
  mnemonicTable[0x9F] = I_ILL_SHA;
  mnemonicTable[0xA0] = I_LDY;
  mnemonicTable[0xA1] = I_LDA;
  mnemonicTable[0xA2] = I_LDX;
  mnemonicTable[0xA3] = I_ILL_LAX;
  mnemonicTable[0xA4] = I_LDY;
  mnemonicTable[0xA5] = I_LDA;
  mnemonicTable[0xA6] = I_LDX;
  mnemonicTable[0xA7] = I_ILL_LAX;
  mnemonicTable[0xA8] = I_TAY;
  mnemonicTable[0xA9] = I_LDA;
  mnemonicTable[0xAA] = I_TAX;
  mnemonicTable[0xAB] = I_ILL_LXA;
  mnemonicTable[0xAC] = I_LDY;
  mnemonicTable[0xAD] = I_LDA;
  mnemonicTable[0xAE] = I_LDX;
  mnemonicTable[0xAF] = I_ILL_LAX;
  mnemonicTable[0xB0] = I_BCS;
  mnemonicTable[0xB1] = I_LDA;
  mnemonicTable[0xB2] = I_ILL_JAM;
  mnemonicTable[0xB3] = I_ILL_LAX;
  mnemonicTable[0xB4] = I_LDY;
  mnemonicTable[0xB5] = I_LDA;
  mnemonicTable[0xB6] = I_LDX;
  mnemonicTable[0xB7] = I_ILL_LAX;
  mnemonicTable[0xB8] = I_CLV;
  mnemonicTable[0xB9] = I_LDA;
  mnemonicTable[0xBA] = I_TSX;
  mnemonicTable[0xBB] = I_ILL_LAS;
  mnemonicTable[0xBC] = I_LDY;
  mnemonicTable[0xBD] = I_LDA;
  mnemonicTable[0xBE] = I_LDX;
  mnemonicTable[0xBF] = I_ILL_LAX;
  mnemonicTable[0xC0] = I_CPY;
  mnemonicTable[0xC1] = I_CMP;
  mnemonicTable[0xC2] = I_ILL_NOP;
  mnemonicTable[0xC3] = I_ILL_DCP;
  mnemonicTable[0xC4] = I_CPY;
  mnemonicTable[0xC5] = I_CMP;
  mnemonicTable[0xC6] = I_DEC;
  mnemonicTable[0xC7] = I_ILL_DCP;
  mnemonicTable[0xC8] = I_INY;
  mnemonicTable[0xC9] = I_CMP;
  mnemonicTable[0xCA] = I_DEX;
  mnemonicTable[0xCB] = I_ILL_SBX;
  mnemonicTable[0xCC] = I_CPY;
  mnemonicTable[0xCD] = I_CMP;
  mnemonicTable[0xCE] = I_DEC;
  mnemonicTable[0xCF] = I_ILL_DCP;
  mnemonicTable[0xD0] = I_BNE;
  mnemonicTable[0xD1] = I_CMP;
  mnemonicTable[0xD2] = I_ILL_JAM;
  mnemonicTable[0xD3] = I_ILL_DCP;
  mnemonicTable[0xD4] = I_ILL_NOP;
  mnemonicTable[0xD5] = I_CMP;
  mnemonicTable[0xD6] = I_DEC;
  mnemonicTable[0xD7] = I_ILL_DCP;
  mnemonicTable[0xD8] = I_CLD;
  mnemonicTable[0xD9] = I_CMP;
  mnemonicTable[0xDA] = I_ILL_NOP;
  mnemonicTable[0xDB] = I_ILL_DCP;
  mnemonicTable[0xDC] = I_ILL_NOP;
  mnemonicTable[0xDD] = I_CMP;
  mnemonicTable[0xDE] = I_DEC;
  mnemonicTable[0xDF] = I_ILL_DCP;
  mnemonicTable[0xE0] = I_CPX;
  mnemonicTable[0xE1] = I_SBC;
  mnemonicTable[0xE2] = I_ILL_NOP;
  mnemonicTable[0xE3] = I_ILL_ISC;
  mnemonicTable[0xE4] = I_CPX;
  mnemonicTable[0xE5] = I_SBC;
  mnemonicTable[0xE6] = I_INC;
  mnemonicTable[0xE7] = I_ILL_ISC;
  mnemonicTable[0xE8] = I_INX;
  mnemonicTable[0xE9] = I_SBC;
  mnemonicTable[0xEA] = I_NOP;
  mnemonicTable[0xEB] = I_ILL_USBC;
  mnemonicTable[0xEC] = I_CPX;
  mnemonicTable[0xED] = I_SBC;
  mnemonicTable[0xEE] = I_INC;
  mnemonicTable[0xEF] = I_ILL_ISC;
  mnemonicTable[0xF0] = I_BEQ;
  mnemonicTable[0xF1] = I_SBC;
  mnemonicTable[0xF2] = I_ILL_JAM;
  mnemonicTable[0xF3] = I_ILL_ISC;
  mnemonicTable[0xF4] = I_ILL_NOP;
  mnemonicTable[0xF5] = I_SBC;
  mnemonicTable[0xF6] = I_INC;
  mnemonicTable[0xF7] = I_ILL_ISC;
  mnemonicTable[0xF8] = I_SED;
  mnemonicTable[0xF9] = I_SBC;
  mnemonicTable[0xFA] = I_ILL_NOP;
  mnemonicTable[0xFB] = I_ILL_ISC;
  mnemonicTable[0xFC] = I_ILL_NOP;
  mnemonicTable[0xFD] = I_SBC;
  mnemonicTable[0xFE] = I_INC;
  mnemonicTable[0xFF] = I_ILL_ISC;

  addrModeTable[0x00] = AM_IMPLIED;
  addrModeTable[0x01] = AM_ZP_X_INDIRECT;
  addrModeTable[0x02] = AM_IMPLIED;
  addrModeTable[0x03] = AM_ZP_X_INDIRECT;
  addrModeTable[0x04] = AM_ZERO_PAGE;
  addrModeTable[0x05] = AM_ZERO_PAGE;
  addrModeTable[0x06] = AM_ZERO_PAGE;
  addrModeTable[0x07] = AM_ZERO_PAGE;
  addrModeTable[0x08] = AM_IMPLIED;
  addrModeTable[0x09] = AM_IMMEDIATE;
  addrModeTable[0x0A] = AM_ACCUMULATOR;
  addrModeTable[0x0B] = AM_IMMEDIATE;
  addrModeTable[0x0C] = AM_ABSOLUTE;
  addrModeTable[0x0D] = AM_ABSOLUTE;
  addrModeTable[0x0E] = AM_ABSOLUTE;
  addrModeTable[0x0F] = AM_ABSOLUTE;
  addrModeTable[0x10] = AM_RELATIVE;
  addrModeTable[0x11] = AM_ZP_INDIRECT_Y;
  addrModeTable[0x12] = AM_IMPLIED;
  addrModeTable[0x13] = AM_ZP_INDIRECT_Y;
  addrModeTable[0x14] = AM_ZP_X;
  addrModeTable[0x15] = AM_ZP_X;
  addrModeTable[0x16] = AM_ZP_X;
  addrModeTable[0x17] = AM_ZP_X;
  addrModeTable[0x18] = AM_IMPLIED;
  addrModeTable[0x19] = AM_ABS_Y;
  addrModeTable[0x1A] = AM_IMPLIED;
  addrModeTable[0x1B] = AM_ABS_Y;
  addrModeTable[0x1C] = AM_ABS_X;
  addrModeTable[0x1D] = AM_ABS_X;
  addrModeTable[0x1E] = AM_ABS_X;
  addrModeTable[0x1F] = AM_ABS_X;
  addrModeTable[0x20] = AM_ABSOLUTE;
  addrModeTable[0x21] = AM_ZP_X_INDIRECT;
  addrModeTable[0x22] = AM_IMPLIED;
  addrModeTable[0x23] = AM_ZP_X_INDIRECT;
  addrModeTable[0x24] = AM_ZERO_PAGE;
  addrModeTable[0x25] = AM_ZERO_PAGE;
  addrModeTable[0x26] = AM_ZERO_PAGE;
  addrModeTable[0x27] = AM_ZERO_PAGE;
  addrModeTable[0x28] = AM_IMPLIED;
  addrModeTable[0x29] = AM_IMMEDIATE;
  addrModeTable[0x2A] = AM_ACCUMULATOR;
  addrModeTable[0x2B] = AM_IMMEDIATE;
  addrModeTable[0x2C] = AM_ABSOLUTE;
  addrModeTable[0x2D] = AM_ABSOLUTE;
  addrModeTable[0x2E] = AM_ABSOLUTE;
  addrModeTable[0x2F] = AM_ABSOLUTE;
  addrModeTable[0x30] = AM_RELATIVE;
  addrModeTable[0x31] = AM_ZP_INDIRECT_Y;
  addrModeTable[0x32] = AM_IMPLIED;
  addrModeTable[0x33] = AM_ZP_INDIRECT_Y;
  addrModeTable[0x34] = AM_ZP_X;
  addrModeTable[0x35] = AM_ZP_X;
  addrModeTable[0x36] = AM_ZP_X;
  addrModeTable[0x37] = AM_ZP_X;
  addrModeTable[0x38] = AM_IMPLIED;
  addrModeTable[0x39] = AM_ABS_Y;
  addrModeTable[0x3A] = AM_IMPLIED;
  addrModeTable[0x3B] = AM_ABS_Y;
  addrModeTable[0x3C] = AM_ABS_X;
  addrModeTable[0x3D] = AM_ABS_X;
  addrModeTable[0x3E] = AM_ABS_X;
  addrModeTable[0x3F] = AM_ABS_X;
  addrModeTable[0x40] = AM_IMPLIED;
  addrModeTable[0x41] = AM_ZP_X_INDIRECT;
  addrModeTable[0x42] = AM_IMPLIED;
  addrModeTable[0x43] = AM_ZP_X_INDIRECT;
  addrModeTable[0x44] = AM_ZERO_PAGE;
  addrModeTable[0x45] = AM_ZERO_PAGE;
  addrModeTable[0x46] = AM_ZERO_PAGE;
  addrModeTable[0x47] = AM_ZERO_PAGE;
  addrModeTable[0x48] = AM_IMPLIED;
  addrModeTable[0x49] = AM_IMMEDIATE;
  addrModeTable[0x4A] = AM_ACCUMULATOR;
  addrModeTable[0x4B] = AM_ACCUMULATOR;
  addrModeTable[0x4C] = AM_ABSOLUTE;
  addrModeTable[0x4D] = AM_ABSOLUTE;
  addrModeTable[0x4E] = AM_ABSOLUTE;
  addrModeTable[0x4F] = AM_ABSOLUTE;
  addrModeTable[0x50] = AM_RELATIVE;
  addrModeTable[0x51] = AM_ZP_INDIRECT_Y;
  addrModeTable[0x52] = AM_IMPLIED;
  addrModeTable[0x53] = AM_ZP_INDIRECT_Y;
  addrModeTable[0x54] = AM_ZP_X;
  addrModeTable[0x55] = AM_ZP_X;
  addrModeTable[0x56] = AM_ZP_X;
  addrModeTable[0x57] = AM_ZP_X;
  addrModeTable[0x58] = AM_IMPLIED;
  addrModeTable[0x59] = AM_ABS_Y;
  addrModeTable[0x5A] = AM_IMPLIED;
  addrModeTable[0x5B] = AM_ABS_Y;
  addrModeTable[0x5C] = AM_ABS_X;
  addrModeTable[0x5D] = AM_ABS_X;
  addrModeTable[0x5E] = AM_ABS_X;
  addrModeTable[0x5F] = AM_ABS_X;
  addrModeTable[0x60] = AM_IMPLIED;
  addrModeTable[0x61] = AM_ZP_X_INDIRECT;
  addrModeTable[0x62] = AM_IMPLIED;
  addrModeTable[0x63] = AM_ZP_X_INDIRECT;
  addrModeTable[0x64] = AM_ZERO_PAGE;
  addrModeTable[0x65] = AM_ZERO_PAGE;
  addrModeTable[0x66] = AM_ZERO_PAGE;
  addrModeTable[0x67] = AM_ZERO_PAGE;
  addrModeTable[0x68] = AM_IMPLIED;
  addrModeTable[0x69] = AM_IMMEDIATE;
  addrModeTable[0x6A] = AM_ACCUMULATOR;
  addrModeTable[0x6B] = AM_IMMEDIATE;
  addrModeTable[0x6C] = AM_ABS_INDIRECT;
  addrModeTable[0x6D] = AM_ABSOLUTE;
  addrModeTable[0x6E] = AM_ABSOLUTE;
  addrModeTable[0x6F] = AM_ABSOLUTE;
  addrModeTable[0x70] = AM_RELATIVE;
  addrModeTable[0x71] = AM_ZP_INDIRECT_Y;
  addrModeTable[0x72] = AM_IMPLIED;
  addrModeTable[0x73] = AM_ZP_INDIRECT_Y;
  addrModeTable[0x74] = AM_ZP_X;
  addrModeTable[0x75] = AM_ZP_X;
  addrModeTable[0x76] = AM_ZP_X;
  addrModeTable[0x77] = AM_ZP_X;
  addrModeTable[0x78] = AM_IMPLIED;
  addrModeTable[0x79] = AM_ABS_Y;
  addrModeTable[0x7A] = AM_IMPLIED;
  addrModeTable[0x7B] = AM_ABS_Y;
  addrModeTable[0x7C] = AM_ABS_X;
  addrModeTable[0x7D] = AM_ABS_X;
  addrModeTable[0x7E] = AM_ABS_X;
  addrModeTable[0x7F] = AM_ABS_X;
  addrModeTable[0x80] = AM_IMMEDIATE;
  addrModeTable[0x81] = AM_ZP_X_INDIRECT;
  addrModeTable[0x82] = AM_IMPLIED;
  addrModeTable[0x83] = AM_ZP_X_INDIRECT;
  addrModeTable[0x84] = AM_ZERO_PAGE;
  addrModeTable[0x85] = AM_ZERO_PAGE;
  addrModeTable[0x86] = AM_ZERO_PAGE;
  addrModeTable[0x87] = AM_ZERO_PAGE;
  addrModeTable[0x88] = AM_IMPLIED;
  addrModeTable[0x89] = AM_IMMEDIATE;
  addrModeTable[0x8A] = AM_IMPLIED;
  addrModeTable[0x8B] = AM_IMMEDIATE;
  addrModeTable[0x8C] = AM_ABSOLUTE;
  addrModeTable[0x8D] = AM_ABSOLUTE;
  addrModeTable[0x8E] = AM_ABSOLUTE;
  addrModeTable[0x8F] = AM_ABSOLUTE;
  addrModeTable[0x90] = AM_RELATIVE;
  addrModeTable[0x91] = AM_ZP_INDIRECT_Y;
  addrModeTable[0x92] = AM_IMPLIED;
  addrModeTable[0x93] = AM_ZP_INDIRECT_Y;
  addrModeTable[0x94] = AM_ZP_X;
  addrModeTable[0x95] = AM_ZP_X;
  addrModeTable[0x96] = AM_ZP_Y;
  addrModeTable[0x97] = AM_ZP_Y;
  addrModeTable[0x98] = AM_IMPLIED;
  addrModeTable[0x99] = AM_ABS_Y;
  addrModeTable[0x9A] = AM_IMPLIED;
  addrModeTable[0x9B] = AM_ABS_Y;
  addrModeTable[0x9C] = AM_ABS_X;
  addrModeTable[0x9D] = AM_ABS_X;
  addrModeTable[0x9E] = AM_ABS_Y;
  addrModeTable[0x9F] = AM_ABS_Y;
  addrModeTable[0xA0] = AM_IMMEDIATE;
  addrModeTable[0xA1] = AM_ZP_X_INDIRECT;
  addrModeTable[0xA2] = AM_IMMEDIATE;
  addrModeTable[0xA3] = AM_ZP_X_INDIRECT;
  addrModeTable[0xA4] = AM_ZERO_PAGE;
  addrModeTable[0xA5] = AM_ZERO_PAGE;
  addrModeTable[0xA6] = AM_ZERO_PAGE;
  addrModeTable[0xA7] = AM_ZERO_PAGE;
  addrModeTable[0xA8] = AM_IMPLIED;
  addrModeTable[0xA9] = AM_IMMEDIATE;
  addrModeTable[0xAA] = AM_IMPLIED;
  addrModeTable[0xAB] = AM_IMMEDIATE;
  addrModeTable[0xAC] = AM_ABSOLUTE;
  addrModeTable[0xAD] = AM_ABSOLUTE;
  addrModeTable[0xAE] = AM_ABSOLUTE;
  addrModeTable[0xAF] = AM_ABSOLUTE;
  addrModeTable[0xB0] = AM_RELATIVE;
  addrModeTable[0xB1] = AM_ZP_INDIRECT_Y;
  addrModeTable[0xB2] = AM_IMPLIED;
  addrModeTable[0xB3] = AM_ZP_INDIRECT_Y;
  addrModeTable[0xB4] = AM_ZP_X;
  addrModeTable[0xB5] = AM_ZP_X;
  addrModeTable[0xB6] = AM_ZP_Y;
  addrModeTable[0xB7] = AM_ZP_Y;
  addrModeTable[0xB8] = AM_IMPLIED;
  addrModeTable[0xB9] = AM_ABS_Y;
  addrModeTable[0xBA] = AM_IMPLIED;
  addrModeTable[0xBB] = AM_ABS_Y;
  addrModeTable[0xBC] = AM_ABS_X;
  addrModeTable[0xBD] = AM_ABS_X;
  addrModeTable[0xBE] = AM_ABS_Y;
  addrModeTable[0xBF] = AM_ABS_Y;
  addrModeTable[0xC0] = AM_IMMEDIATE;
  addrModeTable[0xC1] = AM_ZP_X_INDIRECT;
  addrModeTable[0xC2] = AM_IMPLIED;
  addrModeTable[0xC3] = AM_ZP_X_INDIRECT;
  addrModeTable[0xC4] = AM_ZERO_PAGE;
  addrModeTable[0xC5] = AM_ZERO_PAGE;
  addrModeTable[0xC6] = AM_ZERO_PAGE;
  addrModeTable[0xC7] = AM_ZERO_PAGE;
  addrModeTable[0xC8] = AM_IMPLIED;
  addrModeTable[0xC9] = AM_IMMEDIATE;
  addrModeTable[0xCA] = AM_IMPLIED;
  addrModeTable[0xCB] = AM_IMMEDIATE;
  addrModeTable[0xCC] = AM_ABSOLUTE;
  addrModeTable[0xCD] = AM_ABSOLUTE;
  addrModeTable[0xCE] = AM_ABSOLUTE;
  addrModeTable[0xCF] = AM_ABSOLUTE;
  addrModeTable[0xD0] = AM_RELATIVE;
  addrModeTable[0xD1] = AM_ZP_INDIRECT_Y;
  addrModeTable[0xD2] = AM_IMPLIED;
  addrModeTable[0xD3] = AM_ZP_INDIRECT_Y;
  addrModeTable[0xD4] = AM_ZP_X;
  addrModeTable[0xD5] = AM_ZP_X;
  addrModeTable[0xD6] = AM_ZP_X;
  addrModeTable[0xD7] = AM_ZP_X;
  addrModeTable[0xD8] = AM_IMPLIED;
  addrModeTable[0xD9] = AM_ABS_Y;
  addrModeTable[0xDA] = AM_IMPLIED;
  addrModeTable[0xDB] = AM_ABS_Y;
  addrModeTable[0xDC] = AM_ABS_X;
  addrModeTable[0xDD] = AM_ABS_X;
  addrModeTable[0xDE] = AM_ABS_X;
  addrModeTable[0xDF] = AM_ABS_X;
  addrModeTable[0xE0] = AM_IMMEDIATE;
  addrModeTable[0xE1] = AM_ZP_X_INDIRECT;
  addrModeTable[0xE2] = AM_IMMEDIATE;
  addrModeTable[0xE3] = AM_ZP_X_INDIRECT;
  addrModeTable[0xE4] = AM_ZERO_PAGE;
  addrModeTable[0xE5] = AM_ZERO_PAGE;
  addrModeTable[0xE6] = AM_ZERO_PAGE;
  addrModeTable[0xE7] = AM_ZERO_PAGE;
  addrModeTable[0xE8] = AM_IMPLIED;
  addrModeTable[0xE9] = AM_IMMEDIATE;
  addrModeTable[0xEA] = AM_IMPLIED;
  addrModeTable[0xEB] = AM_IMMEDIATE;
  addrModeTable[0xEC] = AM_ABSOLUTE;
  addrModeTable[0xED] = AM_ABSOLUTE;
  addrModeTable[0xEE] = AM_ABSOLUTE;
  addrModeTable[0xEF] = AM_ABSOLUTE;
  addrModeTable[0xF0] = AM_RELATIVE;
  addrModeTable[0xF1] = AM_ZP_INDIRECT_Y;
  addrModeTable[0xF2] = AM_IMPLIED;
  addrModeTable[0xF3] = AM_ZP_INDIRECT_Y;
  addrModeTable[0xF4] = AM_ZP_X;
  addrModeTable[0xF5] = AM_ZP_X;
  addrModeTable[0xF6] = AM_ZP_X;
  addrModeTable[0xF7] = AM_ZP_X;
  addrModeTable[0xF8] = AM_IMPLIED;
  addrModeTable[0xF9] = AM_ABS_Y;
  addrModeTable[0xFA] = AM_IMPLIED;
  addrModeTable[0xFB] = AM_ABS_Y;
  addrModeTable[0xFC] = AM_ABS_X;
  addrModeTable[0xFD] = AM_ABS_X;
  addrModeTable[0xFE] = AM_ABS_X;
  addrModeTable[0xFF] = AM_ABS_X;

  cycleTable[0x00] = 7;
  cycleTable[0x01] = 6;
  cycleTable[0x02] = 0;
  cycleTable[0x03] = 8;
  cycleTable[0x04] = 3;
  cycleTable[0x05] = 3;
  cycleTable[0x06] = 5;
  cycleTable[0x07] = 5;
  cycleTable[0x08] = 3;
  cycleTable[0x09] = 2;
  cycleTable[0x0A] = 2;
  cycleTable[0x0B] = 2;
  cycleTable[0x0C] = 4;
  cycleTable[0x0D] = 4;
  cycleTable[0x0E] = 6;
  cycleTable[0x0F] = 6;
  cycleTable[0x10] = 2;
  cycleTable[0x11] = 5;
  cycleTable[0x12] = 0;
  cycleTable[0x13] = 8;
  cycleTable[0x14] = 4;
  cycleTable[0x15] = 4;
  cycleTable[0x16] = 6;
  cycleTable[0x17] = 6;
  cycleTable[0x18] = 2;
  cycleTable[0x19] = 4;
  cycleTable[0x1A] = 2;
  cycleTable[0x1B] = 7;
  cycleTable[0x1C] = 4;
  cycleTable[0x1D] = 4;
  cycleTable[0x1E] = 7;
  cycleTable[0x1F] = 7;
  cycleTable[0x20] = 6;
  cycleTable[0x21] = 6;
  cycleTable[0x22] = 0;
  cycleTable[0x23] = 8;
  cycleTable[0x24] = 3;
  cycleTable[0x25] = 3;
  cycleTable[0x26] = 5;
  cycleTable[0x27] = 5;
  cycleTable[0x28] = 4;
  cycleTable[0x29] = 2;
  cycleTable[0x2A] = 2;
  cycleTable[0x2B] = 2;
  cycleTable[0x2C] = 4;
  cycleTable[0x2D] = 4;
  cycleTable[0x2E] = 6;
  cycleTable[0x2F] = 6;
  cycleTable[0x30] = 2;
  cycleTable[0x31] = 5;
  cycleTable[0x32] = 0;
  cycleTable[0x33] = 8;
  cycleTable[0x34] = 4;
  cycleTable[0x35] = 4;
  cycleTable[0x36] = 6;
  cycleTable[0x37] = 6;
  cycleTable[0x38] = 2;
  cycleTable[0x39] = 4;
  cycleTable[0x3A] = 2;
  cycleTable[0x3B] = 7;
  cycleTable[0x3C] = 4;
  cycleTable[0x3D] = 4;
  cycleTable[0x3E] = 7;
  cycleTable[0x3F] = 7;
  cycleTable[0x40] = 6;
  cycleTable[0x41] = 6;
  cycleTable[0x42] = 0;
  cycleTable[0x43] = 8;
  cycleTable[0x44] = 3;
  cycleTable[0x45] = 3;
  cycleTable[0x46] = 5;
  cycleTable[0x47] = 5;
  cycleTable[0x48] = 3;
  cycleTable[0x49] = 2;
  cycleTable[0x4A] = 2;
  cycleTable[0x4B] = 2;
  cycleTable[0x4C] = 3;
  cycleTable[0x4D] = 4;
  cycleTable[0x4E] = 6;
  cycleTable[0x4F] = 6;
  cycleTable[0x50] = 2;
  cycleTable[0x51] = 5;
  cycleTable[0x52] = 0;
  cycleTable[0x53] = 8;
  cycleTable[0x54] = 4;
  cycleTable[0x55] = 4;
  cycleTable[0x56] = 6;
  cycleTable[0x57] = 6;
  cycleTable[0x58] = 2;
  cycleTable[0x59] = 4;
  cycleTable[0x5A] = 2;
  cycleTable[0x5B] = 7;
  cycleTable[0x5C] = 4;
  cycleTable[0x5D] = 4;
  cycleTable[0x5E] = 7;
  cycleTable[0x5F] = 7;
  cycleTable[0x60] = 6;
  cycleTable[0x61] = 6;
  cycleTable[0x62] = 0;
  cycleTable[0x63] = 8;
  cycleTable[0x64] = 3;
  cycleTable[0x65] = 3;
  cycleTable[0x66] = 5;
  cycleTable[0x67] = 5;
  cycleTable[0x68] = 4;
  cycleTable[0x69] = 2;
  cycleTable[0x6A] = 2;
  cycleTable[0x6B] = 2;
  cycleTable[0x6C] = 5;
  cycleTable[0x6D] = 4;
  cycleTable[0x6E] = 6;
  cycleTable[0x6F] = 6;
  cycleTable[0x70] = 2;
  cycleTable[0x71] = 5;
  cycleTable[0x72] = 0;
  cycleTable[0x73] = 8;
  cycleTable[0x74] = 4;
  cycleTable[0x75] = 4;
  cycleTable[0x76] = 6;
  cycleTable[0x77] = 6;
  cycleTable[0x78] = 2;
  cycleTable[0x79] = 4;
  cycleTable[0x7A] = 2;
  cycleTable[0x7B] = 7;
  cycleTable[0x7C] = 4;
  cycleTable[0x7D] = 4;
  cycleTable[0x7E] = 7;
  cycleTable[0x7F] = 7;
  cycleTable[0x80] = 2;
  cycleTable[0x81] = 6;
  cycleTable[0x82] = 2;
  cycleTable[0x83] = 6;
  cycleTable[0x84] = 3;
  cycleTable[0x85] = 3;
  cycleTable[0x86] = 3;
  cycleTable[0x87] = 3;
  cycleTable[0x88] = 2;
  cycleTable[0x89] = 2;
  cycleTable[0x8A] = 2;
  cycleTable[0x8B] = 2;
  cycleTable[0x8C] = 4;
  cycleTable[0x8D] = 4;
  cycleTable[0x8E] = 4;
  cycleTable[0x8F] = 4;
  cycleTable[0x90] = 2;
  cycleTable[0x91] = 6;
  cycleTable[0x92] = 0;
  cycleTable[0x93] = 6;
  cycleTable[0x94] = 4;
  cycleTable[0x95] = 4;
  cycleTable[0x96] = 4;
  cycleTable[0x97] = 4;
  cycleTable[0x98] = 2;
  cycleTable[0x99] = 5;
  cycleTable[0x9A] = 2;
  cycleTable[0x9B] = 5;
  cycleTable[0x9C] = 5;
  cycleTable[0x9D] = 5;
  cycleTable[0x9E] = 5;
  cycleTable[0x9F] = 5;
  cycleTable[0xA0] = 2;
  cycleTable[0xA1] = 6;
  cycleTable[0xA2] = 2;
  cycleTable[0xA3] = 6;
  cycleTable[0xA4] = 3;
  cycleTable[0xA5] = 3;
  cycleTable[0xA6] = 3;
  cycleTable[0xA7] = 3;
  cycleTable[0xA8] = 2;
  cycleTable[0xA9] = 2;
  cycleTable[0xAA] = 2;
  cycleTable[0xAB] = 2;
  cycleTable[0xAC] = 4;
  cycleTable[0xAD] = 4;
  cycleTable[0xAE] = 4;
  cycleTable[0xAF] = 4;
  cycleTable[0xB0] = 2;
  cycleTable[0xB1] = 5;
  cycleTable[0xB2] = 0;
  cycleTable[0xB3] = 5;
  cycleTable[0xB4] = 4;
  cycleTable[0xB5] = 4;
  cycleTable[0xB6] = 4;
  cycleTable[0xB7] = 4;
  cycleTable[0xB8] = 2;
  cycleTable[0xB9] = 4;
  cycleTable[0xBA] = 2;
  cycleTable[0xBB] = 4;
  cycleTable[0xBC] = 4;
  cycleTable[0xBD] = 4;
  cycleTable[0xBE] = 4;
  cycleTable[0xBF] = 4;
  cycleTable[0xC0] = 2;
  cycleTable[0xC1] = 6;
  cycleTable[0xC2] = 2;
  cycleTable[0xC3] = 8;
  cycleTable[0xC4] = 3;
  cycleTable[0xC5] = 3;
  cycleTable[0xC6] = 5;
  cycleTable[0xC7] = 5;
  cycleTable[0xC8] = 2;
  cycleTable[0xC9] = 2;
  cycleTable[0xCA] = 2;
  cycleTable[0xCB] = 2;
  cycleTable[0xCC] = 4;
  cycleTable[0xCD] = 4;
  cycleTable[0xCE] = 6;
  cycleTable[0xCF] = 6;
  cycleTable[0xD0] = 2;
  cycleTable[0xD1] = 5;
  cycleTable[0xD2] = 0;
  cycleTable[0xD3] = 8;
  cycleTable[0xD4] = 4;
  cycleTable[0xD5] = 4;
  cycleTable[0xD6] = 6;
  cycleTable[0xD7] = 6;
  cycleTable[0xD8] = 2;
  cycleTable[0xD9] = 4;
  cycleTable[0xDA] = 2;
  cycleTable[0xDB] = 7;
  cycleTable[0xDC] = 4;
  cycleTable[0xDD] = 4;
  cycleTable[0xDE] = 7;
  cycleTable[0xDF] = 7;
  cycleTable[0xE0] = 2;
  cycleTable[0xE1] = 6;
  cycleTable[0xE2] = 2;
  cycleTable[0xE3] = 8;
  cycleTable[0xE4] = 3;
  cycleTable[0xE5] = 3;
  cycleTable[0xE6] = 5;
  cycleTable[0xE7] = 5;
  cycleTable[0xE8] = 2;
  cycleTable[0xE9] = 2;
  cycleTable[0xEA] = 2;
  cycleTable[0xEB] = 2;
  cycleTable[0xEC] = 4;
  cycleTable[0xED] = 4;
  cycleTable[0xEE] = 6;
  cycleTable[0xEF] = 6;
  cycleTable[0xF0] = 2;
  cycleTable[0xF1] = 5;
  cycleTable[0xF2] = 0;
  cycleTable[0xF3] = 8;
  cycleTable[0xF4] = 4;
  cycleTable[0xF5] = 4;
  cycleTable[0xF6] = 6;
  cycleTable[0xF7] = 6;
  cycleTable[0xF8] = 2;
  cycleTable[0xF9] = 4;
  cycleTable[0xFA] = 2;
  cycleTable[0xFB] = 7;
  cycleTable[0xFC] = 4;
  cycleTable[0xFD] = 4;
  cycleTable[0xFE] = 7;
  cycleTable[0xFF] = 7;


  mnemonicStringTable[0] = "???";
  mnemonicStringTable[1] = "ADC";
  mnemonicStringTable[2] = "AND";
  mnemonicStringTable[3] = "ASL";
  mnemonicStringTable[4] = "BCC";
  mnemonicStringTable[5] = "BCS";
  mnemonicStringTable[6] = "BEQ";
  mnemonicStringTable[7] = "BIT";
  mnemonicStringTable[8] = "BMI";
  mnemonicStringTable[9] = "BNE";
  mnemonicStringTable[10] = "BPL";
  mnemonicStringTable[11] = "BRK";
  mnemonicStringTable[12] = "BVC";
  mnemonicStringTable[13] = "BVS";
  mnemonicStringTable[14] = "CLC";
  mnemonicStringTable[15] = "CLD";
  mnemonicStringTable[16] = "CLI";
  mnemonicStringTable[17] = "CLV";
  mnemonicStringTable[18] = "CMP";
  mnemonicStringTable[19] = "CPX";
  mnemonicStringTable[20] = "CPY";
  mnemonicStringTable[21] = "DEC";
  mnemonicStringTable[22] = "DEX";
  mnemonicStringTable[23] = "DEY";
  mnemonicStringTable[24] = "EOR";
  mnemonicStringTable[25] = "INC";
  mnemonicStringTable[26] = "INX";
  mnemonicStringTable[27] = "INY";
  mnemonicStringTable[28] = "JMP";
  mnemonicStringTable[29] = "JSR";
  mnemonicStringTable[30] = "LDA";
  mnemonicStringTable[31] = "LDX";
  mnemonicStringTable[32] = "LDY";
  mnemonicStringTable[33] = "LSR";
  mnemonicStringTable[34] = "NOP";
  mnemonicStringTable[35] = "ORA";
  mnemonicStringTable[36] = "PHA";
  mnemonicStringTable[37] = "PHP";
  mnemonicStringTable[38] = "PLA";
  mnemonicStringTable[39] = "PLP";
  mnemonicStringTable[40] = "ROL";
  mnemonicStringTable[41] = "ROR";
  mnemonicStringTable[42] = "RTI";
  mnemonicStringTable[43] = "RTS";
  mnemonicStringTable[44] = "SBC";
  mnemonicStringTable[45] = "SEC";
  mnemonicStringTable[46] = "SED";
  mnemonicStringTable[47] = "SEI";
  mnemonicStringTable[48] = "STA";
  mnemonicStringTable[49] = "STX";
  mnemonicStringTable[50] = "STY";
  mnemonicStringTable[51] = "TAX";
  mnemonicStringTable[52] = "TAY";
  mnemonicStringTable[53] = "TSX";
  mnemonicStringTable[54] = "TXA";
  mnemonicStringTable[55] = "TXS";
  mnemonicStringTable[56] = "TYA";
  mnemonicStringTable[57] = "*ALR";
  mnemonicStringTable[58] = "*ANC";
  mnemonicStringTable[59] = "*ANC";
  mnemonicStringTable[60] = "*ANE";
  mnemonicStringTable[61] = "*ARR";
  mnemonicStringTable[62] = "*DCP";
  mnemonicStringTable[63] = "*ISB";
  mnemonicStringTable[64] = "*LAS";
  mnemonicStringTable[65] = "*LAX";
  mnemonicStringTable[66] = "*LXA";
  mnemonicStringTable[67] = "*RLA";
  mnemonicStringTable[68] = "*RRA";
  mnemonicStringTable[69] = "*SAX";
  mnemonicStringTable[70] = "*SBX";
  mnemonicStringTable[71] = "*SHA";
  mnemonicStringTable[72] = "*SHX";
  mnemonicStringTable[73] = "*SHY";
  mnemonicStringTable[74] = "*SLO";
  mnemonicStringTable[75] = "*SRE";
  mnemonicStringTable[76] = "*TAS";
  mnemonicStringTable[77] = "*SBC";
  mnemonicStringTable[78] = "*NOP";
  mnemonicStringTable[79] = "*JAM";
}
