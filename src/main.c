/**
 * main.c
 * 
 * @author Noah Sadir
 * @date 2023-07-30
 */

#include "include/main.h"

int main(int argc, char* argv[]) {
  // manually set config & rom if suppressed
#if (SUPPRESS_EXTIO)
  CONFIG_PLATFORM = FALLBACK_PLATFORM;
  if (CONFIG_PLATFORM == EMU_PLAT_NES) {
    CONFIG_DISPLAY.height = 240;
    CONFIG_DISPLAY.width = 256;
    CONFIG_DISPLAY.scale = 1;
    CONFIG_DISPLAY.screens = 1;
    CONFIG_CPU.frequency = 1789773;
    CONFIG_CPU.shouldCacheInstructions = false;
    CONFIG_DEBUG.shouldDebugCPU = false;
    CONFIG_DEBUG.shouldDisplayDebugScreen = false;
    CONFIG_DEBUG.shouldDisplayPerformance = false;
    CONFIG_DEBUG.shouldLimitFrequency = false;
  }
#else
  if (argc < 2) {
    printf("ERROR: Please specify config path\n");
    return EXIT_FAILURE;
  } else if (argc < 3) {
    printf("ERROR: Please specify rom path\n");
    return EXIT_FAILURE;
  }
#endif

  if (SUPPRESS_EXTIO || config_init(argv[1])) {
    io_init();
    if (CONFIG_PLATFORM == EMU_PLAT_NES) {
      nes_init(SUPPRESS_EXTIO ? NULL : argv[2]);
    }

    io_panic("Emulation halted.");
  } else {
#if (!SUPPRESS_EXTIO)
    printf("ERROR: Unable to parse config file.\n");
#endif
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
