#ifndef __WAV_H__
#define __WAV_H__
#include "rifftypes.h"

#ifndef WAVE_FORMAT_PCM
#define WAVE_FORMAT_PCM 0x0001
#endif

typedef struct {
  myWORD wFormatTag;
  myWORD wChannels;
  myDWORD dwSamplesPerSec;
  myDWORD dwAvgBytesPerSec;
  myWORD wBlockAlign;
  myWORD wBitsPerSample;
} fmt_t;

typedef struct {
  long offset; /* bytes */
  long size; /* bytes */
} data_t;

typedef struct {
  fmt_t fmt;
  data_t data;
  void (*destroy)(void *);
  void (*header_set)(void *, myWORD, myWORD, myDWORD, long);
  int (*header_read)(void *, FILE *);
  void (*header_write)(void *, FILE *);
  chunk_t *src;
  chunk_t *dst;
  chunk_t *tmp;
  tree_t *tree;
} wav_t;

extern wav_t *wav_new(void);
#endif
