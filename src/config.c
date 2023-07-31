/**
 * config.c
 * 
 * @author Noah Sadir
 * @date 2023-07-30
 */

#include "include/config.h"

PlatformConfig CONFIG_PLATFORM;
DisplayConfig CONFIG_DISPLAY;
CpuConfig CONFIG_CPU;
DebugConfig CONFIG_DEBUG;

#if (SUPPRESS_CONFIG)

bool config_init(char* path) {
  return false;
}

#else

bool config_init(char* path) {
  char* cfgStr;
  if (fileio_readFileAsString(path, &cfgStr) != -1) {
    config_trimWhitespace(&cfgStr);
    char* token = strtok(cfgStr, "=");
    while (token != NULL) {
      config_parseSetting(token);
      token = strtok(NULL, "=");
    }
    free(cfgStr);

    if (CONFIG_DEBUG.shouldDisplayDebugScreen) {
      CONFIG_DISPLAY.screens = 2;
    } else {
      CONFIG_DISPLAY.screens = 1;
    }

    config_print();
    return true;
  } else {
    return false;
  }
}

void config_print() {
#if (!SUPPRESS_PRINTF)
  printf("\nPLATFORM: ");
  if (CONFIG_PLATFORM == NES) {
    printf("NES\n");
  } else {
    printf("(unknown)");
  }

  printf("\nDISPLAY\n");
  printf("- Resolution: %d x %d\n", CONFIG_DISPLAY.width, CONFIG_DISPLAY.height);
  printf("- Scale: %d\n", CONFIG_DISPLAY.scale);

  printf("\nCPU\n");
  printf("- Frequency: %ld Hz\n", CONFIG_CPU.frequency);
  printf("- Cache instructions? %s\n", CONFIG_CPU.shouldCacheInstructions ? "yes" : "no");

  printf("\nDEBUG\n");
  printf(" - Display performance stats? %s\n", CONFIG_DEBUG.shouldDisplayPerformance ? "yes" : "no");
  printf(" - Display debug screen? %s\n", CONFIG_DEBUG.shouldDisplayDebugScreen ? "yes" : "no");
  printf(" - Store instruction trace? %s\n", CONFIG_DEBUG.shouldTraceInstructions ? "yes" : "no");
  printf(" - Limit frequency? %s\n", CONFIG_DEBUG.shouldLimitFrequency ? "yes" : "no");

  printf("\n");
#endif
}

void config_parseSetting(char* line) {
  char* arg = line;
  char* val = strtok(NULL, ";");

  if (!strcmp(arg, "Platform")) {
    if (!strcmp(val, "NES")) {
      CONFIG_PLATFORM = NES;
    } else {
      config_throwInvalidConfigVal(arg, val);
    }
  } else if (!strcmp(arg, "DISPLAY_width")) {
    CONFIG_DISPLAY.width = atoi(val);
  } else if (!strcmp(arg, "DISPLAY_height")) {
    CONFIG_DISPLAY.height = atoi(val);
  } else if (!strcmp(arg, "DISPLAY_scale")) {
    CONFIG_DISPLAY.scale = atoi(val);
  } else if (!strcmp(arg, "CPU_frequency")) {
    CONFIG_CPU.frequency = atoi(val);
  } else if (!strcmp(arg, "CPU_shouldCacheInstructions")) {
    CONFIG_CPU.shouldCacheInstructions = config_boolFromString(arg, val);
  } else if (!strcmp(arg, "DEBUG_shouldDisplayPerformance")) {
    CONFIG_DEBUG.shouldDisplayPerformance = config_boolFromString(arg, val);
  } else if (!strcmp(arg, "DEBUG_shouldDisplayDebugScreen")) {
    CONFIG_DEBUG.shouldDisplayDebugScreen = config_boolFromString(arg, val);
  } else if (!strcmp(arg, "DEBUG_shouldTraceInstructions")) {
    CONFIG_DEBUG.shouldTraceInstructions = config_boolFromString(arg, val);
  } else if (!strcmp(arg, "DEBUG_shouldLimitFrequency")) {
    CONFIG_DEBUG.shouldLimitFrequency = config_boolFromString(arg, val);
  }else {
    config_throwInvalidConfigArg(arg);
  }
}

void config_trimWhitespace(char** strPtr) {
  char* str = *strPtr;
  char* newStr = malloc(sizeof(char) * strlen(str));
  
  int newStrLen = 0;
  for (int i = 0; str[i] != '\0'; i++) {
    if (str[i] != ' ' && str[i] != '\n' && str[i] != '\t' && str[i] != '\r') {
      newStr[newStrLen] = str[i];
      newStrLen += 1;
    }
  }

  newStr[newStrLen] = '\0';
  free(str);
  *strPtr = newStr;
}

void config_throwInvalidConfig() {
#if (!SUPPRESS_PRINTF)
  printf("CONFIGURATION ERROR: Invalid config file\n");
#endif
  exit(1);
}

void config_throwInvalidConfigArg(char* arg) {
#if (!SUPPRESS_PRINTF)
  printf("CONFIGURATION ERROR: Invalid argument '%s'\n", arg);
#endif
  exit(1);
}

void config_throwInvalidConfigVal(char* arg, char* val) {
#if (!SUPPRESS_PRINTF)
  printf("CONFIGURATION ERROR: Invalid value '%s' for property '%s'\n", val, arg);
#endif
  exit(1);
}

bool config_boolFromString(char* arg, char* val) {
  if (!strcmp(val, "true")) {
    return true;
  } else if (!strcmp(val, "false")) {
    return false;
  } else {
#if (!SUPPRESS_PRINTF)
    printf("CONFIGURATION ERROR: Invalid value '%s' for property '%s'\n", val, arg);
#endif
    exit(1);
    return false;
  }
}

#endif
