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

#if (!SUPPRESS_CONFIG)
#include "fileio.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#endif

typedef enum {
  false = 0,
  true =  1
} bool;

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
