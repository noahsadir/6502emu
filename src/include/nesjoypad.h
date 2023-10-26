/**
 * nesjoypad.h
 * 
 * Abstraction layer for joypad.
 * 
 * @author Noah Sadir
 * @date 2023-10-26
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
#include "global.h"
#include "config.h"

typedef enum {
  NJP_RIGHT   = BIT_MASK_8,
  NJP_LEFT    = BIT_MASK_7,
  NJP_DOWN    = BIT_MASK_6,
  NJP_UP      = BIT_MASK_5,
  NJP_START   = BIT_MASK_4,
  NJP_SELECT  = BIT_MASK_3,
  NJP_B       = BIT_MASK_2,
  NJP_A       = BIT_MASK_1
} NESJoypadButton;

uint8_t nesjoypad_get(void);
void nesjoypad_set(NESJoypadButton button, bool enabled);
void nesjoypad_setStrobeMode(bool mode);
