/**
 * config.h
 * 
 * Handle program settings.
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

#ifndef CONFIG_H
#define CONFIG_H

#include "global.h"
#include "fileio.h"

typedef enum {
  false = 0,
  true =  1
} bool;

typedef enum {
  K_ZERO    = 0,
  K_ONE     = 1,
  K_TWO     = 2,
  K_THREE   = 3,
  K_FOUR    = 4,
  K_FIVE    = 5,
  K_SIX     = 6,
  K_SEVEN   = 7,
  K_EIGHT   = 8,
  K_NINE    = 9,
  K_Q       = 10,
  K_W       = 11,
  K_E       = 12,
  K_R       = 13,
  K_T       = 14,
  K_Y       = 15,
  K_U       = 16,
  K_I       = 17,
  K_O       = 18,
  K_P       = 19,
  K_A       = 20,
  K_S       = 21,
  K_D       = 22,
  K_F       = 23,
  K_G       = 24,
  K_H       = 25,
  K_J       = 26,
  K_K       = 27,
  K_L       = 28,
  K_Z       = 29,
  K_X       = 30,
  K_C       = 31,
  K_V       = 32,
  K_B       = 33,
  K_N       = 34,
  K_M       = 35,
  K_LSHIFT  = 36,
  K_RSHIFT  = 37,
  K_RETURN  = 38,
  K_FN      = 39,
  K_LCTRL   = 40,
  K_LALT    = 41,
  K_LCMD    = 42,
  K_SPACE   = 43,
  K_RCMD    = 44,
  K_RALT    = 45,
  K_RCTRL   = 46,
  K_UP      = 47,
  K_LEFT    = 48,
  K_DOWN    = 49,
  K_RIGHT   = 50,
  K_CAPSLK  = 51,
  K_TAB     = 52,
  K_BACKSP  = 53,
  K_TILDE   = 54,
  K_MINUS   = 55,
  K_PLUS    = 56,
  K_LBRACK  = 57,
  K_RBRACK  = 58,
  K_PIPE    = 59,
  K_COLON   = 60,
  K_QUOTE   = 61,
  K_LCHEV   = 62,
  K_RCHEV   = 63,
  K_QMARK   = 64,
  K_F1      = 65,
  K_F2      = 66,
  K_F3      = 67,
  K_F4      = 68,
  K_F5      = 69,
  K_F6      = 70,
  K_F7      = 71,
  K_F8      = 72,
  K_F9      = 73,
  K_F10     = 74,
  K_F11     = 75,
  K_F12     = 76,
  K_ESCAPE  = 77
} Keyboard;

typedef enum {
  EMU_PLAT_NES
} PlatformConfig;

typedef struct {
  int width;
  int height;
  int scale;
  int screens;
} DisplayConfig;

typedef struct {
  long frequency;
  bool shouldCacheInstructions;
} CpuConfig;

typedef struct {
  bool shouldDisplayPerformance;
  bool shouldDisplayDebugScreen;
  bool shouldTraceInstructions;
  bool shouldLimitFrequency;
  bool shouldDebugCPU;
} DebugConfig;

extern PlatformConfig CONFIG_PLATFORM;
extern DisplayConfig CONFIG_DISPLAY;
extern CpuConfig CONFIG_CPU;
extern DebugConfig CONFIG_DEBUG;

bool config_init(char* json);
void config_print();
void config_trimWhitespace(char** str);
void config_parseSetting(char* line);
bool config_boolFromString(char* arg, char* val);
void config_throwInvalidConfig();
void config_throwInvalidConfigArg(char* arg);
void config_throwInvalidConfigVal(char* arg, char* val);

#endif
