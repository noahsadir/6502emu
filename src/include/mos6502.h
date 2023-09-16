/**
 * mos6502.h
 * 
 * Main controller for NES emulation.
 * 
 * @author Noah Sadir
 * @date 2023-08-03
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

#ifndef MOS6502_H
#define MOS6502_H

#include "global.h"
#include "config.h"
#include <stdint.h>
#include <stdlib.h>

typedef enum {
  CPUSTAT_CARRY       = BIT_MASK_1,
  CPUSTAT_ZERO        = BIT_MASK_2,
  CPUSTAT_NO_INTRPT   = BIT_MASK_3,
  CPUSTAT_DECIMAL     = BIT_MASK_4,
  CPUSTAT_BREAK       = BIT_MASK_5,
  CPUSTAT_BREAK2      = BIT_MASK_6,
  CPUSTAT_OVERFLOW    = BIT_MASK_7,
  CPUSTAT_NEGATIVE    = BIT_MASK_8,
} CPUStatusFlag;

typedef enum {
  AM_UNSET          = 0,
  AM_ACCUMULATOR    = 1,
  AM_IMPLIED        = 2,
  AM_IMMEDIATE      = 3,
  AM_ABSOLUTE       = 4,
  AM_ZERO_PAGE      = 5,
  AM_RELATIVE       = 6,
  AM_ABS_INDIRECT   = 7,
  AM_ABS_X          = 8,
  AM_ABS_Y          = 9,
  AM_ZP_X           = 10,
  AM_ZP_Y           = 11,
  AM_ZP_X_INDIRECT  = 12,
  AM_ZP_INDIRECT_Y  = 13
} CPUAddressingMode;

typedef enum {
  I_UNSET = 0,
  I_ADC   = 1,
  I_AND   = 2,
  I_ASL   = 3,
  I_BCC   = 4,
  I_BCS   = 5,
  I_BEQ   = 6,
  I_BIT   = 7,
  I_BMI   = 8,
  I_BNE   = 9,
  I_BPL   = 10,
  I_BRK   = 11,
  I_BVC   = 12,
  I_BVS   = 13,
  I_CLC   = 14,
  I_CLD   = 15,
  I_CLI   = 16,
  I_CLV   = 17,
  I_CMP   = 18,
  I_CPX   = 19,
  I_CPY   = 20,
  I_DEC   = 21,
  I_DEX   = 22,
  I_DEY   = 23,
  I_EOR   = 24,
  I_INC   = 25,
  I_INX   = 26,
  I_INY   = 27,
  I_JMP   = 28,
  I_JSR   = 29,
  I_LDA   = 30,
  I_LDX   = 31,
  I_LDY   = 32,
  I_LSR   = 33,
  I_NOP   = 34,
  I_ORA   = 35,
  I_PHA   = 36,
  I_PHP   = 37,
  I_PLA   = 38,
  I_PLP   = 39,
  I_ROL   = 40,
  I_ROR   = 41,
  I_RTI   = 42,
  I_RTS   = 43,
  I_SBC   = 44,
  I_SEC   = 45,
  I_SED   = 46,
  I_SEI   = 47,
  I_STA   = 48,
  I_STX   = 49,
  I_STY   = 50,
  I_TAX   = 51,
  I_TAY   = 52,
  I_TSX   = 53,
  I_TXA   = 54,
  I_TXS   = 55,
  I_TYA   = 56,
  I_ILL_ALR   = 57,
  I_ILL_ANC   = 58,
  I_ILL_ANC2  = 59,
  I_ILL_ANE   = 60,
  I_ILL_ARR   = 61,
  I_ILL_DCP   = 62,
  I_ILL_ISC   = 63,
  I_ILL_LAS   = 64,
  I_ILL_LAX   = 65,
  I_ILL_LXA   = 66,
  I_ILL_RLA   = 67,
  I_ILL_RRA   = 68,
  I_ILL_SAX   = 69,
  I_ILL_SBX   = 70,
  I_ILL_SHA   = 71,
  I_ILL_SHX   = 72,
  I_ILL_SHY   = 73,
  I_ILL_SLO   = 74,
  I_ILL_SRE   = 75,
  I_ILL_TAS   = 76,
  I_ILL_USBC  = 77,
  I_ILL_NOP   = 78,
  I_ILL_JAM   = 79
} CPUMnemonic;

typedef struct {
  uint8_t a;
  uint8_t x;
  uint8_t y;
  uint16_t pc;
  uint8_t s;
  uint8_t p;
} CPURegisters;

typedef struct {
  CPUMnemonic mnemonic;
  CPUAddressingMode addressingMode;
  uint8_t count;
  uint8_t data[3];
} Bytecode;

typedef struct {
  Bytecode* bytecodes;
  uint16_t addrMap[65536];
  uint16_t bytecodeCount;
} BytecodeProgram;

extern CPURegisters reg;

void mos6502_init(void(*w)(uint16_t, uint8_t), uint8_t(*r)(uint16_t));
void mos6502_step(char* traceStr, void(*c)(uint8_t));
void mos6502_interrupt_reset();
void mos6502_interrupt_nmi();
void mos6502_interrupt_irq();
void mos6502_setflag(CPUStatusFlag flag, uint8_t value);
void mos6502_stack_push(uint8_t data);
uint8_t mos6502_stack_pop();
uint16_t mos6502_read16(uint16_t addr);
void mos6502_execute(Bytecode* bytecode);

/**
 * @brief Decode an instruction and translate into bytecode
 * 
 * @param bytecode pointer to the Bytecode struct storing the software-friendly
 *                 translation of the instruction. Pass in NULL to disable.
 * @param assemblyResult pointer to string storing the assembly translated
 *                       from machine code. Pass in NULL to disable.
 * @param byteCount pointer to int storing number of bytes used by the
 *                  instruction. Pass in NULL to disable.
 * @param pc the location of the instruction in memory
 */
void mos6502_decode(Bytecode* bytecode, char* assemblyResult, uint8_t* byteCount, uint16_t pc);

/**
 * @brief Configure tables for translating opcodes
 */
void mos6502_configureTables();

#endif
