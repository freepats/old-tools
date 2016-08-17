#include <sys/stat.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <trap.h>
#include <parsetxt.h>
#include <riffraff/bio.h>
#include <riffraff/bits.h>
#include <riffraff/riff.h>
#include <riffraff/rifftypes.h>
#include <riffraff/wav.h>
#include "wavutil.h"

#ifndef BUFLEN
#define BUFLEN 256
#endif

void wp_meta(void *vtree, chunk_t *chunk, char *prefix)
{ 
  tree_t *tree = vtree;
  fprintf(tree->ofp, "%s ~ BEGIN\n", prefix);
}

void wp_ignore(void *vtree, chunk_t *chunk, char *prefix)
{ 
  /* ignore this item, print nothing */
}

void wp_fmt(void *vtree, chunk_t *chunk, char *prefix)
{
  tree_t *tree = vtree;
  wav_t *wav = tree->parent;
  fprintf(tree->ofp, "%s.wFormatTag=%u\n", prefix, wav->fmt.wFormatTag);
  fprintf(tree->ofp, "%s.wChannels=%u\n", prefix, wav->fmt.wChannels);
  fprintf(tree->ofp, "%s.dwSamplesPerSec=%u\n", prefix,
    wav->fmt.dwSamplesPerSec);
  fprintf(tree->ofp, "%s.dwAvgBytesPerSec=%u\n", prefix,
    wav->fmt.dwAvgBytesPerSec);
  fprintf(tree->ofp, "%s.wBlockAlign=%u\n", prefix,
    wav->fmt.wBlockAlign);
  fprintf(tree->ofp, "%s.wBitsPerSample=%u\n", prefix,
    wav->fmt.wBitsPerSample);
}

void wp_data(void *vtree, chunk_t *chunk, char *prefix)
{
  tree_t *tree = vtree;
  wav_t *wav = tree->parent;
  char filename[BUFLEN];
  FILE *sfp;
  
  snprintf(filename, BUFLEN, "%s.raw", tree->wt_prefix);
  fprintf(tree->ofp, "%s.size=%ld\n", prefix, wav->data.size);
  fprintf(tree->ofp, "%s.file=%s\n", prefix, filename);
  sfp = bens_fopen(filename, "w");
  chunk->data_seek(chunk, tree->ifp);
  data_copy(tree->ifp, sfp, wav->data.size, 0);
  bens_fclose(sfp);
}

void wav_to_txt(char *infile, char *outfile, int format)
{
  FILE *ifp;
  FILE *ofp;
  wav_t *wav = wav_new();
  char wt_prefix[BUFLEN];
  char *p;
  int n;
  int riff_item;

  strncpy(wt_prefix, outfile, BUFLEN - 1);
  wt_prefix[BUFLEN - 1] = '\000';
  p = strrchr(wt_prefix, '.');
  if (p != NULL)
    *p = '\000';

  ifp = bens_fopen(infile, "r");
  ofp = bens_fopen(outfile, "w");
  wav->header_read(wav, ifp);
  riff_item = n = wav->tree->ckID_find(wav->tree, "RIFF", "WAVE", -1);
  wav->tree->items[n].print = (format == 0) ? wp_ignore : wp_meta;
  n = wav->tree->ckID_find(wav->tree, "fmt ", NULL, -1);
  wav->tree->items[n].print = wp_fmt;
  n = wav->tree->ckID_find(wav->tree, "data", NULL, -1);
  wav->tree->items[n].print = (format == 0) ? wp_data : wp_ignore;
  wav->tree->print(wav->tree, riff_item, "", wt_prefix, ifp, ofp);

  wav->destroy(wav);
  bens_fclose(ifp);
  bens_fclose(ofp);
}

void txt_to_wav(char *infile, char *outfile)
{
  FILE *ifp;
  FILE *ofp;
  FILE *sfp;
  txt_t *txt;
  wav_t *wav;
  char *str;
  myWORD wFormatTag = -1;
  myWORD wChannels = -1;
  myDWORD dwSamplesPerSec = -1;
  myWORD wBitsPerSample = -1;
  long bytes_per_sample;
  long data_size = -1;
  long sample_count;
  int byteswap_do = 0;

  ifp = bens_fopen(infile, "r");
  ofp = bens_fopen(outfile, "w");
  txt = txt_new();
  txt->read_lines(txt, ifp);

  str = txt->get_data(txt, "RIFF.fmt.wFormatTag");
  if (str != NULL) {
    wFormatTag = atoi(str);
  }
  if (wFormatTag != WAVE_FORMAT_PCM)
    trap_exit("unsupported wFormatTag");

  str = txt->get_data(txt, "RIFF.fmt.wChannels");
  if (str != NULL) {
    wChannels = atoi(str);
  }
  if (wChannels < 1)
    trap_exit("unsupported wChannels");

  str = txt->get_data(txt, "RIFF.fmt.dwSamplesPerSec");
  if (str != NULL) {
    dwSamplesPerSec = atoi(str);
  }
  if (dwSamplesPerSec < 1)
    trap_exit("unsupported dwSamplesPerSec");

  str = txt->get_data(txt, "RIFF.fmt.dwAvgBytesPerSec");
  str = txt->get_data(txt, "RIFF.fmt.wBlockAlign");
  str = txt->get_data(txt, "RIFF.fmt.wBitsPerSample");
  if (str != NULL) {
    wBitsPerSample = atoi(str);
  }
  if (wBitsPerSample < 1 || wBitsPerSample % 8 != 0)
    trap_exit("unsupported wBitsPerSample");
  bytes_per_sample = wBitsPerSample / 8;

  str = txt->get_data(txt, "RIFF.data.size");
  if (str != NULL) {
    data_size = atol(str);
    sample_count = data_size / bytes_per_sample;
  }
  if (data_size % bytes_per_sample != 0)
    trap_exit("unsupported data.size");

  str = txt->get_data(txt, "RIFF.data.byteswap");
  if (str != NULL)
    byteswap_do = atoi(str);

  wav = wav_new();
  wav->header_set(wav, wChannels, bytes_per_sample, dwSamplesPerSec,
    sample_count);
  wav->header_write(wav, ofp);
  wav->destroy(wav);

  str = txt->get_data(txt, "RIFF.data.file");
  if (str == NULL)
    trap_exit("missing data.file");
  sfp = bens_fopen(str, "r");
  data_copy(sfp, ofp, sample_count * bytes_per_sample, byteswap_do);
  bens_fclose(sfp);

  bens_fclose(ifp);
  bens_fclose(ofp);
  txt->warn_unused(txt);
  txt->destroy(txt);
}

/* move to wav.c
void wp_smpl(void *vtree, chunk_t *chunk, char *prefix)
{
  void *data;
  void *p;
  char filename[] = "sampler.dat";
  FILE *sfp;

  myDWORD dwManufacturer;
  myDWORD dwProduct;
  myDWORD dwSamplePeriod;
  myDWORD dwMIDIUnityNote;
  myDWORD dwMIDIPitchFraction;
  myDWORD dwSMPTEFormat;
  myDWORD dwSMPTEOffset;
  myDWORD cSampleLoops;
  myDWORD cbSamplerData;

  int i;
  myDWORD dwIdentifier;
  myDWORD dwType;
  myDWORD dwStart;
  myDWORD dwEnd;
  myDWORD dwFraction;
  myDWORD dwPlayCount;

  p = data = chunk->data_read(chunk, ifp);
  p = my_get_DWORD_LE(&dwManufacturer, p);
  p = my_get_DWORD_LE(&dwProduct, p);
  p = my_get_DWORD_LE(&dwSamplePeriod, p);
  p = my_get_DWORD_LE(&dwMIDIUnityNote, p);
  p = my_get_DWORD_LE(&dwMIDIPitchFraction, p);
  p = my_get_DWORD_LE(&dwSMPTEFormat, p);
  p = my_get_DWORD_LE(&dwSMPTEOffset, p);
  p = my_get_DWORD_LE(&cSampleLoops, p);
  p = my_get_DWORD_LE(&cbSamplerData, p);
  fprintf(ofp, "%s.dwManufacturer=%u\n", prefix, dwManufacturer);
  fprintf(ofp, "%s.dwProduct=%u\n", prefix, dwProduct);
  fprintf(ofp, "%s.dwSamplePeriod=%u\n", prefix, dwSamplePeriod);
  fprintf(ofp, "%s.dwMIDIUnityNote=%u\n", prefix, dwMIDIUnityNote);
  fprintf(ofp, "%s.dwMIDIPitchFraction=%u\n", prefix, dwMIDIPitchFraction);
  fprintf(ofp, "%s.dwSMPTEFormat=%u\n", prefix, dwSMPTEFormat);
  fprintf(ofp, "%s.dwSMPTEOffset=%u\n", prefix, dwSMPTEOffset);
  fprintf(ofp, "%s.cSampleLoops=%u\n", prefix, cSampleLoops);
  fprintf(ofp, "%s.cbSamplerData=%u\n", prefix, cbSamplerData);
  if (cbSamplerData > 0) {
    fprintf(ofp, "%s.SamplerDataFile=%s\n", prefix, filename);
  }

  for (i = 0; i < cSampleLoops; i++) {
    p = my_get_DWORD_LE(&dwIdentifier, p);
    p = my_get_DWORD_LE(&dwType, p);
    p = my_get_DWORD_LE(&dwStart, p);
    p = my_get_DWORD_LE(&dwEnd, p);
    p = my_get_DWORD_LE(&dwFraction, p);
    p = my_get_DWORD_LE(&dwPlayCount, p);
    fprintf(ofp, "%s.loop.%d.dwIdentifier=%u\n", prefix, i, dwIdentifier);
    fprintf(ofp, "%s.loop.%d.dwType=%u\n", prefix, i, dwType);
    fprintf(ofp, "%s.loop.%d.dwStart=%u\n", prefix, i, dwStart);
    fprintf(ofp, "%s.loop.%d.dwEnd=%u\n", prefix, i, dwEnd);
    fprintf(ofp, "%s.loop.%d.dwFraction=%u\n", prefix, i, dwFraction);
    fprintf(ofp, "%s.loop.%d.dwPlayCount=%u\n", prefix, i, dwPlayCount);
  }

  if (cbSamplerData > 0) {
    sfp = bens_fopen(filename, "w");
    bens_fwrite(p, cbSamplerData, 1, sfp);
    bens_fclose(sfp);
  }
}
*/

