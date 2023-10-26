/**
 * nesjoypad.c
 * 
 * @author Noah Sadir
 * @date 2023-10-26
 */

#include "include/nesjoypad.h"

bool strobeMode = false;
uint8_t shiftIndex = 0;
uint8_t state = 0;

uint8_t nesjoypad_get(void) {
  if (shiftIndex > 7) {
    shiftIndex = 0;
  }
  uint8_t result = (state >> shiftIndex) & 1;
  if (!strobeMode) shiftIndex += 1;
  return result;
}

void nesjoypad_set(NESJoypadButton button, bool enabled) {
  if (enabled) {
    state |= button;
  } else {
    state &= ~button;
  }
}

void nesjoypad_setStrobeMode(bool mode) {
  strobeMode = mode;
  if (strobeMode) {
    shiftIndex = 0;
  }
}
