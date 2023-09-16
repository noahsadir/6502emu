/**
 * main.c
 * 
 * @author Noah Sadir
 * @date 2023-07-30
 */

#include "include/main.h"

int main(int argc, char* argv[]) {
  if (argc < 2) {
    printf("ERROR: Please specify config path\n");
    return EXIT_FAILURE;
  } else if (argc < 3) {
    printf("ERROR: Please specify rom path\n");
    return EXIT_FAILURE;
  }

  if (config_init(argv[1])) {
    io_init();

    if (CONFIG_PLATFORM == EMU_PLAT_NES) {
      nes_init(argv[2]);
    }

    io_panic("Emulation halted.");
  } else {
    printf("ERROR: Unable to parse config file at '%s'\n", argv[1]);
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
