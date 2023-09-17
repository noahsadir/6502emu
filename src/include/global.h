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

// used for removing dependencies
#define SUPPRESS_PRINTF FALSE
#define SUPPRESS_FILEIO FALSE
#define SUPPRESS_CONFIG FALSE
#define SUPPRESS_64BIT  FALSE

#define INTERVALS_PER_SEC 60
#define TIMING_INTERVAL (1000000 / INTERVALS_PER_SEC)

#define BIT_FILL_0 0x0
#define BIT_FILL_1 0x1
#define BIT_FILL_2 0x3
#define BIT_FILL_3 0x7
#define BIT_FILL_4 0xF
#define BIT_FILL_5 0x1F
#define BIT_FILL_6 0x3F
#define BIT_FILL_7 0x7F
#define BIT_FILL_8 0xFF

#define BIT_MASK_1 0x1
#define BIT_MASK_2 0x2
#define BIT_MASK_3 0x4
#define BIT_MASK_4 0x8
#define BIT_MASK_5 0x10
#define BIT_MASK_6 0x20
#define BIT_MASK_7 0x40
#define BIT_MASK_8 0x80

#endif
