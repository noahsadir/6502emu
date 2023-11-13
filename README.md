# 6502emu

General purpose emulator for MOS 6502 based systems.

## Background

This is an extension of the NES emulator I wrote back in 2022.

There are better emulators out there! This is mainly just a pet project
to learn more about low-level stuff.

## Principles

- Dependency Isolation: Standard library functions are used sparingly, and
  external libraries (e.g. SDL) are kept isolated and abstracted away from
  the actual program logic.

- Portability: Since dependencies are kept at a minimum, the emulator should
  be fairly easy to port across numerous platforms, old and new.

- Configurability: Emulator can be loaded with a configuration file at runtime
  which extends its functionality without needing to make code changes.

## Configuration

The emulator can be configured with numerous parameters, specified in a
`.config` file with the following syntax for each property:

`PROPERTY_NAME=VALUE`

Currently, the following options are supported:

`Platform` ({NES}): The platform to emulate

`DISPLAY_width` (int): The unscaled display width provided to the target

`DISPLAY_height` (int): The unscaled display height provided to the target

`DISPLAY_scale` (int): The factor to scale the resolution (unknown to target)

`CPU_frequency` (int): The CPU frequency in hertz

`CPU_shouldCacheInstructions` ({true,false}): Cache instructions as bytecode

`DEBUG_shouldDisplayPerformance` ({true,false}): Display performance stats

`DEBUG_shouldDisplayDebugScreen` ({true,false}): Display debug information

`DEBUG_shouldTraceInstructions` ({true,false}): Record a CPU trace in ./debug

`DEBUG_shouldLimitFrequency` ({true,false}): Limit emulation frequency

`DEBUG_shouldDebugCPU` ({true,false}): Run CPU in platform-specific debug mode

## CPU Emulation

`src/mos6502.c` and `src/include/mos6502.h` contain the implementation for
the CPU.

It contains five primary external calls:

### Initialize

`void mos6502_init(void(*w)(uint16_t, uint8_t), uint8_t(*r)(uint16_t))`

Initialize the CPU.

Takes in a pointer to a write and read function with the following properties:

- `void w(uint16_t addr, uint8_t val)`:
  Write a byte to the specified 16-bit memory address

- `uint8_t r(uint16_t addr)`:
  Read a byte from the specified 16-bit memory address

### Execute Instruction

`void mos6502_step(char* traceStr, void(*c)(uint8_t))`

Read in an instruction from memory at the current PC value using the
functions passed during `mos6502_init`.

Takes in an optional trace string and mandatory callback.

The callback executes when the instruction finished and contains an 8-bit int
for the number of cycles elapsed during the instruction. This may be used to
synchronize graphics on a system like the NES.

*Note:* if using a trace string, ensure it is allocated with size >= 128

### Pull RESET

`void mos6502_interrupt_reset(void)`

Set stack pointer back to 0xFD and set the PC to the 16-bit word stored at
0xFFFC,0xFFFD

### Pull NMI

`void mos6502_interrupt_nmi(void)`

Push the current PC and status register to the stack and set the PC to the 
16-bit word stored at 0xFFFA,0xFFFB

### Pull IRQ

`void mos6502_interrupt_irq(void)`

Push the current PC and status register to the stack and set the PC to the 
16-bit word stored at 0xFFFE,0xFFFF