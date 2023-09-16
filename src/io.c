/**
 * io.c
 * 
 * @author Noah Sadir
 * @date 2023-07-30
 */

#include "include/io.h"

uint32_t* BITMAP0;
uint32_t* BITMAP1;
uint32_t* BITMAP2;
uint32_t* BITMAP3;
char* OVERLAY_MSG;
char* PANIC_MSG;
bool PANIC_MODE;

SDL_Window* window;
SDL_Surface* surface;

void io_init() {
  SDL_Init(SDL_INIT_VIDEO);
  window = SDL_CreateWindow(
    "Emulator",
    SDL_WINDOWPOS_UNDEFINED,
    SDL_WINDOWPOS_UNDEFINED,
    CONFIG_DISPLAY.width * CONFIG_DISPLAY.scale * (CONFIG_DISPLAY.screens == 1 ? 1 : 2),
    CONFIG_DISPLAY.height * CONFIG_DISPLAY.scale * (CONFIG_DISPLAY.screens <= 2 ? 1 : 2),
    SDL_WINDOW_SHOWN
  );
  surface = SDL_GetWindowSurface(window);

  OVERLAY_MSG = "";
  PANIC_MSG = "";
  PANIC_MODE = false;
  BITMAP0 = malloc(sizeof(uint32_t) * CONFIG_DISPLAY.width * CONFIG_DISPLAY.height);
  BITMAP1 = malloc(sizeof(uint32_t) * CONFIG_DISPLAY.width * CONFIG_DISPLAY.height);
  BITMAP2 = malloc(sizeof(uint32_t) * CONFIG_DISPLAY.width * CONFIG_DISPLAY.height);
  BITMAP3 = malloc(sizeof(uint32_t) * CONFIG_DISPLAY.width * CONFIG_DISPLAY.height);
}

int io_pollInput(SDL_KeyCode* key) {
  SDL_Event event;
  if (SDL_PollEvent(&event)) {

    if (event.type == SDL_KEYDOWN) {
      *key = event.key.keysym.sym;
      return 1;
    } else if (event.type == SDL_KEYUP) {
      *key = event.key.keysym.sym;
      return -1;
    }

    if (event.type == SDL_QUIT) {
      io_kill();
      exit(0);
    }
  }

  return 0;
}

void io_render() {
  SDL_FreeSurface(surface);
  uint32_t* pixels = (uint32_t*)surface->pixels;
  SDL_LockSurface(surface);

  if (PANIC_MODE) {
    for (int i = 0; i < (CONFIG_DISPLAY.width * CONFIG_DISPLAY.height); i++) {
      BITMAP0[i] = (rand() % 2) ? 0xFFFFFF : 0x000000;
    }
    io_drawString(PANIC_MSG, 0);
  } else {
    io_drawString(OVERLAY_MSG, CONFIG_DISPLAY.screens - 1);
  }

  for (int i = 0; i < CONFIG_DISPLAY.screens; i++) {
    io_drawScreen(i, pixels);
  }

  SDL_UnlockSurface(surface);
  SDL_UpdateWindowSurface(window);
}

void io_drawString(char* str, int screen) {
  uint32_t* bmp;

  if (screen == 0) {
    bmp = BITMAP0;
  } else if (screen == 1) {
    bmp = BITMAP1;
  } else if (screen == 2) {
    bmp = BITMAP2;
  } else if (screen == 3) {
    bmp = BITMAP3;
  }

  int charPos = 0;  
  int charsPerRow = CONFIG_DISPLAY.width / 8;
  for (int i = 0; str[i] != '\0'; i++) {
    if (str[i] == '\n') {
      charPos = charPos - (charPos % charsPerRow) + charsPerRow;
    } else if (str[i] == '\t') {
      charPos += 1;
    } else {
      io_drawChar(str[i], charPos, bmp);
      charPos += 1;
    }
  }
}

void io_drawChar(char chr, int charPos, uint32_t* bmp) {
  int charsPerRow = CONFIG_DISPLAY.width / 8;
  int startX = (charPos % charsPerRow) * 8;
  int startY = (charPos / charsPerRow) * 8;

  uint64_t charPix = font[chr & 127];
  int8_t shift = 63;
  for (int row = 0; row < 8; row++) {
    for (int col = 0; col < 8; col++) {
      int pos = ((startY + row) * CONFIG_DISPLAY.width) + startX + col;
      bmp[pos] = ((charPix >> shift) & 1) ? 0xFFFFFF : 0x000000;
      shift -= 1;
    }
  }
}

void io_drawScreen(int screen, uint32_t* pixels) {
  uint32_t* bmp;

  if (screen == 0) {
    bmp = BITMAP0;
  } else if (screen == 1) {
    bmp = BITMAP1;
  } else if (screen == 2) {
    bmp = BITMAP2;
  } else if (screen == 3) {
    bmp = BITMAP3;
  }

  for (int x = 0; x < CONFIG_DISPLAY.width; x++) {
    for (int y = 0; y < CONFIG_DISPLAY.height; y++) {
      io_drawPixel(x, y, screen, bmp[(y * CONFIG_DISPLAY.width) + x], pixels);
    }
  }
}

void io_drawPixel(int x, int y, int screen, uint32_t color, uint32_t* pixels) {
  // find starting x & y in screen
  int screenX = (screen % 2 == 0) ? 0 : (CONFIG_DISPLAY.width * CONFIG_DISPLAY.scale);
  int screenY = (screen < 2) ? 0 : (CONFIG_DISPLAY.height * CONFIG_DISPLAY.scale);
  
  int pixelsWidth = CONFIG_DISPLAY.width * CONFIG_DISPLAY.scale * (CONFIG_DISPLAY.screens == 1 ? 1 : 2);

  // find starting x & y for pixel
  int pixelX = screenX + (x * CONFIG_DISPLAY.scale);
  int pixelY = screenY + (y * CONFIG_DISPLAY.scale);

  for (int row = 0; row < CONFIG_DISPLAY.scale; row++) {
    for (int col = 0; col < CONFIG_DISPLAY.scale; col++) {
      pixels[((pixelY + row) * pixelsWidth) + pixelX + col] = color;
    }
  }
}

void io_kill() {
  free(BITMAP0);
  free(BITMAP1);
  free(BITMAP2);
  free(BITMAP3);
}

void io_clear() {
  OVERLAY_MSG = "";
  uint32_t* bmp;

  for (int screen = 0; screen < CONFIG_DISPLAY.screens; screen++) {
    if (screen == 0) {
      bmp = BITMAP0;
    } else if (screen == 1) {
      bmp = BITMAP1;
    } else if (screen == 2) {
      bmp = BITMAP2;
    } else if (screen == 3) {
      bmp = BITMAP3;
    }
    for (int i = 0; i < (CONFIG_DISPLAY.width * CONFIG_DISPLAY.height); i += 1) {
      bmp[i] = 0x000000;
    }
  }
  io_render();
}

void io_panic(char* str) {
  PANIC_MODE = true;

  char fullStr[1024];
  fullStr[0] = '\0';
  strcat(fullStr, "\n\tpanic!\n\t");
  strcat(fullStr, str);
  PANIC_MSG = fullStr;

  while (true) {
    SDL_KeyCode key;
    io_pollInput(&key);
    io_render();
  }
}
