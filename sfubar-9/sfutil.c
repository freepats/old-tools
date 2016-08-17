#include <sys/stat.h>
#include <ctype.h>
#include <errno.h>
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
#include <riffraff/wav.h>
#include <riffraff/aif.h>
#include "sfutil.h"

#define BUFLEN 256

void sfp_zstr(void *vtree, chunk_t *chunk, char *prefix)
{
  tree_t *tree = vtree;
  void *data;
  char *str;
  data = chunk->data_read(chunk, tree->ifp);
  my_get_ZSTR(&str, data, chunk->ckSize);
  fprintf(tree->ofp, "%s.zstr=%s\n", prefix, str);
}

void sfp_iver_rec(void *vtree, chunk_t *chunk, char *prefix)
{
  tree_t *tree = vtree;
  void *data;
  void *p;
  myWORD wMajor;
  myWORD wMinor;

  p = data = chunk->data_read(chunk, tree->ifp);
  p = my_get_WORD_LE(&wMajor, p);
  p = my_get_WORD_LE(&wMinor, p);
  fprintf(tree->ofp, "%s.wMajor.word=%u\n", prefix, wMajor);
  fprintf(tree->ofp, "%s.wMinor.word=%u\n", prefix, wMinor);
}

void sfp_phdr_rec(void *vtree, chunk_t *chunk, char *prefix)
{
  tree_t *tree = vtree;
  void *data;
  void *p;
  void *e;
  int order = 0;
  myCHAR achPresetName[21];
  myWORD wPreset;
  myWORD wBank;
  myWORD wBagNdx;
  myDWORD dwLibrary;
  myDWORD dwGenre;
  myDWORD dwMorphology;

  memset(achPresetName, 0, sizeof(achPresetName));
  p = data = chunk->data_read(chunk, tree->ifp);
  e = p + chunk->ckSize;
  while (p < e) {
    p = my_get_CHAR(achPresetName, p, 20);
    p = my_get_WORD_LE(&wPreset, p);
    p = my_get_WORD_LE(&wBank, p);
    p = my_get_WORD_LE(&wBagNdx, p);
    p = my_get_DWORD_LE(&dwLibrary, p);
    p = my_get_DWORD_LE(&dwGenre, p);
    p = my_get_DWORD_LE(&dwMorphology, p);
    fprintf(tree->ofp, "%s.%d.achPresetName.char[20]=%s\n", prefix,
      order, achPresetName);
    fprintf(tree->ofp, "%s.%d.wPreset.word=%u\n", prefix, order,
      wPreset);
    fprintf(tree->ofp, "%s.%d.wBank.word=%u\n", prefix, order, wBank);
    fprintf(tree->ofp, "%s.%d.wPresetBagNdx.word=%u\n", prefix, order,
      wBagNdx);
    fprintf(tree->ofp, "%s.%d.dwLibrary.dword=%u\n", prefix, order,
      dwLibrary);
    fprintf(tree->ofp, "%s.%d.dwGenre.dword=%u\n", prefix, order,
      dwGenre);
    fprintf(tree->ofp, "%s.%d.dwMorphology.dword=%u\n", prefix, order,
      dwMorphology);
    order++;
  }
}

void sfp_pbag_rec(void *vtree, chunk_t *chunk, char *prefix)
{
  tree_t *tree = vtree;
  void *data;
  void *p;
  void *e;
  int order = 0;
  myWORD wGenNdx;
  myWORD wModNdx;

  p = data = chunk->data_read(chunk, tree->ifp);
  e = p + chunk->ckSize;
  while (p < e) {
    p = my_get_WORD_LE(&wGenNdx, p);
    p = my_get_WORD_LE(&wModNdx, p);
    fprintf(tree->ofp, "%s.%d.wGenNdx.word=%u\n", prefix, order, wGenNdx);
    fprintf(tree->ofp, "%s.%d.wModNdx.word=%u\n", prefix, order, wModNdx);
    order++;
  }
}

void sfp_sfModulator(char *prefix, myWORD mod, FILE *ofp)
{
  int type;
  int polarity;
  int direction;
  int cc;
  int index;
  char *descr;

  index = mod & 127;
  cc = mod & 128;
  direction = mod & 256;
  polarity = mod & 512;
  type = mod & ~512;

  fprintf(ofp, "%s.dword=%u\n", prefix, mod);
  fprintf(ofp, "%s.cc.int=%d\n", prefix, cc);
  fprintf(ofp, "%s.index.int=%d\n", prefix, index);
  if (cc == 0) {
    descr = NULL;
    switch (index) {
    case 0: descr = "No Controller"; break;
    case 2: descr = "Note-On Velocity"; break;
    case 3: descr = "Note-On Key Number"; break;
    case 10: descr = "Poly Pressure"; break;
    case 13: descr = "Channel Pressure"; break;
    case 14: descr = "Pitch Wheel"; break;
    case 16: descr = "Pitch Wheel Sensitivity"; break;
    }
    if (descr != NULL)
      fprintf(ofp, "%s.index.descr=%s\n", prefix, descr);
  }
  fprintf(ofp, "%s.direction.int=%d\n", prefix, direction);
  fprintf(ofp, "%s.polarity.int=%d\n", prefix, polarity);
  fprintf(ofp, "%s.type.int=%d\n", prefix, type);
  switch (type) {
  case 0: descr = "Linear"; break;
  case 1: descr = "Concave"; break;
  case 2: descr = "Convex"; break;
  case 3: descr = "Switch"; break;
  default: descr = "Unknown"; break;
  }
  fprintf(ofp, "%s.type.descr=%s\n", prefix, descr);
}

char *sf_gen_names[] = {
  "startAddrsOffset",
  "endAddrsOffset",
  "startloopAddrsOffset",
  "endloopAddrsOffset",
  "startAddrsCoarseOffset",
  "modLfoToPitch",
  "vibLfoToPitch",
  "modEnvToPitch",
  "initialFilterFc",
  "initialFilterQ",
  "modLfoToFilterFc",
  "modEnvToFilterFc",
  "endAddrsCoarseOffset",
  "modLfoToVolume",
  "unused1",
  "chorusEffectsSend",
  "reverbEffectsSend",
  "pan",
  "unused2",
  "unused3",
  "unused4",
  "delayModLFO",
  "freqModLFO",
  "delayVibLFO",
  "freqVibLFO",
  "delayModEnv",
  "attackModEnv",
  "holdModEnv",
  "decayModEnv",
  "sustainModEnv",
  "releaseModEnv",
  "keynumToModEnvHold",
  "keynumToModEnvDecay",
  "delayVolEnv",
  "attackVolEnv",
  "holdVolEnv",
  "decayVolEnv",
  "sustainVolEnv",
  "releaseVolEnv",
  "keynumToVolEnvHold",
  "keynumToVolEnvDecay",
  "instrument",
  "reserved1",
  "keyRange",
  "velRange",
  "startloopAddrsCoarseOffset",
  "keynum",
  "velocity",
  "initialAttenuation",
  "reserved2",
  "endloopAddrsCoarseOffset",
  "coarseTune",
  "fineTune",
  "sampleID",
  "sampleModes",
  "reserved3",
  "scaleTuning",
  "exclusiveClass",
  "overridingRootKey",
  "unused5",
  "endOper"
};

char *unknown_str = "unknown";

char *sfp_sfGenerator_name_get(myWORD mod)
{
  if (mod < sfg_60_endOper)
    return sf_gen_names[mod];
  return unknown_str;
}

void sfp_sfGenerator(char *prefix, myWORD mod, FILE *ofp)
{
  fprintf(ofp, "%s.word=%u\n", prefix, mod);
  if (((sf_gen_enum_t)mod) <= sfg_60_endOper)
    fprintf(ofp, "%s.descr=%s\n", prefix, sfp_sfGenerator_name_get(mod));
}

void sfp_sfTransform(char *prefix, myWORD op, FILE *ofp)
{
  char *descr;

  fprintf(ofp, "%s.word=%u\n", prefix, op);
  switch (op) {
  case 0: descr = "Linear"; break;
  default: descr = "unknown"; break;
  }
  fprintf(ofp, "%s.descr=%s\n", prefix, descr);
}

void sfp_xmod_rec(void *vtree, chunk_t *chunk, char *prefix)
{
  tree_t *tree = vtree;
  void *data;
  void *p;
  void *e;
  int order = 0;
  char my_prefix[BUFLEN];
  myWORD sfModSrcOper;
  myWORD sfModDestOper;
  mySHORT modAmount;
  myWORD sfModAmtSrcOper;
  myWORD sfModTransOper;
	
  p = data = chunk->data_read(chunk, tree->ifp);
  e = p + chunk->ckSize;
  while (p < e) {
    p = my_get_WORD_LE(&sfModSrcOper, p);
    p = my_get_WORD_LE(&sfModDestOper, p);
    p = my_get_SHORT_LE(&modAmount, p);
    p = my_get_WORD_LE(&sfModAmtSrcOper, p);
    p = my_get_WORD_LE(&sfModTransOper, p);
    snprintf(my_prefix, BUFLEN, "%s.%d.sfModSrcOper", prefix, order);
    sfp_sfModulator(my_prefix, sfModSrcOper, tree->ofp);
    snprintf(my_prefix, BUFLEN, "%s.%d.sfModDestOper", prefix, order);
    sfp_sfGenerator(my_prefix, sfModDestOper, tree->ofp);
    fprintf(tree->ofp, "%s.%d.modAmount.short=%d\n", prefix, order,
      modAmount);
    snprintf(my_prefix, BUFLEN, "%s.%d.sfModAmtSrcOper", prefix, order);
    sfp_sfModulator(my_prefix, sfModAmtSrcOper, tree->ofp);
    snprintf(my_prefix, BUFLEN, "%s.%d.sfModTransOper", prefix, order);
    sfp_sfTransform(my_prefix, sfModTransOper, tree->ofp);
    order++;
  }
}

void sfp_xgen_rec(void *vtree, chunk_t *chunk, char *prefix)
{
  tree_t *tree = vtree;
  void *data;
  void *p;
  void *e;
  int order = 0;
  char my_prefix[BUFLEN];
  myWORD sfGenOper;
  genAmountType genAmount;
	
  p = data = chunk->data_read(chunk, tree->ifp);
  e = p + chunk->ckSize;
  while (p < e) {
    p = my_get_WORD_LE(&sfGenOper, p);
    p = my_get_WORD_LE(&genAmount.wAmount, p);
    if (MY_BYTE_ORDER == MY_BIG_ENDIAN)
    	genAmount.wAmount = XCHG16(genAmount.wAmount);
    snprintf(my_prefix, BUFLEN, "%s.%d.sfGenOper", prefix, order);
    sfp_sfGenerator(my_prefix, sfGenOper, tree->ofp);
    fprintf(tree->ofp, "%s.%d.genAmount.wAmount.word=%u\n", prefix,
      order, genAmount.wAmount);
    fprintf(tree->ofp, "%s.%d.genAmount.shAmount.short=%d\n", prefix,
      order, genAmount.shAmount);
    fprintf(tree->ofp, "%s.%d.genAmount.ranges.byLo.byte=%d\n", prefix,
      order, genAmount.ranges.byLo);
    fprintf(tree->ofp, "%s.%d.genAmount.ranges.byHi.byte=%d\n", prefix,
      order, genAmount.ranges.byHi);
    order++;
  }
}

void sfp_inst_rec(void *vtree, chunk_t *chunk, char *prefix)
{
  tree_t *tree = vtree;
  void *data;
  void *p;
  void *e;
  int order = 0;
  myCHAR achInstName[21];
  myWORD wInstBagNdx;

  memset(achInstName, 0, sizeof(achInstName));
  p = data = chunk->data_read(chunk, tree->ifp);
  e = p + chunk->ckSize;
  while (p < e) {
    p = my_get_CHAR(achInstName, p, 20);
    p = my_get_WORD_LE(&wInstBagNdx, p);
    fprintf(tree->ofp, "%s.%d.achInstName.char[20]=%s\n", prefix, order,
      achInstName);
    fprintf(tree->ofp, "%s.%d.wInstBagNdx.word=%u\n", prefix, order,
      wInstBagNdx);
    order++;
  }
}

void sfp_ibag_rec(void *vtree, chunk_t *chunk, char *prefix)
{
  tree_t *tree = vtree;
  void *data;
  void *p;
  void *e;
  int order = 0;
  myWORD wInstGenNdx;
  myWORD wInstModNdx;

  p = data = chunk->data_read(chunk, tree->ifp);
  e = p + chunk->ckSize;
  while (p < e) {
    p = my_get_WORD_LE(&wInstGenNdx, p);
    p = my_get_WORD_LE(&wInstModNdx, p);
    fprintf(tree->ofp, "%s.%d.wInstGenNdx.word=%u\n", prefix, order,
      wInstGenNdx);
    fprintf(tree->ofp, "%s.%d.wInstModNdx.word=%u\n", prefix, order,
      wInstModNdx);
    order++;
  }
}

int st_vals[] = { 1, 2, 4,
  8, 32769, 32770, 32772,
  32776, -1 };

char *st_names[] = { "monoSample", "rightSample", "leftSample",
  "linkedSample", "RomMonoSample", "RomRightSamnple", "RomLeftSample",
  "RomLinkedSample", "unknown" };

char * sfp_st_name_get(int st)
{
  int i = 0;
  char *retval;
  while (st_vals[i] != -1) {
    retval = st_names[i];
    if (st_vals[i] == st)
      break;
    i++;
  }
  return retval;
}

void sfp_shdr_rec(void *vtree, chunk_t *chunk, char *prefix)
{
  tree_t *tree = vtree;
  void *data;
  void *p;
  void *e;
  int order = 0;
  myCHAR achSampleName[21];
  myDWORD dwStart;
  myDWORD dwEnd;
  myDWORD dwStartloop;
  myDWORD dwEndloop;
  myDWORD dwSampleRate;
  myBYTE byOriginalPitch;
  myCHAR chPitchCorrection;
  myWORD wSampleLink;
  myWORD sfSampleType;

  memset(achSampleName, 0, sizeof(achSampleName));
  p = data = chunk->data_read(chunk, tree->ifp);
  e = p + chunk->ckSize;
  while (p < e) {
    p = my_get_CHAR(achSampleName, p, 20);
    p = my_get_DWORD_LE(&dwStart, p);
    p = my_get_DWORD_LE(&dwEnd, p);
    p = my_get_DWORD_LE(&dwStartloop, p);
    p = my_get_DWORD_LE(&dwEndloop, p);
    p = my_get_DWORD_LE(&dwSampleRate, p);
    p = my_get_BYTE(&byOriginalPitch, p);
    p = my_get_CHAR(&chPitchCorrection, p, 1);
    p = my_get_WORD_LE(&wSampleLink, p);
    p = my_get_WORD_LE(&sfSampleType, p);
    fprintf(tree->ofp, "%s.%d.achSampleName.char[20]=%s\n", prefix,
      order, achSampleName);
    fprintf(tree->ofp, "%s.%d.dwStart.dword=%u\n", prefix, order,
      dwStart);
    fprintf(tree->ofp, "%s.%d.dwEnd.dword=%u\n", prefix, order, dwEnd);
    fprintf(tree->ofp, "%s.%d.dwStartloop.dword=%u\n", prefix, order,
      dwStartloop);
    fprintf(tree->ofp, "%s.%d.dwEndloop.dword=%u\n", prefix, order,
      dwEndloop);
    fprintf(tree->ofp, "%s.%d.SampleRate.dword=%u\n", prefix, order,
      dwSampleRate);
    fprintf(tree->ofp, "%s.%d.byOriginalPitch.byte=%d\n", prefix, order,
      byOriginalPitch);
    fprintf(tree->ofp, "%s.%d.chPitchCorrection.char=%d\n", prefix,
      order, chPitchCorrection);
    fprintf(tree->ofp, "%s.%d.wSampleLink.word=%u\n", prefix, order,
      wSampleLink);
    fprintf(tree->ofp, "%s.%d.sfSampleType.word=%u\n", prefix, order,
      sfSampleType);
    fprintf(tree->ofp, "%s.%d.sfSampleType.descr=%s\n", prefix, order,
      sfp_st_name_get(sfSampleType));
    order++;
  }
}

void sfp_meta(void *vtree, chunk_t *chunk, char *prefix)
{
  tree_t *tree = vtree;
  fprintf(tree->ofp, "%s ~ BEGIN\n", prefix);
}

void sfp_ignore(void *vtree, chunk_t *chunk, char *prefix)
{
  /* ignore, print nothing */
}

void sfp_shdr_txt(FILE *ifp, FILE *ofp, char *wt_prefix, tree_t *tree)
{
  int n;
  chunk_t *shdr;
  chunk_t *smpl;
  void *data;
  void *p;
  void *e;
  int order = 0;
  char filename[BUFLEN];
  FILE *sfp;
  myCHAR achSampleName[21];
  myDWORD dwStart;
  myDWORD dwEnd;
  myDWORD dwStartloop;
  myDWORD dwEndloop;
  myDWORD dwSampleRate;
  myBYTE byOriginalPitch;
  myCHAR chPitchCorrection;
  myWORD wSampleLink;
  myWORD sfSampleType;
  long sample_count;
  wav_t *wav;

  n = tree->ckID_find(tree, "smpl", NULL, -1);
  if (n == -1)
    trap_exit("no wavetable data");
  smpl = tree->items[n].chunk;

  n = tree->ckID_find(tree, "shdr", NULL, -1);
  if (n == -1)
    trap_exit("no wavetable header");
  shdr = tree->items[n].chunk;

  memset(achSampleName, 0, sizeof(achSampleName));
  p = data = shdr->data_read(shdr, ifp);
  e = p + shdr->ckSize - 46;
  while (p < e) {
    p = my_get_CHAR(achSampleName, p, 20);
    p = my_get_DWORD_LE(&dwStart, p);
    p = my_get_DWORD_LE(&dwEnd, p);
    p = my_get_DWORD_LE(&dwStartloop, p);
    p = my_get_DWORD_LE(&dwEndloop, p);
    p = my_get_DWORD_LE(&dwSampleRate, p);
    p = my_get_BYTE(&byOriginalPitch, p);
    p = my_get_CHAR(&chPitchCorrection, p, 1);
    p = my_get_WORD_LE(&wSampleLink, p);
    p = my_get_WORD_LE(&sfSampleType, p);
    if (sfSampleType == 2 || sfSampleType == 4) {
      trap_warn("wavetable %d, %s unsupported, defaulting to monoSample",
      order + 1, sfp_st_name_get(sfSampleType));
    } else if (sfSampleType != 1) {
      trap_exit("wavetable %d, %s unsupported", order + 1,
        sfp_st_name_get(sfSampleType));
    }
    fprintf(ofp, "SF2.wt.%d.wav=%s%d.wav\n", order + 1, wt_prefix,
      order + 1);
    fprintf(ofp, "SF2.wt.%d.name=%s\n", order + 1, achSampleName);
    if (dwStartloop != dwStart || dwEndloop != dwStart) {
      fprintf(ofp, "SF2.wt.%d.loopstart=%u\n", order + 1,
        dwStartloop - dwStart);
      fprintf(ofp, "SF2.wt.%d.loopend=%u\n", order + 1,
        dwEndloop - dwStart);
    }

    if (byOriginalPitch != 60) {
      fprintf(ofp, "SF2.wt.%d.pitch=%d\n", order + 1, byOriginalPitch);
    }
    if (chPitchCorrection != 0) {
      fprintf(ofp, "SF2.wt.%d.pitchcorr=%d\n", order + 1, chPitchCorrection);
    }

    snprintf(filename, BUFLEN, "%s%d.wav", wt_prefix, order + 1);
    sfp = bens_fopen(filename, "w");
    smpl->data_seek(smpl, ifp);
    fseek(ifp, dwStart * sizeof(mySHORT), SEEK_CUR);
    sample_count = dwEnd - dwStart;
    wav = wav_new();
    wav->header_set(wav, 1, sizeof(mySHORT), dwSampleRate, sample_count);
    wav->header_write(wav, sfp);
    wav->destroy(wav);
    data_copy(ifp, sfp, sample_count * sizeof(mySHORT), 0);
    bens_fclose(sfp);

    order++;
  }
}

zone_type_t zone_types[] = {
 { zt_instrument, "instrument", "in", "wavetable", "wt", 0,
   sfg_53_sampleID, "ibag", "igen", "imod", "inst" },
 { zt_preset, "preset", "ps", "instrument", "in", 0, sfg_41_instrument,
   "pbag", "pgen", "pmod", "phdr" }
};

zone_type_t *sfp_zone_type_get(zone_type_enum_t zone_type)
{
  zone_type_t *retval;

  switch (zone_type) {
  case zt_instrument:
  case zt_preset:
    retval = &(zone_types[zone_type]);
    break;
  default:
    trap_exit("Invalid zone_type %d", zone_type);
  }
  return retval;
}

void sfp_zone_txt(FILE *ifp, FILE *ofp, tree_t *tree,
  zone_type_enum_t zone_type)
{
  int n;
  int i;
  int j;
  int k;
  chunk_t *chunk;
  void *data;
  void *bag_data;
  void *gen_data;
  void *p;
  void *e;
  int order = 0;
  int range_begin;
  int range_end;
  int env_val;
  double dt;
  zone_type_t *zt;

  /* inst record */
  myCHAR achName[21];
  myWORD wBagNdx;
  myCHAR last_achName[21];
  myWORD last_wBagNdx;

  /* hdr record */
  myWORD wPreset;
  myWORD wBank;
  myDWORD dwLibrary;
  myDWORD dwGenre;
  myDWORD dwMorphology;
  myWORD last_wBank;

  /* bag */
  myWORD wGenNdx;
  int bag_rec_len;

  /* gen */
  myWORD sfGenOper;
  genAmountType genAmount;
  int gen_rec_len;

  zt = sfp_zone_type_get(zone_type);

  n = tree->ckID_find(tree, zt->bagc, NULL, -1);
  if (n == -1)
    trap_exit("no %s section", zt->bagc);
  bag_data = tree->items[n].chunk->data_read(tree->items[n].chunk, ifp);
  bag_rec_len = sizeof(myWORD) * 2;

  n = tree->ckID_find(tree, zt->genc, NULL, -1);
  if (n == -1)
    trap_exit("no %s section", zt->genc);
  gen_data = tree->items[n].chunk->data_read(tree->items[n].chunk, ifp);
  gen_rec_len = sizeof(myWORD) + sizeof(genAmountType);

  n = tree->ckID_find(tree, zt->hdrc, NULL, -1);
  if (n == -1)
    trap_exit("no %s section", zt->hdrc);
  chunk = tree->items[n].chunk;

  memset(achName, 0, sizeof(achName));
  p = data = chunk->data_read(chunk, ifp);
  e = p + chunk->ckSize;
  while (p < e) {
    k = 1;
    strcpy(last_achName, achName);
    last_wBagNdx = wBagNdx;
    last_wBank = wBank;
    switch (zone_type) {
    case zt_instrument:
      p = my_get_CHAR(achName, p, 20);
      p = my_get_WORD_LE(&wBagNdx, p);
      break;
    case zt_preset:
      p = my_get_CHAR(achName, p, 20);
      p = my_get_WORD_LE(&wPreset, p);
      p = my_get_WORD_LE(&wBank, p);
      p = my_get_WORD_LE(&wBagNdx, p);
      p = my_get_DWORD_LE(&dwLibrary, p);
      p = my_get_DWORD_LE(&dwGenre, p);
      p = my_get_DWORD_LE(&dwMorphology, p);
      break;
    }
    if (order > 0) {
      fprintf(ofp, "SF2.%s.%d.name=%s\n", zt->key, order, last_achName);
      if (zone_type == zt_preset)
        fprintf(ofp, "SF2.%s.%d.bank=%u\n", zt->key, order, last_wBank);
      for (i = last_wBagNdx; i < wBagNdx; i++) {
        my_get_WORD_LE(&wGenNdx, bag_data + i * bag_rec_len);
        j = wGenNdx;
        do {
          my_get_WORD_LE(&sfGenOper, gen_data + j * gen_rec_len);
          switch (sfGenOper) {
          case sfg_43_keyRange:
          case sfg_44_velRange:
            my_get_BYTE(&genAmount.ranges.byLo,
              gen_data + sizeof(myWORD) + j * gen_rec_len);
            my_get_BYTE(&genAmount.ranges.byHi,
              1 + gen_data + sizeof(myWORD) + j * gen_rec_len);
            range_begin = genAmount.ranges.byLo;
            range_end = genAmount.ranges.byHi;
            fprintf(ofp, "SF2.%s.%d.zone.%d.%s=%d,%d\n", zt->key, order,
              k, sfp_sfGenerator_name_get(sfGenOper), range_begin,
              range_end);
            break;
          case sfg_33_delayVolEnv:
          case sfg_34_attackVolEnv:
          case sfg_35_holdVolEnv:
          case sfg_36_decayVolEnv:
          case sfg_38_releaseVolEnv:
            my_get_SHORT_LE(&genAmount.shAmount,
              gen_data + sizeof(myWORD) + j * gen_rec_len);
            env_val = genAmount.shAmount;
            if (env_val == SHMIN) {
              dt = 0.0;
            } else {
              dt = pow(2.0, env_val / 1200.0);
            }
            fprintf(ofp, "SF2.%s.%d.zone.%d.%s=%0.04f\n", zt->key,
              order, k, sfp_sfGenerator_name_get(sfGenOper), dt);
            break;
          case sfg_37_sustainVolEnv:
          case sfg_39_keynumToVolEnvHold:
          case sfg_40_keynumToVolEnvDecay:
            my_get_SHORT_LE(&genAmount.shAmount,
              gen_data + sizeof(myWORD) + j * gen_rec_len);
            env_val = genAmount.shAmount;
            fprintf(ofp, "SF2.%s.%d.zone.%d.%s=%d\n", zt->key, order, k,
              sfp_sfGenerator_name_get(sfGenOper), env_val);
            break;
          case sfg_53_sampleID:
          case sfg_41_instrument:
            if (sfGenOper != zt->oper)
              trap_exit("%s operator found in %s context",
                sfp_sfGenerator_name_get(sfGenOper), zt->keyn);
            my_get_WORD_LE(&genAmount.wAmount,
              gen_data + sizeof(myWORD) + j * gen_rec_len);
            fprintf(ofp, "SF2.%s.%d.zone.%d.%s=%u\n", zt->key, order, k,
              zt->item, genAmount.wAmount + 1);
            break;
          case sfg_58_overridingRootKey:
            if (zone_type == zt_preset) {
              trap_warn("ignoring overridingRootKey in preset zone");
            } else {
              my_get_WORD_LE(&genAmount.wAmount,
                gen_data + sizeof(myWORD) + j * gen_rec_len);
              fprintf(ofp, "SF2.%s.%d.zone.%d.overridingRootKey=%d\n",
                zt->key, order, k, genAmount.wAmount);
            }
            break;
          case sfg_57_exclusiveClass:
            if (zone_type == zt_preset) {
              trap_warn("ignoring exclusiveClass in preset zone");
            } else {
              my_get_WORD_LE(&genAmount.wAmount,
                gen_data + sizeof(myWORD) + j * gen_rec_len);
              fprintf(ofp, "SF2.%s.%d.zone.%d.exclusiveClass=%d\n",
                zt->key, order, k, genAmount.wAmount);
            }
            break;
          }
          j++;
        } while (sfGenOper != zt->oper);
        k++;
      }
    }
    order++;
  }
}

tree_item sf_items[] = {
{ 0, "RIFF", "sfbk", sfp_meta, NULL },
{ 1, "LIST", "INFO", sfp_meta, NULL },
{ 1, "LIST", "sdta", sfp_meta, NULL },
{ 1, "LIST", "pdta", sfp_meta, NULL },
{ 2, "ifil", NULL, sfp_iver_rec, NULL },
{ 2, "isng", NULL, sfp_zstr, NULL },
{ 2, "INAM", NULL, sfp_zstr, NULL },
{ 2, "irom", NULL, sfp_zstr, NULL },
{ 2, "iver", NULL, sfp_iver_rec, NULL },
{ 2, "ICRD", NULL, sfp_zstr, NULL },
{ 2, "IENG", NULL, sfp_zstr, NULL },
{ 2, "IPRD", NULL, sfp_zstr, NULL },
{ 2, "ICOP", NULL, sfp_zstr, NULL },
{ 2, "ICMT", NULL, sfp_zstr, NULL },
{ 2, "ISFT", NULL, sfp_zstr, NULL },
{ 2, "smpl", NULL, sfp_ignore, NULL },
{ 2, "phdr", NULL, sfp_phdr_rec, NULL },
{ 2, "pbag", NULL, sfp_pbag_rec, NULL },
{ 2, "pmod", NULL, sfp_xmod_rec, NULL },
{ 2, "pgen", NULL, sfp_xgen_rec, NULL },
{ 2, "inst", NULL, sfp_inst_rec, NULL },
{ 2, "ibag", NULL, sfp_ibag_rec, NULL },
{ 2, "imod", NULL, sfp_xmod_rec, NULL },
{ 2, "igen", NULL, sfp_xgen_rec, NULL },
{ 2, "shdr", NULL, sfp_shdr_rec, NULL },
{ -1, "", NULL, NULL, NULL }
};

tree_container sf_containers[] = { "RIFF", "LIST", "" };

char get_ascii_date_retval[BUFLEN];
char *get_ascii_date(void)
{
  time_t now;

  time(&now);
  strftime(get_ascii_date_retval, BUFLEN, "%b %d, %Y", localtime(&now));
  get_ascii_date_retval[0] = toupper((int)get_ascii_date_retval[0]);
  return get_ascii_date_retval;
}

void sfp_txt(FILE *ifp, FILE *ofp, char *wt_prefix, tree_t *tree)
{
  int n;
  char *str;

  n = tree->ckID_find(tree, "isng", NULL, -1);
  if (n == -1) {
    str = "sfubar song";
  } else {
    str = tree->items[n].chunk->data_read(tree->items[n].chunk, ifp);
  }
  fprintf(ofp, "RIFF.LIST.isng.zstr=%s\n", str);
	
  n = tree->ckID_find(tree, "INAM", NULL, -1);
  if (n == -1) {
    str = "sfubar instruments";
  } else {
    str = tree->items[n].chunk->data_read(tree->items[n].chunk, ifp);
  }
  fprintf(ofp, "RIFF.LIST.INAM.zstr=%s\n", str);

  n = tree->ckID_find(tree, "ICRD", NULL, -1);
  if (n == -1) {
    str = get_ascii_date();
  } else {
    str = tree->items[n].chunk->data_read(tree->items[n].chunk, ifp);
  }
  fprintf(ofp, "RIFF.LIST.ICRD.zstr=%s\n", str);

  n = tree->ckID_find(tree, "IPRD", NULL, -1);
  if (n == -1) {
    str = "SBAWE32";
  } else {
    str = tree->items[n].chunk->data_read(tree->items[n].chunk, ifp);
  }
  fprintf(ofp, "RIFF.LIST.IPRD.zstr=%s\n", str);
	
  n = tree->ckID_find(tree, "ISFT", NULL, -1);
  if (n == -1) {
    str = malloc(BUFLEN);
    snprintf(str, BUFLEN, "sfubar %d:sfubar %d", SFUTIL_VERSION,
      SFUTIL_VERSION);
  } else {
    str = strdup(tree->items[n].chunk->data_read(tree->items[n].chunk, ifp));
  }
  fprintf(ofp, "RIFF.LIST.ISFT.zstr=%s\n", str);
  free(str);

  sfp_shdr_txt(ifp, ofp, wt_prefix, tree);
  sfp_zone_txt(ifp, ofp, tree, zt_instrument);
  sfp_zone_txt(ifp, ofp, tree, zt_preset);
}

void sf_to_txt(char *infile, char *outfile, int format)
{
  FILE *ifp;
  FILE *ofp;
  chunk_t *chunk;
  char wt_prefix[BUFLEN];
  char *p;
  tree_t *tree;
  int riff_item;

  strncpy(wt_prefix, outfile, BUFLEN - 1);
  wt_prefix[BUFLEN - 1] = '\000';
  p = strrchr(wt_prefix, '.');
  if (p != NULL)
    *p = '\000';

  ifp = bens_fopen(infile, "r");
  ofp = bens_fopen(outfile, "w");

  chunk = chunk_new(NULL, ON_DISK);
  chunk->header_read(chunk, ifp);
  tree = tree_new(sf_items, sf_containers, NULL);
  tree->read(tree, chunk, 0, ifp);

  if (format == 1) {
    riff_item = tree->ckID_find(tree, "RIFF", "sfbk", -1);
    tree->print(tree, riff_item, "", wt_prefix, ifp, ofp);
  } else {
    sfp_txt(ifp, ofp, wt_prefix, tree);
  }
  tree->destroy(tree);
  bens_fclose(ifp);
  bens_fclose(ofp);
}

file_type_t wavetable_find(txt_t *txt, char *buffer, int i, int *ret_n)
{
  file_type_t retval = file_type_end;
  int n;
  snprintf(buffer, BUFLEN, "SF2.wt.%d.file", i + 1);
  n = txt->find_key(txt, buffer);
  if (n == -1) {
    snprintf(buffer, BUFLEN, "SF2.wt.%d.wav", i + 1);
    n = txt->find_key(txt, buffer);
  }
  if (n != -1) {
    retval = file_type_wav;
  } else {
    snprintf(buffer, BUFLEN, "SF2.wt.%d.aif", i + 1);
    n = txt->find_key(txt, buffer);
    if (n != -1)
      retval = file_type_aif;
  }
  *ret_n = n;
  return retval;
}

int wavetable_max(txt_t *txt)
{
  char buffer[BUFLEN];
  int i = 0;
  int r;
  int ft;
  ft = wavetable_find(txt, buffer, i, &r);
  while (ft != file_type_end) {
    i++;
    ft = wavetable_find(txt, buffer, i, &r);
  }
  return i - 1;
}

int instrument_max(txt_t *txt)
{
  char buffer[BUFLEN];
  int i = 0;
  int n;

  snprintf(buffer, BUFLEN, "SF2.in.%d.name", i + 1);
  n = txt->find_key(txt, buffer);
  while (n != -1) {
    i++;
    snprintf(buffer, BUFLEN, "SF2.in.%d.name", i + 1);
    n = txt->find_key(txt, buffer);
  }
  return i - 1;
}

wav_t *wav_header_read_for_sf2(FILE *sfp, int i)
{
  wav_t *wav = wav_new();

  if (wav->header_read(wav, sfp) != 1)
    trap_exit("write_sample_chunks: wave_header_read failed");
  if (wav->fmt.wFormatTag != WAVE_FORMAT_PCM)
    trap_exit("wavetable %d:  wFormatTag = %d", i + 1, wav->fmt.wFormatTag);
  if (wav->fmt.wChannels != 1)
    trap_exit("wavetable %d: wChannels = %d", i + 1, wav->fmt.wChannels);
  if (wav->fmt.wBitsPerSample != 16)
    trap_exit("wavetable %d: wBitsPerSample = %d", i + 1,
      wav->fmt.wBitsPerSample);
  if (wav->fmt.dwSamplesPerSec < 1)
    trap_exit("wavetable %d: illegal rate", i + 1);
  if (wav->fmt.dwSamplesPerSec < 400 || wav->fmt.dwSamplesPerSec > 50000)
    trap_warn("wavetable %d: rate out of bounds", i + 1);
  if (!is_even(wav->data.size))
    trap_exit("wavetable %d: odd bytecount", i + 1);
  return wav;
}

aif_t *aif_header_read_for_sf2(FILE *sfp, int i)
{
  aif_t *aif = aif_new();
  if (aif->header_read(aif, sfp) != 1)
    trap_exit("write_sample_chunks: aif_header_read failed");
  if (aif->comm.numChannels != 1)
    trap_exit("wavetable %d: numChannels = %d", i + 1, aif->comm.numChannels);
  if (aif->comm.sampleSize != 16)
    trap_exit("wavetable %d: sampleSize = %d", i + 1, aif->comm.sampleSize);
  if (aif->comm.double_sampleRate < 1)
    trap_exit("wavetable %d: negative rate", i + 1);
  if (aif->comm.double_sampleRate != ceil(aif->comm.double_sampleRate))
    trap_exit("wavetable %d: fractional rate", i + 1);
  if (aif->comm.double_sampleRate < 400.0 ||
    aif->comm.double_sampleRate > 50000.0)
      trap_warn("wavetable %d: rate out of bounds", i + 1);
  if (!is_even(aif->ssnd.bytes_WaveformData))
    trap_exit("wavetable %d: odd bytecount", i + 1);
  if (strncmp(aif->comm.compressionType.str, "NONE", 4) != 0)
    trap_exit("wavetable %d: compressed encoding", i + 1);
  return aif;
}

void write_sample_chunks(FILE *ofp, chunk_t *RIFF, chunk_t *shdr, txt_t *txt)
{
  int i = 0;
  int n;
  char buffer[BUFLEN];
  char wt_name_zstr[20];
  char *wt_file;
  char *wt_name;
  char *wt_loopstart_str;
  char *wt_loopend_str;
  char *wt_pitch_str;
  char *wt_pitchcorr_str;
  myDWORD wt_start;
  myDWORD wt_end;
  myDWORD wt_loopstart = 0;
  myDWORD wt_loopend = 0;
  myDWORD wt_rate;
  myBYTE byOriginalPitch = 60;
  myCHAR chPitchCorrection = 0;
  myWORD wSampleLink;
  myWORD sfSampleType;
  FILE *sfp;
  void *p;
  void *pad;
  chunk_t *LIST;
  chunk_t *smpl;
  int wt_size;
  wav_t *wav;
  aif_t *aif;
  file_type_t file_type;
  long wt_fpos;
  int wt_byte_swap;
  int t;

  LIST = chunk_new(NULL, IN_RAM);
  strncpy(LIST->ckID.str, "LIST", 4);
  LIST->header_write(LIST, ofp);
  LIST->data_write(LIST, "sdta", 4, ofp);

  smpl = chunk_new(NULL, IN_RAM);
  strncpy(smpl->ckID.str, "smpl", 4);
  smpl->header_write(smpl, ofp);

  pad = malloc(46 * sizeof(mySHORT));
  memset(pad, 0, 46 * sizeof(mySHORT));

  file_type = wavetable_find(txt, buffer, i, &n);
  if (n == -1)
    trap_exit("Could not find any wavetables");
  while (n != -1) {
    wt_file = txt->get_data(txt, buffer);
    if (wt_file == NULL)
      trap_exit("wavetable %d: lacks filename", i + 1);

    sfp = bens_fopen(wt_file, "r");
    switch (file_type) {
    case file_type_wav:
      wav = wav_header_read_for_sf2(sfp, i);
      wt_fpos = wav->data.offset;
      wt_rate = wav->fmt.dwSamplesPerSec;
      wt_size = wav->data.size / sizeof(mySHORT);
      wt_byte_swap = 0;
      wav->destroy(wav);
      break;
    case file_type_aif:
      aif = aif_header_read_for_sf2(sfp, i);
      wt_fpos = aif->ssnd.fpos_WaveformData;
      wt_rate = (myDWORD)(aif->comm.double_sampleRate);
      wt_size = aif->comm.numSampleFrames;
      wt_byte_swap = 1;
      aif->destroy(aif);
      break;
    default:
      trap_exit("wavetable %d: unsupported format", i + 1);
    }

    snprintf(buffer, BUFLEN, "SF2.wt.%d.name", i + 1);
    wt_name = txt->get_data(txt, buffer);
    if (wt_name == NULL)
      wt_name = "";
    if (strlen(wt_name) > 19)
      trap_warn("wavetable %d: name longer than 19 characters, truncating",
        i + 1);
    memset(wt_name_zstr, 0, 20);
    strncpy(wt_name_zstr, wt_name, 19);

    wt_loopstart = 0;
    snprintf(buffer, BUFLEN, "SF2.wt.%d.loopstart", i + 1);
    wt_loopstart_str = txt->get_data(txt, buffer);
    if (wt_loopstart_str != NULL)
      wt_loopstart = atoi(wt_loopstart_str);
    if (wt_loopstart < 0) {
      trap_warn("wavetable %d: loopstart < 0, changing to 0", i + 1);
      wt_loopstart = 0;
    }

    wt_loopend = 0;
    snprintf(buffer, BUFLEN, "SF2.wt.%d.loopend", i + 1);
    wt_loopend_str = txt->get_data(txt, buffer);
    if (wt_loopend_str != NULL)
      wt_loopend = atoi(wt_loopend_str);
    if (wt_loopend < 0) {
      trap_warn("wavetable %d: loopend < 0, changing to 0", i + 1);
      wt_loopend = 0;
    }

    if (wt_loopstart > wt_size)
      trap_exit("wavetable %d: loopstart larger than wavetable", i + 1);
    if (wt_loopend > wt_size)
      trap_exit("wavetable %d: loopend larger than wavetable", i + 1);
    if (wt_loopstart > 0 || wt_loopend > 0) {
      if (wt_loopstart < 8 * sizeof(mySHORT))
        trap_warn("wavetable %d: less than 8 samples before loopstart (%d)",
          i + 1, wt_loopstart);
      if (wt_loopend - wt_loopstart < 31 * sizeof(mySHORT))
        trap_warn("wavetable %d: loop is less than 32 samples long (%d)",
          i + 1, wt_loopend - wt_loopstart);
      if (wt_size - wt_loopend < 7 * sizeof(mySHORT))
        trap_warn("wavetable %d: less than 8 samples after loopend (%d)",
          i + 1, wt_loopend);
    }

    snprintf(buffer, BUFLEN, "SF2.wt.%d.pitch", i + 1);
    wt_pitch_str = txt->get_data(txt, buffer);
    t = byOriginalPitch;
    if (wt_pitch_str != NULL)
      byOriginalPitch = atoi(wt_pitch_str);
    if (byOriginalPitch > 127) {
      byOriginalPitch = 60; /* MIDI C-5 */
      if (byOriginalPitch != 255)
        trap_warn("wavetable %d: illegal pitch MIDI note", i + 1);
    }

    snprintf(buffer, BUFLEN, "SF2.wt.%d.pitchcorr", i + 1);
    wt_pitchcorr_str = txt->get_data(txt, buffer);
    t = chPitchCorrection;
    if (wt_pitchcorr_str != NULL)
      chPitchCorrection = atoi(wt_pitchcorr_str);

    wSampleLink = 0; /* not linked */
    sfSampleType = 1; /* mono wavetable */

    wt_start = smpl->ckSize / sizeof(mySHORT);
    wt_end = wt_start + wt_size;
    wt_loopstart += wt_start;
    wt_loopend += wt_start;
    smpl->ckSize = (wt_end + 46) * sizeof(mySHORT);

    fseek(sfp, wt_fpos, SEEK_SET);
    data_copy(sfp, ofp, wt_size * sizeof(mySHORT), wt_byte_swap);
    bens_fclose(sfp);
    bens_fwrite(pad, sizeof(mySHORT), 46, ofp);

    shdr->data = realloc(shdr->data, (i + 1) * 46);
    p = shdr->data + i * 46;
    p = my_put_CHAR(wt_name_zstr, p, 20);
    p = my_put_DWORD_LE(&wt_start, p);
    p = my_put_DWORD_LE(&wt_end, p);
    p = my_put_DWORD_LE(&wt_loopstart, p);
    p = my_put_DWORD_LE(&wt_loopend, p);
    p = my_put_DWORD_LE(&wt_rate, p);
    p = my_put_BYTE(&byOriginalPitch, p);
    p = my_put_CHAR(&chPitchCorrection, p, 1);
    p = my_put_WORD_LE(&wSampleLink, p);
    p = my_put_WORD_LE(&sfSampleType, p);

    i++;
    file_type = wavetable_find(txt, buffer, i, &n);
  }

  memset(wt_name_zstr, 0, 20);
  strcpy(wt_name_zstr, "EOS");
  wt_start = 0;
  wt_end = 0;
  wt_loopstart = 0;
  wt_loopend = 0;
  wt_rate = 0;
  byOriginalPitch = 0;
  chPitchCorrection = 0;
  wSampleLink = 0;
  sfSampleType = 0;

  shdr->ckSize = (i + 1) * 46;
  shdr->data = realloc(shdr->data, shdr->ckSize);
  p = shdr->data + i * 46;
  p = my_put_CHAR(wt_name_zstr, p, 20);
  p = my_put_DWORD_LE(&wt_start, p);
  p = my_put_DWORD_LE(&wt_end, p);
  p = my_put_DWORD_LE(&wt_loopstart, p);
  p = my_put_DWORD_LE(&wt_loopend, p);
  p = my_put_DWORD_LE(&wt_rate, p);
  p = my_put_BYTE(&byOriginalPitch, p);
  p = my_put_CHAR(&chPitchCorrection, p, 1);
  p = my_put_WORD_LE(&wSampleLink, p);
  p = my_put_WORD_LE(&sfSampleType, p);

  LIST->ckSize += smpl->ckSize + 8;
  smpl->start_seek(smpl, ofp);
  smpl->header_write(smpl, ofp);
  smpl->end_seek(smpl, ofp);
  LIST->ckSize += smpl->pad_write(smpl, ofp);
  RIFF->ckSize += LIST->ckSize + 8;
  LIST->start_seek(LIST, ofp);
  LIST->header_write(LIST, ofp);
  LIST->end_seek(LIST, ofp);
  RIFF->ckSize += LIST->pad_write(LIST, ofp);
}


void write_zone(FILE *ofp, zone_type_enum_t zone_type, chunk_t *gen,
  chunk_t *mod, chunk_t *bag, chunk_t *hdr, txt_t *txt)
{
  int k = 1;
  char buffer[BUFLEN];
  char *name;
  char name_zstr[20];
  char *temp_str;
  char *temp_begin_str;
  char *temp_end_str;
  int key_range_begin;
  int key_range_end;
  int vel_range_begin;
  int vel_range_end;
  int env_delay;
  int env_attack;
  int env_hold;
  int env_decay;
  int env_sustain;
  int env_release;
  int ktve_hold;
  int ktve_decay;
  int overridingRootKey;
  int exclusiveClass;
  int item;
  int last_nbag = 0;
  int loopstart = 0;
  int loopend = 0;
  int iops_count = 0;
  void *p;
  zone_type_t *zt;
  int n;
  int j;

  myWORD sfGenOper;
  genAmountType genAmount;
  myWORD wGenNdx;
  myWORD wModNdx;
  myWORD wBagNdx;
  myWORD wPreset;
  myWORD wBank;
  myDWORD dwLibrary;
  myDWORD dwGenre;
  myDWORD dwMorphology;
 
  zt = sfp_zone_type_get(zone_type);
  switch (zone_type) {
  case zt_instrument:
    zt->item_max = wavetable_max(txt);
    break;
  case zt_preset:
    zt->item_max = instrument_max(txt);
    break;
  }

  snprintf(buffer, BUFLEN, "SF2.%s.%d.name", zt->key, k);
  n = txt->find_key(txt, buffer);
  if (n == -1)
    trap_exit("Could not find any %ss", zt->keyn);
  while (n != -1) {
    name = txt->get_data(txt, buffer);
    if (name == NULL)
      name = "";
    if (strlen(name) > 19)
      trap_warn("%s %d: name greater than 19 characters, truncating",
        zt->keyn, k);

    j = 0;
    snprintf(buffer, BUFLEN, "SF2.%s.%d.zone.%d.%s", zt->key, k, j + 1,
      zt->item);
    n = txt->find_key(txt, buffer);
    if (n == -1) {
      trap_warn("%s %d: no %s", zt->keyn, k, zt->itemn);

      gen->data = realloc(gen->data, (gen->count + 2) * 4);
      p = gen->data + gen->count * 4;
      sfGenOper = sfg_60_endOper;
      genAmount.wAmount = 0;
      p = my_put_WORD_LE(&sfGenOper, p);
      p = my_put_WORD_LE(&genAmount.wAmount, p);

      bag->data = realloc(bag->data, (bag->count + 1) * 4);
      p = bag->data + bag->count * 4;
      wGenNdx = gen->count;
      wModNdx = mod->count;
      p = my_put_WORD_LE(&wGenNdx, p);
      p = my_put_WORD_LE(&wModNdx, p);

      gen->count++;
      bag->count++;
    }
    while (n != -1) {
      iops_count = 1;
      if (zone_type == zt_instrument) {
        iops_count++;
      }

      temp_str = txt->get_data(txt, buffer);
      if (temp_str == NULL)
        trap_exit("%s %d zone %d: no %s", zt->keyn, k, j + 1, zt->itemn);
      item = atoi(temp_str) - 1;
      if (item < 0 || item > zt->item_max)
        trap_exit("%s %d: zone %d: invalid %s index %d (range 0,%d)",
          zt->keyn, k, j + 1, zt->itemn, item, zt->item_max);

      snprintf(buffer, BUFLEN, "SF2.%s.%d.zone.%d.keyRange", zt->key, k, j + 1);
      temp_str = txt->get_data(txt, buffer);
      if (temp_str == NULL) {
      	trap_warn("%s %d zone %d: no keyRange", zt->keyn, k, j + 1);
        key_range_begin = key_range_end = -1;
      } else {
        iops_count++;
        temp_end_str = strchr(temp_str, ',');
        if (temp_end_str == NULL)
          trap_exit("%s %d zone %d: invalid keyRange", zt->keyn, k, j + 1);
        temp_end_str[0] = '\000';
        temp_end_str++;
        temp_begin_str = temp_str;
        key_range_begin = atoi(temp_begin_str);
        key_range_end = atoi(temp_end_str);
        if (key_range_begin < 0 || key_range_begin > 127 ||
          key_range_end < 0 || key_range_end > 127)
            trap_exit("%s %d zone %d: invalid keyRange", zt->keyn, k, j + 1);
      }

      snprintf(buffer, BUFLEN, "SF2.%s.%d.zone.%d.velRange", zt->key, k,
        j + 1);
      n = txt->find_key(txt, buffer);
      if (n == -1) {
        temp_str = NULL;
      } else {
        temp_str = txt->get_data(txt, buffer);
      }
      if (temp_str == NULL) {
        vel_range_begin = vel_range_end = -1;
      } else {
        iops_count++;
        temp_end_str = strchr(temp_str, ',');
        if (temp_end_str == NULL)
          trap_exit("%s %d zone %d: invalid velRange", zt->keyn, k, j + 1);
        temp_end_str[0] = '\000';
        temp_end_str++;
        temp_begin_str = temp_str;
        vel_range_begin = atoi(temp_begin_str);
        vel_range_end = atoi(temp_end_str);
        if (vel_range_begin < 0 || vel_range_begin > 127 ||
          vel_range_end < 0 || vel_range_end > 127)
            trap_exit("%s %d zone %d: invalid velRange", zt->keyn, k, j + 1);
      }

      snprintf(buffer, BUFLEN, "SF2.%s.%d.zone.%d.overridingRootKey",
        zt->key, k, j + 1);
      temp_str = txt->get_data(txt, buffer);
      if (temp_str == NULL) {
        overridingRootKey = -1;
      } else {
        iops_count++;
        overridingRootKey = atoi(temp_str);
      }

      snprintf(buffer, BUFLEN, "SF2.%s.%d.zone.%d.exclusiveClass",
        zt->key, k, j + 1);
      temp_str = txt->get_data(txt, buffer);
      if (temp_str == NULL) {
        exclusiveClass = -1;
      } else {
        iops_count++;
        exclusiveClass = atoi(temp_str);
      }

      if (zone_type == zt_instrument) {
        snprintf(buffer, BUFLEN, "SF2.%s.%d.loopstart", zt->item, item + 1);
        temp_str = txt->get_data(txt, buffer);
        if (temp_str == NULL) {
          loopstart = 0;
        } else {
          loopstart = atoi(temp_str);
        }
        snprintf(buffer, BUFLEN, "SF2.%s.%d.loopend", zt->item, item + 1);
        temp_str = txt->get_data(txt, buffer);
        if (temp_str == NULL) {
          loopend = 0;
        } else {
          loopend = atoi(temp_str);
        }
      }

      if (zone_type == zt_preset) {
        snprintf(buffer, BUFLEN, "SF2.%s.%d.bank", zt->key, k);
        temp_str = txt->get_data(txt, buffer);
        if (temp_str == NULL) {
          wBank = 0;
        } else {
          wBank = atoi(temp_str);
        }
      }

      snprintf(buffer, BUFLEN, "SF2.%s.%d.zone.%d.delayVolEnv", zt->key,
        k, j + 1);
      temp_str = txt->get_data(txt, buffer);
      if (temp_str == NULL) {
        env_delay = SHOOBVAL;
      } else {
        if (atof(temp_str) == 0.0) {
          env_delay = SHMIN;
        } else {
          env_delay = floor(1200.0 * (log(atof(temp_str)) / log(2)));
        }
        iops_count++;
      }

      snprintf(buffer, BUFLEN, "SF2.%s.%d.zone.%d.attackVolEnv", zt->key,
        k, j + 1);
      temp_str = txt->get_data(txt, buffer);
      if (temp_str == NULL) {
        env_attack = SHOOBVAL;
      } else {
        if (atof(temp_str) == 0.0) {
          env_attack = SHMIN;
        } else {
          env_attack = floor(1200.0 * (log(atof(temp_str)) / log(2)));
        }
        iops_count++;
      }

      snprintf(buffer, BUFLEN, "SF2.%s.%d.zone.%d.holdVolEnv", zt->key,
        k, j + 1);
      temp_str = txt->get_data(txt, buffer);
      if (temp_str == NULL) {
        env_hold = SHOOBVAL;
      } else {
        if (atof(temp_str) == 0.0) {
          env_hold = SHMIN;
        } else {
          env_hold = floor(1200.0 * (log(atof(temp_str)) / log(2)));
        }
        iops_count++;
      }

      snprintf(buffer, BUFLEN, "SF2.%s.%d.zone.%d.decayVolEnv", zt->key,
        k, j + 1);
      temp_str = txt->get_data(txt, buffer);
      if (temp_str == NULL) {
        env_decay = SHOOBVAL;
      } else {
        if (atof(temp_str) == 0.0) {
          env_decay = SHMIN;
        } else {
          env_decay = floor(1200.0 * (log(atof(temp_str)) / log(2)));
        }
        iops_count++;
      }

      snprintf(buffer, BUFLEN, "SF2.%s.%d.zone.%d.releaseVolEnv", zt->key,
       k, j + 1);
      temp_str = txt->get_data(txt, buffer);
      if (temp_str == NULL) {
        env_release = SHOOBVAL;
      } else {
        if (atof(temp_str) == 0.0) {
          env_release = SHMIN;
        } else {
          env_release = floor(1200.0 * (log(atof(temp_str)) / log(2)));
        }
        iops_count++;
      }

      snprintf(buffer, BUFLEN, "SF2.%s.%d.zone.%d.sustainVolEnv", zt->key,
        k, j + 1);
      temp_str = txt->get_data(txt, buffer);
      if (temp_str == NULL) {
        env_sustain = SHOOBVAL;
      } else {
        env_sustain = atoi(temp_str);
        iops_count++;
      }

      snprintf(buffer, BUFLEN, "SF2.%s.%d.zone.%d.keynumToVolEnvHold",
        zt->key, k, j + 1);
      temp_str = txt->get_data(txt, buffer);
      if (temp_str == NULL) {
        ktve_hold = SHOOBVAL;
      } else {
        ktve_hold = atoi(temp_str);
        iops_count++;
      }

      snprintf(buffer, BUFLEN, "SF2.%s.%d.zone.%d.keynumToVolEnvDecay",
        zt->key, k, j + 1);
      temp_str = txt->get_data(txt, buffer);
      if (temp_str == NULL) {
        ktve_decay = SHOOBVAL;
      } else {
        ktve_decay = atoi(temp_str);
        iops_count++;
      }

      gen->data = realloc(gen->data, (gen->count + iops_count + 1) * 4);
      p = gen->data + gen->count * 4;

      if (key_range_begin > -1 && key_range_end > -1) {
        sfGenOper = sfg_43_keyRange;
        genAmount.ranges.byLo = key_range_begin;
        genAmount.ranges.byHi = key_range_end;
        p = my_put_WORD_LE(&sfGenOper, p);
        p = my_put_BYTE(&genAmount.ranges.byLo, p);
        p = my_put_BYTE(&genAmount.ranges.byHi, p);
      }

      if (vel_range_begin > -1 && vel_range_end > -1) {
        sfGenOper = sfg_44_velRange;
        genAmount.ranges.byLo = vel_range_begin;
        genAmount.ranges.byHi = vel_range_end;
        p = my_put_WORD_LE(&sfGenOper, p);
        p = my_put_BYTE(&genAmount.ranges.byLo, p);
        p = my_put_BYTE(&genAmount.ranges.byHi, p);
      }

      if (overridingRootKey > -1) {
        sfGenOper = sfg_58_overridingRootKey;
        genAmount.wAmount = overridingRootKey;
        p = my_put_WORD_LE(&sfGenOper, p);
        p = my_put_WORD_LE(&genAmount.wAmount, p);
      }

      if (exclusiveClass > -1) {
        sfGenOper = sfg_57_exclusiveClass;
        genAmount.wAmount = exclusiveClass;
        p = my_put_WORD_LE(&sfGenOper, p);
        p = my_put_WORD_LE(&genAmount.wAmount, p);
      }

      if (zone_type == zt_instrument) {
        sfGenOper = sfg_54_sampleModes;
        if (loopstart != 0 || loopend != 0) {
          genAmount.wAmount = sml_loop_release;
        } else {
          genAmount.wAmount = sml_loop_none;
        }
        p = my_put_WORD_LE(&sfGenOper, p);
        p = my_put_WORD_LE(&genAmount.wAmount, p);
      }

      if (env_delay > SHOOBVAL) {
        sfGenOper = sfg_33_delayVolEnv;
        genAmount.shAmount = env_delay;
        p = my_put_WORD_LE(&sfGenOper, p);
        p = my_put_SHORT_LE(&genAmount.shAmount, p);
      }

      if (env_attack > SHOOBVAL) {
        sfGenOper = sfg_34_attackVolEnv;
        genAmount.shAmount = env_attack;
        p = my_put_WORD_LE(&sfGenOper, p);
        p = my_put_SHORT_LE(&genAmount.shAmount, p);
      }

      if (env_hold > SHOOBVAL) {
        sfGenOper = sfg_35_holdVolEnv;
        genAmount.shAmount = env_hold;
        p = my_put_WORD_LE(&sfGenOper, p);
        p = my_put_SHORT_LE(&genAmount.shAmount, p);
      }

      if (env_decay > SHOOBVAL) {
        sfGenOper = sfg_36_decayVolEnv;
        genAmount.shAmount = env_decay;
        p = my_put_WORD_LE(&sfGenOper, p);
        p = my_put_SHORT_LE(&genAmount.shAmount, p);
      }

      if (env_sustain > SHOOBVAL) {
        sfGenOper = sfg_37_sustainVolEnv;
        genAmount.shAmount = env_sustain;
        p = my_put_WORD_LE(&sfGenOper, p);
        p = my_put_SHORT_LE(&genAmount.shAmount, p);
      }

      if (env_release > SHOOBVAL) {
        sfGenOper = sfg_38_releaseVolEnv;
        genAmount.shAmount = env_release;
        p = my_put_WORD_LE(&sfGenOper, p);
        p = my_put_SHORT_LE(&genAmount.shAmount, p);
      }

      if (ktve_hold > SHOOBVAL) {
        sfGenOper = sfg_39_keynumToVolEnvHold;
        genAmount.shAmount = ktve_hold;
        p = my_put_WORD_LE(&sfGenOper, p);
        p = my_put_SHORT_LE(&genAmount.shAmount, p);
      }

      if (ktve_decay > SHOOBVAL) {
        sfGenOper = sfg_40_keynumToVolEnvDecay;
        genAmount.shAmount = ktve_decay;
        p = my_put_WORD_LE(&sfGenOper, p);
        p = my_put_SHORT_LE(&genAmount.shAmount, p);
      }

      sfGenOper = zt->oper;
      genAmount.wAmount = item;
      p = my_put_WORD_LE(&sfGenOper, p);
      p = my_put_WORD_LE(&genAmount.wAmount, p);

      bag->data = realloc(bag->data, (bag->count + 1) * 4);
      p = bag->data + bag->count * 4;
      wGenNdx = gen->count;
      wModNdx = mod->count;
      p = my_put_WORD_LE(&wGenNdx, p);
      p = my_put_WORD_LE(&wModNdx, p);

      gen->count += iops_count;
      bag->count++;

      j++;
      snprintf(buffer, BUFLEN, "SF2.%s.%d.zone.%d.%s", zt->key, k, j + 1,
        zt->item);
      n = txt->find_key(txt, buffer);
    }

    switch (zone_type) {
    case zt_instrument:
      hdr->data = realloc(hdr->data, (hdr->count + 1) * 22);
      p = hdr->data + hdr->count * 22;
      memset(name_zstr, 0, 20);
      strncpy(name_zstr, name, 19);
      wBagNdx = last_nbag;
      p = my_put_CHAR(name_zstr, p, 20);
      p = my_put_WORD_LE(&wBagNdx, p);
      break;
    case zt_preset:
      hdr->data = realloc(hdr->data, (hdr->count + 1) * 38);
      p = hdr->data + hdr->count * 38;
      memset(name_zstr, 0, 20);
      strncpy(name_zstr, name, 19);
      wPreset = hdr->count;
      wBagNdx = last_nbag;
      dwLibrary = 0;
      dwGenre = 0;
      dwMorphology = 0;
      p = my_put_CHAR(name_zstr, p, 20);
      p = my_put_WORD_LE(&wPreset, p);
      p = my_put_WORD_LE(&wBank, p);
      p = my_put_WORD_LE(&wBagNdx, p);
      p = my_put_DWORD_LE(&dwLibrary, p);
      p = my_put_DWORD_LE(&dwGenre, p);
      p = my_put_DWORD_LE(&dwMorphology, p);
      break;
    }

    hdr->count++;
    last_nbag = bag->count;

    k++;
    snprintf(buffer, BUFLEN, "SF2.%s.%d.name", zt->key, k);
    n = txt->find_key(txt, buffer);
  }
}

void write_hydra_chunks(FILE *ofp, chunk_t *RIFF, chunk_t *shdr, txt_t *txt)
{
  char name_zstr[20];
  void *p;
  myWORD sfGenOper;
  genAmountType genAmount;
  myWORD wGenNdx;
  myWORD wModNdx;
  myWORD wBagNdx;
  myWORD sfModSrcOper;
  myWORD sfModDestOper;
  mySHORT modAmount;
  myWORD sfModAmtSrcOper;
  myWORD sfModTransOper;
  myWORD wPreset;
  myWORD wBank;
  myDWORD dwLibrary;
  myDWORD dwGenre;
  myDWORD dwMorphology;

  chunk_t *LIST;
  chunk_t *phdr;
  chunk_t *pbag;
  chunk_t *pmod;
  chunk_t *pgen;
  chunk_t *inst;
  chunk_t *ibag;
  chunk_t *imod;
  chunk_t *igen;

  LIST = chunk_new(NULL, IN_RAM);
  strncpy(LIST->ckID.str, "LIST", 4);
  LIST->header_write(LIST, ofp);
  LIST->data_write(LIST, "pdta", 4, ofp);

  inst = chunk_new(NULL, IN_RAM);
  strncpy(inst->ckID.str, "inst", 4);
  ibag = chunk_new(NULL, IN_RAM);
  strncpy(ibag->ckID.str, "ibag", 4);
  igen = chunk_new(NULL, IN_RAM);
  strncpy(igen->ckID.str, "igen", 4);
  imod = chunk_new(NULL, IN_RAM);
  strncpy(imod->ckID.str, "imod", 4);
  pbag = chunk_new(NULL, IN_RAM);
  strncpy(pbag->ckID.str, "pbag", 4);
  pgen = chunk_new(NULL, IN_RAM);
  strncpy(pgen->ckID.str, "pgen", 4);
  pmod = chunk_new(NULL, IN_RAM);
  strncpy(pmod->ckID.str, "pmod", 4);
  phdr = chunk_new(NULL, IN_RAM);
  strncpy(phdr->ckID.str, "phdr", 4);

  write_zone(ofp, zt_instrument, igen, imod, ibag, inst, txt);
  write_zone(ofp, zt_preset, pgen, pmod, pbag, phdr, txt);

  /* terminal inst */
  inst->ckSize = (inst->count + 1) * 22;
  inst->data = realloc(inst->data, inst->ckSize);
  p = inst->data + inst->count * 22;
  memset(name_zstr, 0, 20);
  strcpy(name_zstr, "EOI");
  wBagNdx = ibag->count;
  p = my_put_CHAR(name_zstr, p, 20);
  p = my_put_WORD_LE(&wBagNdx, p);

  /* terminal ibag */
  ibag->ckSize = (ibag->count + 1) * 4;
  ibag->data = realloc(ibag->data, ibag->ckSize);
  p = ibag->data + ibag->count * 4;
  wGenNdx = igen->count;
  wModNdx = 0;
  p = my_put_WORD_LE(&wGenNdx, p);
  p = my_put_WORD_LE(&wModNdx, p);

  /* terminal igen */
  igen->ckSize = (igen->count + 1) * 4;
  igen->data = realloc(igen->data, igen->ckSize);
  p = igen->data + igen->count * 4;
  sfGenOper = 0;
  genAmount.wAmount = 0;
  p = my_put_WORD_LE(&sfGenOper, p);
  p = my_put_WORD_LE(&genAmount.wAmount, p);

  /* terminal imod */
  imod->ckSize = (imod->count + 1) * 10;
  imod->data = realloc(imod->data, imod->ckSize);
  p = imod->data + imod->count * 10;
  sfModSrcOper = 0;
  sfModDestOper = 0;
  modAmount = 0;
  sfModAmtSrcOper = 0;
  sfModTransOper = 0;
  p = my_put_WORD_LE(&sfModSrcOper, p);
  p = my_put_WORD_LE(&sfModDestOper, p);
  p = my_put_SHORT_LE(&modAmount, p);
  p = my_put_WORD_LE(&sfModAmtSrcOper, p);
  p = my_put_WORD_LE(&sfModTransOper, p);

  /* terminal pbag */
  pbag->ckSize = (pbag->count + 1) * 4;
  pbag->data = realloc(pbag->data, pbag->ckSize);
  p = pbag->data + pbag->count * 4;
  wGenNdx = pgen->count;
  wModNdx = pmod->count;
  p = my_put_WORD_LE(&wGenNdx, p);
  p = my_put_WORD_LE(&wModNdx, p);

  /* terminal pgen */
  pgen->ckSize = (pgen->count + 1) * 4;
  pgen->data = realloc(pgen->data, pgen->ckSize);
  p = pgen->data + pgen->count * 4;
  sfGenOper = 0;
  genAmount.wAmount = 0;
  p = my_put_WORD_LE(&sfGenOper, p);
  p = my_put_WORD_LE(&genAmount.wAmount, p);

  /* terminal pmod (specification does not require this) */
  pmod->ckSize = (pmod->count + 1) * 10;
  pmod->data = realloc(pmod->data, pmod->ckSize);
  p = pmod->data + pmod->count * 10;
  sfModSrcOper = 0;
  sfModDestOper = 0;
  modAmount = 0;
  sfModAmtSrcOper = 0;
  sfModTransOper = 0;
  p = my_put_WORD_LE(&sfModSrcOper, p);
  p = my_put_WORD_LE(&sfModDestOper, p);
  p = my_put_SHORT_LE(&modAmount, p);
  p = my_put_WORD_LE(&sfModAmtSrcOper, p);
  p = my_put_WORD_LE(&sfModTransOper, p);

  /* terminal phdr */
  phdr->ckSize = (phdr->count + 1) * 38;
  phdr->data = realloc(phdr->data, phdr->ckSize);
  p = phdr->data + phdr->count * 38;
  memset(name_zstr, 0, 20);
  strcpy(name_zstr, "EOP");
  wPreset = 0;
  wBank = 0;
  wBagNdx = pbag->count;
  dwLibrary = 0;
  dwGenre = 0;
  dwMorphology = 0;
  p = my_put_CHAR(name_zstr, p, 20);
  p = my_put_WORD_LE(&wPreset, p);
  p = my_put_WORD_LE(&wBank, p);
  p = my_put_WORD_LE(&wBagNdx, p);
  p = my_put_DWORD_LE(&dwLibrary, p);
  p = my_put_DWORD_LE(&dwGenre, p);
  p = my_put_DWORD_LE(&dwMorphology, p);

  LIST->ckSize += phdr->write(phdr, ofp);
  LIST->ckSize += pbag->write(pbag, ofp);
  LIST->ckSize += pmod->write(pmod, ofp);
  LIST->ckSize += pgen->write(pgen, ofp);
  LIST->ckSize += inst->write(inst, ofp);
  LIST->ckSize += ibag->write(ibag, ofp);
  LIST->ckSize += imod->write(imod, ofp);
  LIST->ckSize += igen->write(igen, ofp);
  LIST->ckSize += shdr->write(shdr, ofp);

  RIFF->ckSize += LIST->ckSize + 8;
  LIST->start_seek(LIST, ofp);
  LIST->header_write(LIST, ofp);
  LIST->end_seek(LIST, ofp);
  RIFF->ckSize += LIST->pad_write(LIST, ofp);
}

void write_info_chunks(FILE *ofp, chunk_t *RIFF, txt_t *txt)
{
  myWORD wMajor;
  myWORD wMinor;
  char *str;
  void *p;
  chunk_t *LIST;
  chunk_t *ifil;
  chunk_t *isng;
  chunk_t *INAM;
  chunk_t *ICRD;
  chunk_t *IENG;
  chunk_t *IPRD;
  chunk_t *ISFT;

  LIST = chunk_new(NULL, IN_RAM);
  strncpy(LIST->ckID.str, "LIST", 4);
  LIST->header_write(LIST, ofp);
  LIST->data_write(LIST, "INFO", 4, ofp);

  ifil = chunk_new(NULL, IN_RAM);
  strncpy(ifil->ckID.str, "ifil", 4);
  ifil->ckSize = 4;
  ifil->data = realloc(ifil->data, ifil->ckSize);
  p = ifil->data;
  wMajor = 2;
  wMinor = 1;
  p = my_put_WORD_LE(&wMajor, p);
  p = my_put_WORD_LE(&wMinor, p);
  LIST->ckSize += ifil->write(ifil, ofp);

  isng = chunk_new(NULL, IN_RAM);
  strncpy(isng->ckID.str, "isng", 4);
  str = txt->get_data(txt, "RIFF.LIST.isng.zstr");
  if (str == NULL)
    str = "EMU8000";
  if (strlen(str) > 255) {
    trap_warn("isng more than 256 bytes, truncating");
    str[255] = '\000';
  }
  isng->ckSize = strlen(str) + 1;
  if (!is_even(isng->ckSize))
    isng->ckSize++;
  isng->data = realloc(isng->data, isng->ckSize);
  memset(isng->data, 0, isng->ckSize);
  p = isng->data;
  p = my_put_CHAR(str, p, strlen(str));
  LIST->ckSize += isng->write(isng, ofp);

  INAM = chunk_new(NULL, IN_RAM);
  strncpy(INAM->ckID.str, "INAM", 4);
  str = txt->get_data(txt, "RIFF.LIST.INAM.zstr");
  if (str == NULL)
    str = "noname";
  if (strlen(str) > 255) {
    trap_warn("INAM more than 256 bytes, truncating");
    str[255] = '\000';
  }
  INAM->ckSize = strlen(str) + 1;
  if (!is_even(INAM->ckSize))
    INAM->ckSize++;
  INAM->data = realloc(INAM->data, INAM->ckSize);
  memset(INAM->data, 0, INAM->ckSize);
  p = INAM->data;
  p = my_put_CHAR(str, p, strlen(str));
  LIST->ckSize += INAM->write(INAM, ofp);

  ICRD = chunk_new(NULL, IN_RAM);
  strncpy(ICRD->ckID.str, "ICRD", 4);
  str = txt->get_data(txt, "RIFF.LIST.ICRD.zstr");
  if (str == NULL)
    str = get_ascii_date();
  if (strlen(str) > 255) {
    trap_warn("ICRD more than 256 bytes, truncating");
    str[255] = '\000';
  }
  ICRD->ckSize = strlen(str) + 1;
  if (!is_even(ICRD->ckSize))
    ICRD->ckSize++;
  ICRD->data = realloc(ICRD->data, ICRD->ckSize);
  memset(ICRD->data, 0, ICRD->ckSize);
  p = ICRD->data;
  p = my_put_CHAR(str, p, strlen(str));
  LIST->ckSize += ICRD->write(ICRD, ofp);

  IENG = NULL;
  str = txt->get_data(txt, "RIFF.LIST.IENG.zstr");
  if (str != NULL) {
    IENG = chunk_new(NULL, IN_RAM);
    strncpy(IENG->ckID.str, "IENG", 4);
    if (strlen(str) > 255) {
      trap_warn("IENG more than 256 bytes, truncating");
      str[255] = '\000';
    }
    IENG->ckSize = strlen(str) + 1;
    if (!is_even(IENG->ckSize))
      IENG->ckSize++;
    IENG->data = realloc(IENG->data, IENG->ckSize);
    memset(IENG->data, 0, IENG->ckSize);
    p = IENG->data;
    p = my_put_CHAR(str, p, strlen(str));
    LIST->ckSize += IENG->write(IENG, ofp);
  }

  IPRD = chunk_new(NULL, IN_RAM);
  strncpy(IPRD->ckID.str, "IPRD", 4);
  str = txt->get_data(txt, "RIFF.LIST.IPRD.zstr");
  if (str == NULL)
    str = "SBAWE32";
  if (strlen(str) > 255) {
    trap_warn("IPRD more than 256 bytes, truncating");
    str[255] = '\000';
  }
  IPRD->ckSize = strlen(str) + 1;
  if (!is_even(IPRD->ckSize))
    IPRD->ckSize++;
  IPRD->data = realloc(IPRD->data, IPRD->ckSize);
  memset(IPRD->data, 0, IPRD->ckSize);
  p = IPRD->data;
  p = my_put_CHAR(str, p, strlen(str));
  LIST->ckSize += IPRD->write(IPRD, ofp);

  ISFT = chunk_new(NULL, IN_RAM);
  strncpy(ISFT->ckID.str, "ISFT", 4);
  str = txt->get_data(txt, "RIFF.LIST.ISFT.zstr");
  if (str == NULL)
    str = "SFEDT v1.28:";
  if (strlen(str) > 255) {
    trap_warn("ISFT more than 256 bytes, truncating");
    str[255] = '\000';
  }
  ISFT->ckSize = strlen(str) + 1;
  if (!is_even(ISFT->ckSize))
    ISFT->ckSize++;
  ISFT->data = realloc(ISFT->data, ISFT->ckSize);
  memset(ISFT->data, 0, ISFT->ckSize);
  p = ISFT->data;
  p = my_put_CHAR(str, p, strlen(str));
  LIST->ckSize += ISFT->write(ISFT, ofp);

  RIFF->ckSize += LIST->ckSize + 8;
  LIST->start_seek(LIST, ofp);
  LIST->header_write(LIST, ofp);
  LIST->end_seek(LIST, ofp);
  RIFF->ckSize += LIST->pad_write(LIST, ofp);
}

void txt_to_sf(char *infile, char *outfile)
{
  FILE *ifp;
  FILE *ofp;
  chunk_t *RIFF;
  chunk_t *shdr;
  txt_t *txt;

  ifp = bens_fopen(infile, "r");
  ofp = bens_fopen(outfile, "w");
  txt = txt_new();
  txt->read_lines(txt, ifp);
  trap_txt = txt;

  RIFF = chunk_new(NULL, IN_RAM);
  strncpy(RIFF->ckID.str, "RIFF", 4);
  RIFF->header_write(RIFF, ofp);
  RIFF->data_write(RIFF, "sfbk", 4, ofp);

  write_info_chunks(ofp, RIFF, txt);

  shdr = chunk_new(NULL, IN_RAM);
  strncpy(shdr->ckID.str, "shdr", 4);
  write_sample_chunks(ofp, RIFF, shdr, txt);
  write_hydra_chunks(ofp, RIFF, shdr, txt);

  RIFF->start_seek(RIFF, ofp);
  RIFF->header_write(RIFF, ofp);
  RIFF->end_seek(RIFF, ofp);
  RIFF->pad_write(RIFF, ofp);

  bens_fclose(ifp);
  bens_fclose(ofp);
  txt->warn_unused(txt);
  txt->destroy(txt);
}
