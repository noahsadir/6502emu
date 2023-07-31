/**
 * fileio.c
 * 
 * @author Noah Sadir
 * @date 2023-07-30
 */

#include "include/fileio.h"

#if (SUPPRESS_FILEIO)

int fileio_readFileAsString(char* path, char** output) {
  return -1;
}

#else

int fileio_readFileAsString(char* path, char** output) {
  int binarySize = 0;
  FILE* fp;
  fp = fopen(path, "rb");

  if (fp == NULL) return -1;

  uint8_t buffer[16];
  uint32_t n;

  // setup data struct
  fseek(fp, 0L, SEEK_END);
  binarySize = ftell(fp) + 1;
  (*output) = (char*)malloc(sizeof(char) * binarySize);
  rewind(fp);

  // populate binary object with file data
  uint32_t count = 0;
  while ((n = fread(buffer, 1, 16, fp)) > 0) {
    for (int i = 0; i < 16; i++) {
      (*output)[count] = buffer[i];
      count += 1;
    }
  }
  (*output)[binarySize - 1] = '\0';
  return binarySize;
}

#endif
