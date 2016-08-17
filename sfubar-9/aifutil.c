#include <sys/stat.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <math.h>
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
#include <riffraff/ieee80.h>
#include <riffraff/aif.h>
#include "aifutil.h"

#ifndef BUFLEN
#define BUFLEN 256
#endif

void ap_meta(void *vtree, chunk_t *chunk, char *prefix)
{ 
  tree_t *tree = vtree;
  fprintf(tree->ofp, "%s ~ BEGIN\n", prefix);
}

void ap_ignore(void *vtree, chunk_t *chunk, char *prefix)
{ 
  /* ignore this item, print nothing */
}

void ap_fver(void *vtree, chunk_t *chunk, char *prefix)
{
  tree_t *tree = vtree;
  aif_t *aif = tree->parent;

  fprintf(tree->ofp, "%s.timestamp=%u\n", prefix, aif->fver.timestamp);
}

void ap_comm(void *vtree, chunk_t *chunk, char *prefix)
{
  tree_t *tree = vtree;
  aif_t *aif = tree->parent;
  char *p;

  fprintf(tree->ofp, "%s.numChannels=%u\n", prefix, aif->comm.numChannels);
  fprintf(tree->ofp, "%s.sampleSize=%u\n", prefix, aif->comm.sampleSize);
  fprintf(tree->ofp, "%s.sampleRate=%u\n", prefix,
    (int)(aif->comm.double_sampleRate));
  p = aif->comm.compressionType.str;
  fprintf(tree->ofp, "%s.compressionType.str=%c%c%c%c\n", prefix,
    p[0], p[1], p[2], p[3]);
  fprintf(tree->ofp, "%s.compressionName.str=%s\n", prefix,
    aif->comm.compressionName.str);
}

void ap_mark(void *vtree, chunk_t *chunk, char *prefix)
{
  tree_t *tree = vtree;
  aif_t *aif = tree->parent;
  int i;

  for (i = 0; i < aif->mark.numMarkers; i++) {
    fprintf(tree->ofp, "%s.%d.id=%u\n", prefix, i + 1,
      aif->mark.markers[i]->id);
    fprintf(tree->ofp, "%s.%d.pos=%u\n", prefix, i + 1,
      aif->mark.markers[i]->position);
    fprintf(tree->ofp, "%s.%d.name=%s\n", prefix, i + 1,
      aif->mark.markers[i]->markerName.str);
  }
}

void ap_inst(void *vtree, chunk_t *chunk, char *prefix)
{
  tree_t *tree = vtree;
  aif_t *aif = tree->parent;

  fprintf(tree->ofp, "%s.baseNote=%u\n", prefix, aif->inst.baseNote);
  fprintf(tree->ofp, "%s.detune=%u\n", prefix, aif->inst.detune);
  fprintf(tree->ofp, "%s.lowNote=%u\n", prefix, aif->inst.lowNote);
  fprintf(tree->ofp, "%s.highNote=%u\n", prefix, aif->inst.highNote);
  fprintf(tree->ofp, "%s.lowVelocity=%u\n", prefix, aif->inst.lowVelocity);
  fprintf(tree->ofp, "%s.highVelocity=%u\n", prefix, aif->inst.highVelocity);
  fprintf(tree->ofp, "%s.gain=%u\n", prefix, aif->inst.gain);
  fprintf(tree->ofp, "%s.sustainLoop.playMode=%u\n", prefix,
    aif->inst.sustainLoop.playMode);
  fprintf(tree->ofp, "%s.sustainLoop.beginLoop=%u\n", prefix,
    aif->inst.sustainLoop.beginLoop);
  fprintf(tree->ofp, "%s.sustainLoop.endLoop=%u\n", prefix,
    aif->inst.sustainLoop.endLoop);
  fprintf(tree->ofp, "%s.releaseLoop.playMode=%u\n", prefix,
    aif->inst.releaseLoop.playMode);
  fprintf(tree->ofp, "%s.releaseLoop.beginLoop=%u\n", prefix,
    aif->inst.releaseLoop.beginLoop);
  fprintf(tree->ofp, "%s.releaseLoop.endLoop=%u\n", prefix,
    aif->inst.releaseLoop.endLoop);
}

void ap_comt(void *vtree, chunk_t *chunk, char *prefix)
{
  tree_t *tree = vtree;
  aif_t *aif = tree->parent;
  int i;
  char filename[BUFLEN];
  FILE *fp;

  for (i = 0; i < aif->comt.numComments; i++) {
    fprintf(tree->ofp, "%s.%d.timeStamp=%u\n", prefix, i + 1,
      aif->comt.comments[i]->timeStamp);
    fprintf(tree->ofp, "%s.%d.marker=%u\n", prefix, i + 1,
      aif->comt.comments[i]->marker);
    snprintf(filename, BUFLEN, "comment%d.txt", i + 1);
    fprintf(tree->ofp, "%s.%d.file=%s\n", prefix, i + 1, filename);
    fp = bens_fopen(filename, "w");
    bens_fwrite(aif->comt.comments[i]->text,
      aif->comt.comments[i]->count, 1, fp);
    fclose(fp);
  }
}

void ap_iff_text(void *vtree, chunk_t *chunk, char *prefix)
{
  tree_t *tree = vtree;
  char *p = chunk->data;
  int i;
  char filename[BUFLEN];
  FILE *fp;

  i = chunk->ckSize;
  if (p[i] == '\000')
    i--; /* skip pad byte */
  snprintf(filename, BUFLEN, "%u.txt", chunk->ckID.dword);
  fprintf(tree->ofp, "%s.file=%s\n", prefix, filename);
  fp = bens_fopen(filename, "w");
  bens_fwrite(p, i, 1, fp);
  fclose(fp);
}

void ap_ssnd(void *vtree, chunk_t *chunk, char *prefix)
{
  tree_t *tree = vtree;
  aif_t *aif = tree->parent;
  char filename[BUFLEN];
  FILE *sfp;
  
  snprintf(filename, BUFLEN, "%s.raw", tree->wt_prefix);
  fprintf(tree->ofp, "%s.file=%s\n", prefix, filename);
  sfp = bens_fopen(filename, "w");
  fseek(tree->ifp, aif->ssnd.fpos_WaveformData, SEEK_SET);
  data_copy(tree->ifp, sfp, aif->ssnd.bytes_WaveformData, 0);
  bens_fclose(sfp);
}

void aif_to_txt(char *infile, char *outfile, int format)
{
  FILE *ifp;
  FILE *ofp;
  aif_t *aif = aif_new();
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
  aif->header_read(aif, ifp);
  riff_item = n = aif->tree->ckID_find(aif->tree, "FORM", "AIFF", -1);
  if (n == -1)
    riff_item = n = aif->tree->ckID_find(aif->tree, "FORM", "AIFC", -1);
  aif->tree->items[n].print = (format == 0) ? ap_ignore : ap_meta;
  n = aif->tree->ckID_find(aif->tree, "FVER", NULL, -1);
  aif->tree->items[n].print = ap_fver;
  n = aif->tree->ckID_find(aif->tree, "COMM", NULL, -1);
  aif->tree->items[n].print = ap_comm;
  n = aif->tree->ckID_find(aif->tree, "MARK", NULL, -1);
  aif->tree->items[n].print = ap_mark;
  n = aif->tree->ckID_find(aif->tree, "INST", NULL, -1);
  aif->tree->items[n].print = ap_inst;
  n = aif->tree->ckID_find(aif->tree, "COMT", NULL, -1);
  aif->tree->items[n].print = ap_comt;
  n = aif->tree->ckID_find(aif->tree, "NAME", NULL, -1);
  aif->tree->items[n].print = ap_iff_text;
  n = aif->tree->ckID_find(aif->tree, "AUTH", NULL, -1);
  aif->tree->items[n].print = ap_iff_text;
  n = aif->tree->ckID_find(aif->tree, "(c) ", NULL, -1);
  aif->tree->items[n].print = ap_iff_text;
  n = aif->tree->ckID_find(aif->tree, "ANNO", NULL, -1);
  aif->tree->items[n].print = ap_iff_text;
  n = aif->tree->ckID_find(aif->tree, "SSND", NULL, -1);
  aif->tree->items[n].print = (format == 0) ? ap_ssnd : ap_ignore;
  aif->tree->print(aif->tree, riff_item, "", wt_prefix, ifp, ofp);

  aif->destroy(aif);
  bens_fclose(ifp);
  bens_fclose(ofp);
}

int get_int(txt_t *txt, char *key, int min, int max, int defval)
{
  int retval = defval;
  char *str;

  str = txt->get_data(txt, key);
  if (str == NULL) {
    trap_warn("missing %s, defaulting to %d", key, defval);
  } else {
    retval = atoi(str);
  }
  if (retval < min || (max > min && retval > max))
    trap_exit("%s %d out of range %d..%d", key, retval, min, max);
  return retval;
}

time_t unix_to_apple_epoch(time_t s)
{
  time_t retval = s;

  #define UNIX_YEAR1 1970
  #define APPLE_YEAR1 1904
  #define DAYS_PER_YEAR 365.25
  #define SECONDS_PER_DAY 86400
  
  retval -= floor((UNIX_YEAR1 - APPLE_YEAR1) * DAYS_PER_YEAR * SECONDS_PER_DAY);
  return retval;
}

void file_read_simple(char *filename, int *count, char **str)
{
  FILE *fp;
  fp = bens_fopen(*str, "r");
  fseek(fp, 0, SEEK_END);
  *count = (int)ftell(fp);
  fseek(fp, 0, SEEK_SET);
  *str = malloc(*count + 1);
  memset(*str, 0, *count + 1);
  bens_fread(*str, *count, 1, fp);
  fclose(fp);
}

void loop_check(aif_t *aif, loop_t *loop, char *prefix)
{
  int r;
  if (loop->playMode == 0)
    return;
  if (loop->beginLoop == 0 && loop->endLoop == 0)
    return;
  if (loop->beginLoop == 0 || loop->endLoop == 0)
    trap_exit("%s, only one loop point defined", prefix);
  r = aif->marker_find(aif, loop->beginLoop);
  if (r == -1)
    trap_exit("%s.beginLoop invalid", prefix);
  r = aif->marker_find(aif, loop->endLoop);
  if (r == -1)
    trap_exit("%s.endLoop invalid", prefix);
}

void txt_to_aif(char *infile, char *outfile)
{
  FILE *ifp;
  FILE *ofp;
  FILE *sfp;
  txt_t *txt;
  aif_t *aif = aif_new();
  char *str;
  myDWORD timestamp;
  myWORD numChannels;
  myDWORD numSampleFrames;
  myWORD sampleSize;
  myIEEE80 sampleRate;
  int n;
  long ssnd_size;
  myWORD samplePointSize;
  char *compressionType;
  char *compressionName;
  char *endptr;
  char buffer[BUFLEN];
  int i;
  int r;
  int byteswap_do = 0;

  ifp = bens_fopen(infile, "r");
  ofp = bens_fopen(outfile, "w");
  txt = txt_new();
  txt->read_lines(txt, ifp);

  str = txt->get_data(txt, "FORM.FVER.timestamp");
  if (str != NULL) {
    timestamp = (myDWORD)(strtoul(str, &endptr, 10));
  }
  if (timestamp < AIFC_VERSION1)
    trap_warn("unusual fver.timestamp %lu, expected >= %lu",
      (unsigned long)timestamp, (unsigned long)AIFC_VERSION1);

  str = txt->get_data(txt, "FORM.COMM.numChannels");
  if (str != NULL) {
    numChannels = atoi(str);
  }
  if (numChannels < 1)
    trap_exit("unsupported numChannels");

  str = txt->get_data(txt, "FORM.COMM.sampleSize");
  if (str != NULL) {
    sampleSize = atoi(str);
  }
  if (sampleSize < 1 || sampleSize % 8 != 0)
    trap_exit("unsupported sampleSize");
  samplePointSize = sampleSize / 8;
  if (sampleSize % 8)
    samplePointSize++;

  str = txt->get_data(txt, "FORM.SSND.file");
  if (str == NULL)
    trap_exit("error: missing SSND.file");
  sfp = bens_fopen(str, "r");
  fseek(sfp, 0, SEEK_END);
  ssnd_size = ftell(sfp);
  fseek(sfp, 0, SEEK_SET);
  if (ssnd_size < 1 || ssnd_size % samplePointSize != 0)
    trap_exit("unsupported SSND.size");
  numSampleFrames = ssnd_size / samplePointSize;

  str = txt->get_data(txt, "FORM.COMM.sampleRate");
  if (str != NULL) {
    n = atoi(str);
    double_to_ieee_80((double)n, sampleRate);
  }
  if (n < 1)
    trap_exit("unsupported sampleRate");

  str = txt->get_data(txt, "FORM.COMM.compressionType.str");
  if (str == NULL)
    str = COMPRESSION_TYPE_NONE;
  if (strncmp(str, COMPRESSION_TYPE_NONE, sizeof(myFOURCC)) != 0)
    trap_exit("unsupported compressionType");
  compressionType = str;

  str = txt->get_data(txt, "FORM.COMM.compressionName.str");
  if (str == NULL)
    str = COMPRESSION_NAME_NOT;
  if (strcmp(str, COMPRESSION_NAME_NOT) != 0)
    trap_warn("unexpected compressionName");
  compressionName = str;

  n = 0;
  snprintf(buffer, BUFLEN, "FORM.MARK.%d.id", n + 1);
  str = txt->get_data(txt, buffer);
  while (str != NULL) {
    n++;
    snprintf(buffer, BUFLEN, "FORM.MARK.%d.id", n + 1);
    str = txt->get_data(txt, buffer);
  }
  if (n > 0) {
    aif->mark.numMarkers = n;
    aif->mark.markers = malloc(n * sizeof(mrkr_t *));
    for (i = 0; i < n; i++) {
      aif->mark.markers[i] = malloc(sizeof(mrkr_t));
      memset(aif->mark.markers[i], 0, sizeof(mrkr_t));
      snprintf(buffer, BUFLEN, "FORM.MARK.%d.id", i + 1);
      str = txt->get_data(txt, buffer);
      if (str == NULL)
        trap_exit("no id for marker %d", i + 1);
      r = aif->mark.markers[i]->id = atoi(str);
      if (r < 1)
        trap_exit("invalid id (%d) for marker %d", r, i + 1);
      r = aif->marker_find(aif, r);
      if (r != i)
        trap_warn("duplicate marker id %d (%d,%d)",
          aif->mark.markers[i]->id, r + 1, i + 1);

      snprintf(buffer, BUFLEN, "FORM.MARK.%d.pos", i + 1);
      aif->mark.markers[i]->position = get_int(txt, buffer, 0,
        numSampleFrames, 0);

      snprintf(buffer, BUFLEN, "FORM.MARK.%d.name", i + 1);
      str = txt->get_data(txt, buffer);
      if (str == NULL) {
        trap_warn("no name for marker %d", i + 1);
      } else {
        strncpy(aif->mark.markers[i]->markerName.str, str,
          PSTRING_STR_MAXLEN);
        aif->mark.markers[i]->markerName.count =
          strlen(aif->mark.markers[i]->markerName.str);
      }
    }
  }

  aif->inst.baseNote = get_int(txt, "FORM.INST.baseNote", 0, 127, 60);
  aif->inst.detune = get_int(txt, "FORM.INST.detune", -50, 50, 0);
  aif->inst.lowNote = get_int(txt, "FORM.INST.lowNote", 0, 127, 0);
  aif->inst.highNote = get_int(txt, "FORM.INST.highNote",
    aif->inst.lowNote, 127, 127);
  aif->inst.lowVelocity = get_int(txt, "FORM.INST.lowVelocity", 1, 127, 1);
  aif->inst.highVelocity = get_int(txt, "FORM.INST.highVelocity",
    aif->inst.lowVelocity, 127, 127);
  aif->inst.gain = get_int(txt, "FORM.INST.gain", -32768, 32767, 0);
  aif->inst.sustainLoop.playMode = get_int(txt,
    "FORM.INST.sustainLoop.playMode", 0, AIF_LOOP_MAX, 0);
  aif->inst.sustainLoop.beginLoop = get_int(txt,
    "FORM.INST.sustainLoop.beginLoop", 0, -1, 0);
  aif->inst.sustainLoop.endLoop = get_int(txt,
    "FORM.INST.sustainLoop.endLoop", 0, -1, 0);
  aif->inst.releaseLoop.playMode = get_int(txt,
    "FORM.INST.releaseLoop.playMode", 0, AIF_LOOP_MAX, 0);
  aif->inst.releaseLoop.beginLoop = get_int(txt,
    "FORM.INST.releaseLoop.beginLoop", 0, -1, 0);
  aif->inst.releaseLoop.endLoop = get_int(txt,
    "FORM.INST.releaseLoop.endLoop", 0, -1, 0);
  loop_check(aif, &(aif->inst.sustainLoop), "sustainLoop");
  loop_check(aif, &(aif->inst.releaseLoop), "releaseLoop");

  n = 0;
  snprintf(buffer, BUFLEN, "FORM.COMT.%d.file", n + 1);
  str = txt->get_data(txt, buffer);
  while (str != NULL) {
    n++;
    snprintf(buffer, BUFLEN, "FORM.COMT.%d.file", n + 1);
    str = txt->get_data(txt, buffer);
  }
  if (n > 0) {
    aif->comt.numComments = n;
    aif->comt.comments = malloc(n * sizeof(cmnt_t *));
    for (i = 0; i < n; i++) {
      aif->comt.comments[i] = malloc(sizeof(cmnt_t));
      memset(aif->comt.comments[i], 0, sizeof(cmnt_t));
      snprintf(buffer, BUFLEN, "FORM.COMT.%d.timeStamp", i + 1);
      aif->comt.comments[i]->timeStamp = get_int(txt, buffer, 0, -1,
        unix_to_apple_epoch(time(NULL)));
      snprintf(buffer, BUFLEN, "FORM.COMT.%d.marker", i + 1);
      aif->comt.comments[i]->marker = get_int(txt, buffer, 0,
        aif->mark.numMarkers, 0);
      snprintf(buffer, BUFLEN, "FORM.COMT.%d.file", i + 1);
      str = txt->get_data(txt, buffer);
      if (str == NULL)
        trap_exit("no file for comment %d", i + 1);
      file_read_simple(str, &r, &(aif->comt.comments[i]->text));
      aif->comt.comments[i]->count = r;
    }
  }
  str = txt->get_data(txt, "FORM.NAME.file");
  if (str != NULL) {
    file_read_simple(str, &r, &(aif->name));
  }
  str = txt->get_data(txt, "FORM.AUTH.file");
  if (str != NULL) {
    file_read_simple(str, &r, &(aif->auth));
  }
  str = txt->get_data(txt, "FORM.(c).file");
  if (str != NULL) {
    file_read_simple(str, &r, &(aif->copy));
  }
  str = txt->get_data(txt, "FORM.ANNO.file");
  if (str != NULL) {
    file_read_simple(str, &r, &(aif->anno));
  }

  str = txt->get_data(txt, "FORM.SSND.byteswap");
  if (str != NULL)
    byteswap_do = atoi(str);

  aif->header_set(aif, numChannels, numSampleFrames, sampleSize,
    sampleRate, timestamp, compressionType, compressionName);
  aif->header_write(aif, ofp);
  fseek(ofp, aif->ssnd.fpos_WaveformData, SEEK_SET);
  aif->destroy(aif);

  data_copy(sfp, ofp, aif->ssnd.bytes_WaveformData, byteswap_do);
  bens_fclose(sfp);

  bens_fclose(ifp);
  bens_fclose(ofp);
  txt->warn_unused(txt);
  txt->destroy(txt);
}
