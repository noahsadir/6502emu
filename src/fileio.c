/**
 * fileio.c
 * 
 * @author Noah Sadir
 * @date 2023-07-30
 */

#include "include/fileio.h"

#if (SUPPRESS_EXTIO)

int fileio_readFileAsString(char* path, char** output) {
  return -1;
}

int fileio_readFileAsBinary(char* path, uint8_t** output) {
  return -1;
}

int fileio_listFilesInPath(char* path, char** output) {
  return -1;
}

int fileio_writeStringToFile(char* path, char* value, uint8_t append) {
  return -1;
}

#else

int fileio_writeStringToFile(char* path, char* value, uint8_t append) {
  FILE* fp;
  fp = fopen(path, (append == 0) ? "w" : "a");

  if (fp == NULL) return -1;

  fprintf(fp, "%s", value);

  fclose(fp);

  return 0;
}

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

int fileio_readFileAsBinary(char* path, uint8_t** output) {
  int binarySize = 0;
  FILE* fp;
  fp = fopen(path, "rb");

  if (fp == NULL) return -1;

  uint8_t buffer[16];
  uint32_t n;

  // setup data struct
  fseek(fp, 0L, SEEK_END);
  binarySize = ftell(fp);
  (*output) = malloc(sizeof(char) * binarySize);
  rewind(fp);

  // populate binary object with file data
  uint32_t count = 0;
  while ((n = fread(buffer, 1, 16, fp)) > 0) {
    for (int i = 0; i < 16; i++) {
      (*output)[count] = buffer[i];
      count += 1;
    }
  }
  return binarySize;
}

int fileio_listFilesInPath(char* path, char* output[FILEIO_MAX_COUNT]) {
  struct dirent *de;
  DIR *dr = opendir(path);
  if (dr == NULL) {
    return -1;
  }

  int count = 0;
  for (int i = 0; i < FILEIO_MAX_COUNT - 1 && (de = readdir(dr)) != NULL; i++) {
    output[i] = de->d_name;
    output[i + 1] = NULL;
    count += 1;
  }
  
  return count;
}

#endif
