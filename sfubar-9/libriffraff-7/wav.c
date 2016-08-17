#include <stdlib.h>
#include <string.h>

#include <trap.h>
#include "bits.h"
#include "riff.h"
#include "rifftypes.h"
#include "wav.h"

/* only supports writing the wave header */

tree_item wav_items[] = {
{ 0, "RIFF", "WAVE", NULL, NULL },
{ 1, "fmt ", NULL, NULL, NULL },
{ 1, "data", NULL, NULL, NULL },
{ 1, "cue ", NULL, NULL, NULL },
{ 1, "plst", NULL, NULL, NULL }, 
{ 1, "LIST", "adtl", NULL, NULL },
{ 1, "smpl", NULL, NULL, NULL },
{ 1, "inst", NULL, NULL, NULL },
{ 2, "labl", NULL, NULL, NULL }, 
{ 2, "note", NULL, NULL, NULL },
{ 2, "ltxt", NULL, NULL, NULL },
{ -1, "", NULL, NULL, NULL }
};

tree_container wav_containers[] = { "RIFF", "LIST", "" };

int wav_header_read(void *vwav, FILE *ifp)
{
  void *p;
  wav_t *wav = vwav;
  int n;
  chunk_t *chunk_src;
  chunk_t *chunk_tmp;

  chunk_src = chunk_new(NULL, ON_DISK);
  chunk_src->header_read(chunk_src, ifp);
  wav->tree->read(wav->tree, chunk_src, 0, ifp);
  /* chunk_src is freed when wav->tree is destroyed */

  n = wav->tree->ckID_find(wav->tree, "RIFF", "WAVE", 0);
  if (n == -1) {
    trap_warn("header_read: no RIFF chunk with WAVE form");
    return 0;
  }

  n = wav->tree->ckID_find(wav->tree, "fmt ", NULL, 1);
  if (n == -1) {
    trap_warn("header_read: no fmt chunk");
    return 0;
  }

  chunk_tmp = wav->tree->items[n].chunk;
  chunk_tmp->data_read(chunk_tmp, ifp);
  p = chunk_tmp->data;
  p = my_get_WORD_LE(&(wav->fmt.wFormatTag), p);
  p = my_get_WORD_LE(&(wav->fmt.wChannels), p);
  p = my_get_DWORD_LE(&(wav->fmt.dwSamplesPerSec), p);
  p = my_get_DWORD_LE(&(wav->fmt.dwAvgBytesPerSec), p);
  p = my_get_WORD_LE(&(wav->fmt.wBlockAlign), p);
  if (wav->fmt.wFormatTag == WAVE_FORMAT_PCM)
    p = my_get_WORD_LE(&(wav->fmt.wBitsPerSample), p);

  n = wav->tree->ckID_find(wav->tree, "data", NULL, 1);
  if (n == -1) {
    trap_warn("header_read: no data chunk");
    return 0;
  }

  chunk_tmp = wav->tree->items[n].chunk;
  chunk_tmp->data_seek(chunk_tmp, ifp);
  wav->data.offset = ftell(ifp);
  wav->data.size = chunk_tmp->ckSize;
  return 1;
}

void wav_header_write(void *vwav, FILE *ofp)
{
  wav_t *wav = vwav;
  void *p;
  chunk_t *chunk_tmp;
  chunk_t *chunk_dst;

  chunk_dst = chunk_new(NULL, IN_RAM);
  strncpy(chunk_dst->ckID.str, "RIFF", 4);
  chunk_dst->header_write(chunk_dst, ofp);
  chunk_dst->data_write(chunk_dst, "WAVE", 4, ofp);

  chunk_tmp = chunk_new(NULL, IN_RAM);
  strncpy(chunk_tmp->ckID.str, "fmt ", 4);
  chunk_tmp->ckSize = 16;
  chunk_tmp->data = realloc(chunk_tmp->data, chunk_tmp->ckSize);
  p = chunk_tmp->data;
  p = my_put_WORD_LE(&(wav->fmt.wFormatTag), p);
  p = my_put_WORD_LE(&(wav->fmt.wChannels), p);
  p = my_put_DWORD_LE(&(wav->fmt.dwSamplesPerSec), p);
  p = my_put_DWORD_LE(&(wav->fmt.dwAvgBytesPerSec), p);
  p = my_put_WORD_LE(&(wav->fmt.wBlockAlign), p);
  p = my_put_WORD_LE(&(wav->fmt.wBitsPerSample), p);
  chunk_dst->ckSize += chunk_tmp->write(chunk_tmp, ofp);
  chunk_tmp->destroy(chunk_tmp);

  chunk_tmp = chunk_new(NULL, IN_RAM);
  strncpy(chunk_tmp->ckID.str, "data", 4);
  chunk_tmp->ckSize = wav->data.size;
  chunk_tmp->header_write(chunk_tmp, ofp);
  chunk_dst->ckSize += CHUNK_HEADER_SIZE + chunk_tmp->ckSize;
  chunk_dst->start_seek(chunk_dst, ofp);
  chunk_dst->header_write(chunk_dst, ofp);
  chunk_tmp->data_seek(chunk_tmp, ofp);
  chunk_tmp->destroy(chunk_tmp);

  chunk_dst->destroy(chunk_dst);
}

void wav_header_set(void *vwav, myWORD channels,
  myWORD bytes_per_sample, myDWORD sample_rate, long sample_count)
{
  wav_t *wav = vwav;
  wav->fmt.wBitsPerSample = bytes_per_sample * BITS_PER_BYTE;
  wav->fmt.wFormatTag = WAVE_FORMAT_PCM;
  wav->fmt.wChannels = channels;
  wav->fmt.dwSamplesPerSec = sample_rate;
  wav->fmt.dwAvgBytesPerSec = sample_rate * bytes_per_sample;
  wav->fmt.wBlockAlign = channels * bytes_per_sample;
  wav->data.offset = 0;
  wav->data.size = sample_count * bytes_per_sample;
}

void wav_destroy(void *vwav)
{
  wav_t *wav = vwav;
  wav->tree->destroy(wav->tree);
  free(wav);
}

wav_t *wav_new(void)
{
  wav_t *retval;
  retval = malloc(sizeof(wav_t));

  wav_header_set(retval, 0, 0, 0, 0);
  retval->tree = tree_new(wav_items, wav_containers, retval);
  retval->destroy = wav_destroy;
  retval->header_read = wav_header_read;
  retval->header_write = wav_header_write;
  retval->header_set = wav_header_set;
  return retval;
}
