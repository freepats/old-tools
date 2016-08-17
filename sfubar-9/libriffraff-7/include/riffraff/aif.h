#ifndef __AIF_H__
#define __AIF_H__
#include "rifftypes.h"

typedef struct {
  myDWORD timestamp;
} fver_t;

typedef struct {
  /* AIFF attributes */
  myWORD numChannels;
  myDWORD numSampleFrames;
  myWORD sampleSize; /* bits */
  myWORD samplePointSize; /* sampleSize padded and converted to bytes */
  myIEEE80 sampleRate;
  double double_sampleRate;

  /* AIFC attributes */
  myFOURCC compressionType;
  myPSTRING compressionName;
} comm_t;

typedef struct {
  myDWORD offset;
  myDWORD blockSize;
  long fpos_WaveformData;
  long bytes_WaveformData;
} ssnd_t;

typedef struct {
  myWORD id;
  myDWORD position;
  myPSTRING markerName;
} mrkr_t;

typedef struct {
  myWORD numMarkers;
  mrkr_t **markers;
} mark_t;

#define AIF_LOOP_NONE 0
#define AIF_LOOP_FORWARD 1
#define AIF_LOOP_PINGPONG 2
#define AIF_LOOP_MAX 2

typedef struct {
  myWORD playMode;
  myWORD beginLoop;
  myWORD endLoop;
} loop_t;

typedef struct {
  myCHAR baseNote; /* MIDI note 0 to 127 */
  myCHAR detune; /* pitch shift in cents, -50 to 50 */
  myCHAR lowNote; /* MIDI note */
  myCHAR highNote; /* MIDI note */
  myCHAR lowVelocity; /* MIDI velocity 1 to 127 */
  myCHAR highVelocity; /* MIDI velocity */
  mySHORT gain; /* decibels */
  loop_t sustainLoop;
  loop_t releaseLoop;
} inst_t;

typedef struct {
  myDWORD timeStamp;
  myWORD marker;
  myWORD count;
  char *text;
} cmnt_t;

typedef struct {
  myWORD numComments;
  cmnt_t **comments;
} comt_t;

typedef struct {
  fver_t fver;
  comm_t comm;
  mark_t mark;
  inst_t inst;
  comt_t comt;
  char *name;
  char *auth;
  char *copy;
  char *anno;
  ssnd_t ssnd;

  chunk_t *src;
  chunk_t *dst;
  chunk_t *tmp;
  void (*destroy)(void *);
  void (*header_set)(void *, myWORD, myDWORD, myWORD, myIEEE80, myDWORD,
    char *, char *);
  int (*header_read)(void *, FILE *);
  void (*header_write)(void *, FILE *);
  void (*sample_calc)(void *);
  int (*marker_find)(void *, myWORD);
  tree_t *tree;
} aif_t;

#define ieee80_44100 "\x40\x0E\xAC\x44\x00\x00\x00\x00\x00\x00"
#define AIFC_VERSION1 2726318400UL
#define COMPRESSION_TYPE_NONE "NONE"
#define COMPRESSION_NAME_NOT "not compressed"
extern aif_t *aif_new(void);
#endif
