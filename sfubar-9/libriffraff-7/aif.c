#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <trap.h>
#include "bits.h"
#include "riff.h"
#include "rifftypes.h"
#include "aif.h"
#include "ieee80.h"

tree_item aif_items[] = {
{ 0, "FORM", "AIFF", NULL, NULL },
{ 0, "FORM", "AIFC", NULL, NULL },
{ 1, "FVER", NULL, NULL, NULL },
{ 1, "COMM", NULL, NULL, NULL },
{ 1, "MARK", NULL, NULL, NULL },
{ 1, "INST", NULL, NULL, NULL },
{ 1, "COMT", NULL, NULL, NULL },
{ 1, "NAME", NULL, NULL, NULL },
{ 1, "AUTH", NULL, NULL, NULL },
{ 1, "(c) ", NULL, NULL, NULL },
{ 1, "ANNO", NULL, NULL, NULL },
{ 1, "SSND", NULL, NULL, NULL },
{ -1, "", NULL, NULL, NULL }
};

tree_container aif_containers[] = { "FORM", "" };

int aif_marker_find(void *vaif, myWORD marker_id)
{
  aif_t *aif = vaif;
  int i;
  int retval = -1;
  for (i = 0; i < aif->mark.numMarkers; i++) {
    if (aif->mark.markers[i] == NULL)
      continue;
    if (aif->mark.markers[i]->id != marker_id)
      continue;
    retval = i;
    break;
  }
  return retval;
}

char *aif_marker_allocate_and_read(void *vaif, int i, char *p)
{
  aif_t *aif = vaif;
  mrkr_t *marker = malloc(sizeof(mrkr_t));
  myWORD tw;
  myDWORD tdw;
  int r;

  p = my_get_WORD_BE(&tw, p);
  if (tw < 1)
    trap_exit("marker at position %d has invalid id", i + 1);
  r = aif_marker_find(aif, tw);
  if (r != -1)
    trap_warn("duplicate marker id %d (%d, %d)", tw, r + 1, i + 1);
  marker->id = tw;

  p = my_get_DWORD_BE(&tdw, p);
  if (tdw < 0) {
    trap_warn("marker %d too small, adjusting", marker->id);
    tdw = 0;
  }
  if (tdw > aif->comm.numSampleFrames) {
    trap_warn("marker %d too large, adjusting", marker->id);
    tdw = aif->comm.numSampleFrames;
  }
  marker->position = tdw;

  p = my_get_PSTRING(&(marker->markerName), p);

  aif->mark.markers[i] = marker;
  return p;
}

char *aif_loop_read(void *vaif, loop_t *loop, const char *prefix, char *p)
{
  aif_t *aif = vaif;
  int r;

  p = my_get_WORD_BE(&(loop->playMode), p);
  if (loop->playMode > AIF_LOOP_MAX) {
    trap_warn("%s.playMode invalid (%d), adjusting", prefix, loop->playMode);
    aif->inst.sustainLoop.playMode = AIF_LOOP_NONE;
  }
  p = my_get_WORD_BE(&(loop->beginLoop), p);
  if (loop->beginLoop != 0) {
    r = aif_marker_find(vaif, loop->beginLoop);
    if (r == -1) {
      trap_warn("%s.beginLoop invalid marker id (%d), adjusting", prefix,
         loop->beginLoop);
      loop->beginLoop = 0;
    }
  }
  p = my_get_WORD_BE(&(loop->endLoop), p);
  r = aif_marker_find(vaif, loop->endLoop);
  if (loop->endLoop != 0) {
    if (r == -1) {
      trap_warn("%s.endLoop invalid marker id (%d), adjusting", prefix,
        loop->endLoop);
      loop->endLoop = 0;
    }
  }
  return p;
}

char *aif_comment_allocate_and_read(void *vaif, int i, char *p)
{
  aif_t *aif = vaif;
  cmnt_t *comment = malloc(sizeof(cmnt_t));
  char pad;
  int r;

  p = my_get_DWORD_BE(&(comment->timeStamp), p);
  p = my_get_WORD_BE(&(comment->marker), p);
  if (comment->marker != 0) {
    r = aif_marker_find(aif, comment->marker);
    if (r == -1)
      trap_warn("comment %d marker id invalid, ignoring", i + 1);
  }
  p = my_get_WORD_BE(&(comment->count), p);
  comment->text = malloc(comment->count);
  p = my_get_CHAR(comment->text, p, comment->count);
  if (!is_even(comment->count))
    p = my_get_CHAR(&pad, p, 1);

  aif->comt.comments[i] = comment;
  return p;
}

int aif_header_read(void *vaif, FILE *ifp)
{
  void *p;
  aif_t *aif = vaif;
  char buffer[sizeof(aif->ssnd)];
  int is_aiff = 0;
  int is_aifc = 0;
  chunk_t *chunk_src;
  chunk_t *chunk_tmp;
  int n;
  int i;

  chunk_src = chunk_new(NULL, ON_DISK);
  chunk_src->endian = MY_BIG_ENDIAN;
  chunk_src->header_read(chunk_src, ifp);
  aif->tree->read(aif->tree, chunk_src, 0, ifp);
  /* chunk_src is freed when aif->tree is destroyed */

  n = aif->tree->ckID_find(aif->tree, "FORM", "AIFF", 0);
  if (n > -1) {
    is_aiff = 1;
  } else {
    n = aif->tree->ckID_find(aif->tree, "FORM", "AIFC", 0);
    if (n > -1) {
      is_aifc = 1;
    }
  }
 
  if (n == -1) {
    trap_warn("header_read: no FORM chunk with AIFF/C form");
    return 0;
  }

  if (is_aifc) {
    n = aif->tree->ckID_find(aif->tree, "FVER", NULL, 1);
    if (n == -1) {
      trap_warn("header_read: no FVER chunk");
      return 0;
    }
    chunk_tmp = aif->tree->items[n].chunk;
    chunk_tmp->data_read(chunk_tmp, ifp);
    p = chunk_tmp->data;
    p = my_get_DWORD_BE(&(aif->fver.timestamp), p);
  }
  if (aif->fver.timestamp < AIFC_VERSION1)
    trap_warn("header_read: unusual fver.timestamp %lu, expected %lu",
      (unsigned long)aif->fver.timestamp, (unsigned long)AIFC_VERSION1);

  n = aif->tree->ckID_find(aif->tree, "COMM", NULL, 1);
  if (n == -1) {
    trap_warn("header_read: no COMM chunk");
    return 0;
  }
  chunk_tmp = aif->tree->items[n].chunk;
  chunk_tmp->data_read(chunk_tmp, ifp);
  p = chunk_tmp->data;
  p = my_get_WORD_BE(&(aif->comm.numChannels), p);
  p = my_get_DWORD_BE(&(aif->comm.numSampleFrames), p);
  p = my_get_WORD_BE(&(aif->comm.sampleSize), p);
  p = my_get_IEEE80(&(aif->comm.sampleRate), p);
  if (is_aifc) {
    p = my_get_CHAR(aif->comm.compressionType.str, p, sizeof(myFOURCC));
    p = my_get_PSTRING(&(aif->comm.compressionName), p);
  }

  n = aif->tree->ckID_find(aif->tree, "MARK", NULL, 1);
  if (n != -1) {
    chunk_tmp = aif->tree->items[n].chunk;
    chunk_tmp->data_read(chunk_tmp, ifp);
    p = chunk_tmp->data;
    p = my_get_WORD_BE(&(aif->mark.numMarkers), p);
    aif->mark.markers = malloc(aif->mark.numMarkers * sizeof(mrkr_t *));
    for (i = 0; i < aif->mark.numMarkers; i++)
      aif->mark.markers[i] = NULL;
    for (i = 0; i < aif->mark.numMarkers; i++)
      p = aif_marker_allocate_and_read(aif, i, p);
  }

  n = aif->tree->ckID_find(aif->tree, "INST", NULL, 1);
  if (n != -1) {
    chunk_tmp = aif->tree->items[n].chunk;
    chunk_tmp->data_read(chunk_tmp, ifp);
    p = chunk_tmp->data;
    p = my_get_CHAR(&(aif->inst.baseNote), p, 1);
    p = my_get_CHAR(&(aif->inst.detune), p, 1);
    p = my_get_CHAR(&(aif->inst.lowNote), p, 1);
    p = my_get_CHAR(&(aif->inst.highNote), p, 1);
    p = my_get_CHAR(&(aif->inst.lowVelocity), p, 1);
    p = my_get_CHAR(&(aif->inst.highVelocity), p, 1);
    p = my_get_SHORT_BE(&(aif->inst.gain), p);
    p = aif_loop_read(aif, &(aif->inst.sustainLoop), "inst.sustainLoop", p);
    p = aif_loop_read(aif, &(aif->inst.releaseLoop), "inst.releaseLoop", p);
  }

  n = aif->tree->ckID_find(aif->tree, "COMT", NULL, 1);
  if (n != -1) {
    chunk_tmp = aif->tree->items[n].chunk;
    chunk_tmp->data_read(chunk_tmp, ifp);
    p = chunk_tmp->data;
    p = my_get_WORD_BE(&(aif->comt.numComments), p);
    aif->comt.comments = malloc(aif->comt.numComments * sizeof(cmnt_t *));
    for (i = 0; i < aif->comt.numComments; i++)
      p = aif_comment_allocate_and_read(aif, i, p);
  }

  n = aif->tree->ckID_find(aif->tree, "NAME", NULL, 1);
  if (n != -1) {
    chunk_tmp = aif->tree->items[n].chunk;
    chunk_tmp->data_read(chunk_tmp, ifp);
    i = chunk_tmp->ckSize + 1;
    aif->name = malloc(i);
    memset(aif->name, 0, i);
    memcpy(aif->name, chunk_tmp->data, chunk_tmp->ckSize);
  }

  n = aif->tree->ckID_find(aif->tree, "AUTH", NULL, 1);
  if (n != -1) {
    chunk_tmp = aif->tree->items[n].chunk;
    chunk_tmp->data_read(chunk_tmp, ifp);
    i = chunk_tmp->ckSize + 1;
    aif->auth = malloc(i);
    memset(aif->auth, 0, i);
    memcpy(aif->auth, chunk_tmp->data, chunk_tmp->ckSize);
  }

  n = aif->tree->ckID_find(aif->tree, "(c) ", NULL, 1);
  if (n != -1) {
    chunk_tmp = aif->tree->items[n].chunk;
    chunk_tmp->data_read(chunk_tmp, ifp);
    i = chunk_tmp->ckSize + 1;
    aif->copy = malloc(i);
    memset(aif->copy, 0, i);
    memcpy(aif->copy, chunk_tmp->data, chunk_tmp->ckSize);
  }

  n = aif->tree->ckID_find(aif->tree, "ANNO", NULL, 1);
  if (n != -1) {
    chunk_tmp = aif->tree->items[n].chunk;
    chunk_tmp->data_read(chunk_tmp, ifp);
    i = chunk_tmp->ckSize + 1;
    aif->anno = malloc(i);
    memset(aif->anno, 0, i);
    memcpy(aif->anno, chunk_tmp->data, chunk_tmp->ckSize);
  }

  n = aif->tree->ckID_find(aif->tree, "SSND", NULL, 1);
  if (n == -1) {
    trap_warn("header_read: no SSND chunk");
    return 0;
  }
  chunk_tmp = aif->tree->items[n].chunk;
  chunk_tmp->data_read_bytes(chunk_tmp, ifp, buffer, 2 * sizeof(myDWORD));
  aif->ssnd.fpos_WaveformData = ftell(ifp);
  p = buffer;
  p = my_get_DWORD_BE(&(aif->ssnd.offset), p);
  p = my_get_DWORD_BE(&(aif->ssnd.blockSize), p);

  aif->sample_calc(aif);
  return 1;
}

void aif_header_write(void *vaif, FILE *ofp)
{
  aif_t *aif = vaif;
  void *p;
  chunk_t *chunk_tmp;
  chunk_t *chunk_dst;
  int i;
  int r;

  chunk_dst = chunk_new(NULL, IN_RAM);
  chunk_dst->endian = MY_BIG_ENDIAN;
  strncpy(chunk_dst->ckID.str, "FORM", 4);
  chunk_dst->header_write(chunk_dst, ofp);
  chunk_dst->data_write(chunk_dst, "AIFC", 4, ofp);

  chunk_tmp = chunk_new(NULL, IN_RAM);
  chunk_tmp->endian = MY_BIG_ENDIAN;
  strncpy(chunk_tmp->ckID.str, "FVER", 4);
  chunk_tmp->ckSize = sizeof(myDWORD);
  chunk_tmp->data = realloc(chunk_tmp->data, chunk_tmp->ckSize);
  p = chunk_tmp->data;
  p = my_put_DWORD_BE(&(aif->fver.timestamp), p);
  chunk_dst->ckSize += chunk_tmp->write(chunk_tmp, ofp);
  chunk_tmp->destroy(chunk_tmp);

  chunk_tmp = chunk_new(NULL, IN_RAM);
  chunk_tmp->endian = MY_BIG_ENDIAN;
  strncpy(chunk_tmp->ckID.str, "COMM", 4);
  chunk_tmp->ckSize = sizeof(myWORD) * 2 + sizeof(myDWORD) +
    sizeof(myIEEE80) + sizeof(myFOURCC) +
    my_size_PSTRING(&(aif->comm.compressionName));
  chunk_tmp->data = realloc(chunk_tmp->data, chunk_tmp->ckSize);
  p = chunk_tmp->data;
  p = my_put_WORD_BE(&(aif->comm.numChannels), p);
  p = my_put_DWORD_BE(&(aif->comm.numSampleFrames), p);
  p = my_put_WORD_BE(&(aif->comm.sampleSize), p);
  p = my_put_IEEE80(&(aif->comm.sampleRate), p);
  p = my_put_CHAR(aif->comm.compressionType.str, p, sizeof(myFOURCC));
  p = my_put_PSTRING(&(aif->comm.compressionName), p);
  chunk_dst->ckSize += chunk_tmp->write(chunk_tmp, ofp);
  chunk_tmp->destroy(chunk_tmp);

  if (aif->mark.numMarkers > 0) {
    chunk_tmp = chunk_new(NULL, IN_RAM);
    chunk_tmp->endian = MY_BIG_ENDIAN;
    strncpy(chunk_tmp->ckID.str, "MARK", 4);
    chunk_tmp->ckSize = sizeof(myWORD);
    for (i = 0; i < aif->mark.numMarkers; i++)
      chunk_tmp->ckSize += sizeof(myWORD) + sizeof(myDWORD) +
        my_size_PSTRING(&(aif->mark.markers[i]->markerName));
    p = chunk_tmp->data = malloc(chunk_tmp->ckSize);
    p = my_put_WORD_BE(&(aif->mark.numMarkers), p);
    for (i = 0; i < aif->mark.numMarkers; i++) {
      p = my_put_WORD_BE(&(aif->mark.markers[i]->id), p);
      p = my_put_DWORD_BE(&(aif->mark.markers[i]->position), p);
      p = my_put_PSTRING(&(aif->mark.markers[i]->markerName), p);
    }
    chunk_dst->ckSize += chunk_tmp->write(chunk_tmp, ofp);
    chunk_tmp->destroy(chunk_tmp);
  }

  if (aif->inst.baseNote > -1) {
    chunk_tmp = chunk_new(NULL, IN_RAM);
    chunk_tmp->endian = MY_BIG_ENDIAN;
    strncpy(chunk_tmp->ckID.str, "INST", 4);
    chunk_tmp->ckSize = 6 * sizeof(myCHAR) + 6 * sizeof(myWORD) +
      sizeof(mySHORT);
    p = chunk_tmp->data = malloc(chunk_tmp->ckSize);
    p = my_put_CHAR(&(aif->inst.baseNote), p, 1);
    p = my_put_CHAR(&(aif->inst.detune), p, 1);
    p = my_put_CHAR(&(aif->inst.lowNote), p, 1);
    p = my_put_CHAR(&(aif->inst.highNote), p, 1);
    p = my_put_CHAR(&(aif->inst.lowVelocity), p, 1);
    p = my_put_CHAR(&(aif->inst.highVelocity), p, 1);
    p = my_put_SHORT_BE(&(aif->inst.gain), p);
    p = my_put_WORD_BE(&(aif->inst.sustainLoop.playMode), p);
    p = my_put_WORD_BE(&(aif->inst.sustainLoop.beginLoop), p);
    p = my_put_WORD_BE(&(aif->inst.sustainLoop.endLoop), p);
    p = my_put_WORD_BE(&(aif->inst.releaseLoop.playMode), p);
    p = my_put_WORD_BE(&(aif->inst.releaseLoop.beginLoop), p);
    p = my_put_WORD_BE(&(aif->inst.releaseLoop.endLoop), p);
    chunk_dst->ckSize += chunk_tmp->write(chunk_tmp, ofp);
    chunk_tmp->destroy(chunk_tmp);
  }

  if (aif->comt.numComments > 0) {
    chunk_tmp = chunk_new(NULL, IN_RAM);
    chunk_tmp->endian = MY_BIG_ENDIAN;
    strncpy(chunk_tmp->ckID.str, "COMT", 4);
    chunk_tmp->ckSize = sizeof(myWORD);
    for (i = 0; i < aif->comt.numComments; i++) {
      r = aif->comt.comments[i]->count;
      chunk_tmp->ckSize += sizeof(myDWORD) + 2 * sizeof(myWORD) + r;
      if (!is_even(r))
        chunk_tmp->ckSize++;
    }
    p = chunk_tmp->data = malloc(chunk_tmp->ckSize);
    p = my_put_WORD_BE(&(aif->comt.numComments), p);
    for (i = 0; i < aif->comt.numComments; i++) {
      p = my_put_DWORD_BE(&(aif->comt.comments[i]->timeStamp), p);
      p = my_put_WORD_BE(&(aif->comt.comments[i]->marker), p);
      p = my_put_WORD_BE(&(aif->comt.comments[i]->count), p);
      p = my_put_CHAR(aif->comt.comments[i]->text, p, r);
      if (!is_even(r))
        p = my_put_CHAR("\000", p, 1);
    }
    chunk_dst->ckSize += chunk_tmp->write(chunk_tmp, ofp);
    chunk_tmp->destroy(chunk_tmp);
  }

  if (aif->name != NULL) {
    chunk_tmp = chunk_new(NULL, IN_RAM);
    chunk_tmp->endian = MY_BIG_ENDIAN;
    strncpy(chunk_tmp->ckID.str, "NAME", 4);
    r = strlen(aif->name);
    chunk_tmp->ckSize = r;
    if (!is_even(r))
      chunk_tmp->ckSize++;
    p = chunk_tmp->data = malloc(chunk_tmp->ckSize);
    p = my_put_CHAR(aif->name, p, r);
    if (!is_even(r))
      p = my_put_CHAR("\000", p, 1);
    chunk_dst->ckSize += chunk_tmp->write(chunk_tmp, ofp);
    chunk_tmp->destroy(chunk_tmp);
  }

  if (aif->auth != NULL) {
    chunk_tmp = chunk_new(NULL, IN_RAM);
    chunk_tmp->endian = MY_BIG_ENDIAN;
    strncpy(chunk_tmp->ckID.str, "AUTH", 4);
    r = strlen(aif->auth);
    chunk_tmp->ckSize = r;
    if (!is_even(r))
      chunk_tmp->ckSize++;
    p = chunk_tmp->data = malloc(chunk_tmp->ckSize);
    p = my_put_CHAR(aif->auth, p, r);
    if (!is_even(r))
      p = my_put_CHAR("\000", p, 1);
    chunk_dst->ckSize += chunk_tmp->write(chunk_tmp, ofp);
    chunk_tmp->destroy(chunk_tmp);
  }

  if (aif->copy != NULL) {
    chunk_tmp = chunk_new(NULL, IN_RAM);
    chunk_tmp->endian = MY_BIG_ENDIAN;
    strncpy(chunk_tmp->ckID.str, "(c) ", 4);
    r = strlen(aif->copy);
    chunk_tmp->ckSize = r;
    if (!is_even(r))
      chunk_tmp->ckSize++;
    p = chunk_tmp->data = malloc(chunk_tmp->ckSize);
    p = my_put_CHAR(aif->copy, p, r);
    if (!is_even(r))
      p = my_put_CHAR("\000", p, 1);
    chunk_dst->ckSize += chunk_tmp->write(chunk_tmp, ofp);
    chunk_tmp->destroy(chunk_tmp);
  }

  if (aif->anno != NULL) {
    chunk_tmp = chunk_new(NULL, IN_RAM);
    chunk_tmp->endian = MY_BIG_ENDIAN;
    strncpy(chunk_tmp->ckID.str, "ANNO", 4);
    r = strlen(aif->anno);
    chunk_tmp->ckSize = r;
    if (!is_even(r))
      chunk_tmp->ckSize++;
    p = chunk_tmp->data = malloc(chunk_tmp->ckSize);
    p = my_put_CHAR(aif->anno, p, r);
    if (!is_even(r))
      p = my_put_CHAR("\000", p, 1);
    chunk_dst->ckSize += chunk_tmp->write(chunk_tmp, ofp);
    chunk_tmp->destroy(chunk_tmp);
  }

  chunk_tmp = chunk_new(NULL, IN_RAM);
  chunk_tmp->endian = MY_BIG_ENDIAN;
  strncpy(chunk_tmp->ckID.str, "SSND", 4);
  chunk_tmp->header_write(chunk_tmp, ofp);
  chunk_tmp->data = realloc(chunk_tmp->data, sizeof(myDWORD) * 2);
  p = chunk_tmp->data;
  p = my_put_DWORD_BE(&(aif->ssnd.offset), p);
  p = my_put_DWORD_BE(&(aif->ssnd.blockSize), p);
  chunk_tmp->data_write(chunk_tmp, chunk_tmp->data, sizeof(myDWORD) * 2, ofp);
  aif->ssnd.fpos_WaveformData = ftell(ofp);
  chunk_tmp->ckSize += aif->ssnd.bytes_WaveformData;
  chunk_tmp->start_seek(chunk_tmp, ofp);
  chunk_tmp->header_write(chunk_tmp, ofp);
  chunk_dst->ckSize += CHUNK_HEADER_SIZE + chunk_tmp->ckSize;

  chunk_dst->start_seek(chunk_dst, ofp);
  chunk_dst->header_write(chunk_dst, ofp);
  chunk_tmp->destroy(chunk_tmp);
  chunk_dst->destroy(chunk_dst);
}

void aif_sample_calc(void *vaif)
{
  aif_t *aif = vaif;

  aif->comm.double_sampleRate = ieee_80_to_double(aif->comm.sampleRate);
  if (aif->comm.double_sampleRate != ceil(aif->comm.double_sampleRate)) {
    trap_warn("aif: rounding fractional sample rate");
    aif->comm.double_sampleRate = ceil(aif->comm.double_sampleRate);
    double_to_ieee_80(aif->comm.double_sampleRate, aif->comm.sampleRate);
  }
  aif->comm.samplePointSize = aif->comm.sampleSize / BITS_PER_BYTE;
  if (aif->comm.sampleSize % BITS_PER_BYTE)
    aif->comm.samplePointSize++;
  aif->ssnd.bytes_WaveformData = aif->comm.numChannels *
    aif->comm.numSampleFrames * aif->comm.samplePointSize;
}

void aif_header_set(void *vaif, myWORD numChannels,
  myDWORD numSampleFrames, myWORD sampleSize, myIEEE80 sampleRate,
  myDWORD timestamp, char *compressionType, char *compressionName)
{
  aif_t *aif = vaif;
  aif->fver.timestamp = timestamp;
  aif->comm.numChannels = numChannels;
  aif->comm.numSampleFrames = numSampleFrames;
  aif->comm.sampleSize = sampleSize;
  memcpy(aif->comm.sampleRate, sampleRate, sizeof(sampleRate));
  memcpy(aif->comm.compressionType.str, compressionType, sizeof(myFOURCC));
  memset(aif->comm.compressionName.str, 0,
    sizeof(aif->comm.compressionName.str));
  snprintf(aif->comm.compressionName.str,
    sizeof(aif->comm.compressionName.str), "%s", compressionName);
  aif->comm.compressionName.count = strlen(aif->comm.compressionName.str);
  aif->ssnd.offset = 0;
  aif->ssnd.blockSize = 0;
  aif->ssnd.fpos_WaveformData = 0;
  aif->sample_calc(aif);
}

void aif_destroy(void *vaif)
{
  aif_t *aif = vaif;
  int i;
  aif->tree->destroy(aif->tree);
  if (aif->mark.markers != NULL) {
    for (i = 0; i < aif->mark.numMarkers; i++) {
      if (aif->mark.markers[i] != NULL)
        free(aif->mark.markers[i]);
    }
    free(aif->mark.markers);
  }
  if (aif->comt.comments != NULL) {
    for (i = 0; i < aif->comt.numComments; i++) {
      if (aif->comt.comments[i] != NULL) {
        if (aif->comt.comments[i]->text != NULL)
          free(aif->comt.comments[i]->text);
        free(aif->comt.comments[i]);
      }
    }
    free(aif->comt.comments);
  }
  if (aif->name != NULL)
    free(aif->name);
  if (aif->auth != NULL)
    free(aif->auth);
  if (aif->copy != NULL)
    free(aif->copy);
  if (aif->anno != NULL)
    free(aif->anno);
  free(aif);
}

aif_t *aif_new(void)
{
  aif_t *retval;
  retval = malloc(sizeof(aif_t));

  retval->name = NULL;
  retval->auth = NULL;
  retval->copy = NULL;
  retval->anno = NULL;
  retval->tree = tree_new(aif_items, aif_containers, retval);
  retval->destroy = aif_destroy;
  retval->header_read = aif_header_read;
  retval->header_write = aif_header_write;
  retval->header_set = aif_header_set;
  retval->sample_calc = aif_sample_calc;
  retval->marker_find = aif_marker_find;
  aif_header_set(retval, 0, 0, 0, (unsigned char *)ieee80_44100,
    AIFC_VERSION1, COMPRESSION_TYPE_NONE, COMPRESSION_NAME_NOT);
  retval->mark.numMarkers = 0;
  retval->mark.markers = NULL;
  retval->inst.baseNote = -1;
  retval->comt.numComments = 0;
  retval->comt.comments = NULL;
  return retval;
}
