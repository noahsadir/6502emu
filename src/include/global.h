/**
 * global.h
 * 
 * Handle global definitions.
 * 
 * @author Noah Sadir
 * @date 2023-07-30
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

#ifndef GLOBAL_H
#define GLOBAL_H

#define FALSE 0
#define TRUE  1

#define SDL2 0

#define IO_LIBRARY SDL2

// fallback for if SUPPRESS_EXTIO is set
#define FALLBACK_PLATFORM EMU_PLAT_NES
#define FALLBACK_NES_ROM_HEADER "include/roms/nestest.h"

// Remove any I/O-dependent operations from binary
#define SUPPRESS_EXTIO  FALSE

// Remove dependencies to unistd.h and sys/time.h
#define SUPPRESS_TIMING FALSE

// Remove dependencies on 64-bit values
#define SUPPRESS_64BIT  FALSE

// Always prefer space-efficient solutions vs time-efficient
#define MINIMIZE_MEMORY FALSE

#include <stdint.h>
#include <string.h>

#if (!SUPPRESS_TIMING)
#include <sys/time.h>
#include <unistd.h>
#endif

#if (!SUPPRESS_EXTIO)
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#endif

#define INTERVALS_PER_SEC 60
#define TIMING_INTERVAL (1000000 / INTERVALS_PER_SEC)

#define force_inline __attribute__((always_inline)) inline

#define GET_bit0(val) (val & 1)
#define GET_bit1(val) ((val >> 1) & 1)
#define GET_bit2(val) ((val >> 2) & 1)
#define GET_bit3(val) ((val >> 3) & 1)
#define GET_bit4(val) ((val >> 4) & 1)
#define GET_bit5(val) ((val >> 5) & 1)
#define GET_bit6(val) ((val >> 6) & 1)
#define GET_bit7(val) ((val >> 7) & 1)

#define SET_bit0(val, f) ((val & ~BIT_MASK_0) | ((f & 1) << 0))
#define SET_bit1(val, f) ((val & ~BIT_MASK_1) | ((f & 1) << 1))
#define SET_bit2(val, f) ((val & ~BIT_MASK_2) | ((f & 1) << 2))
#define SET_bit3(val, f) ((val & ~BIT_MASK_3) | ((f & 1) << 3))
#define SET_bit4(val, f) ((val & ~BIT_MASK_4) | ((f & 1) << 4))
#define SET_bit5(val, f) ((val & ~BIT_MASK_5) | ((f & 1) << 5))
#define SET_bit6(val, f) ((val & ~BIT_MASK_6) | ((f & 1) << 6))
#define SET_bit7(val, f) ((val & ~BIT_MASK_7) | ((f & 1) << 7))

#define BIT_FILL_0 0x0
#define BIT_FILL_1 0x1
#define BIT_FILL_2 0x3
#define BIT_FILL_3 0x7
#define BIT_FILL_4 0xF
#define BIT_FILL_5 0x1F
#define BIT_FILL_6 0x3F
#define BIT_FILL_7 0x7F
#define BIT_FILL_8 0xFF

#define BIT_MASK_0 0x1
#define BIT_MASK_1 0x2
#define BIT_MASK_2 0x4
#define BIT_MASK_3 0x8
#define BIT_MASK_4 0x10
#define BIT_MASK_5 0x20
#define BIT_MASK_6 0x40
#define BIT_MASK_7 0x80

#endif
