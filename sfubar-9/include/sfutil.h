#ifndef __SFUTIL_H__
#define __SFUTIL_H__
#include <riffraff/rifftypes.h>
extern void sf_to_txt(char *, char *, int);
extern void txt_to_sf(char *, char *);
#define SFUTIL_VERSION 9

#define SHMIN -32768
#define SHOOBVAL -32769

typedef enum {
  file_type_wav,
  file_type_aif,
  file_type_end
} file_type_t;

typedef struct {
  myBYTE byLo;
  myBYTE byHi;
} rangesType;

typedef union {
  rangesType ranges;
  mySHORT shAmount;
  myWORD wAmount;
} genAmountType;

typedef enum {
  sfg_0_startAddrsOffset = 0,
  sfg_1_endAddrsOffset,
  sfg_2_startloopAddrsOffset,
  sfg_3_endloopAddrsOffset,
  sfg_4_startAddrsCoarseOffset,
  sfg_5_modLfoToPitch,
  sfg_6_vibLfoToPitchi,
  sfg_7_modEnvToPitch,
  sfg_8_initialFilterFc,
  sfg_9_initialFilterQ,
  sfg_10_modLfoToFilterFc,
  sfg_11_modEnvToFilterFc,
  sfg_12_endAddrsCoarseOffset,
  sfg_13_modLfoToVolume,
  sfg_14_unused,
  sfg_15_chorusEffectsSend,
  sfg_16_reverbEffectsSend,
  sfg_17_pan,
  sfg_18_unused,
  sfg_19_unused,
  sfg_20_unused,
  sfg_21_delayModLFO,
  sfg_22_freqModLFO,
  sfg_23_delayVibLFO,
  sfg_24_freqVibLFO,
  sfg_25_delayModEnv,
  sfg_26_attackModEnv,
  sfg_27_holdModEnv,
  sfg_28_decayModEnv,
  sfg_29_sustainModEnv,
  sfg_30_releaseModEnv,
  sfg_31_keynumToModEnvHold,
  sfg_32_keynumToModEnvDecay,
  sfg_33_delayVolEnv,
  sfg_34_attackVolEnv,
  sfg_35_holdVolEnv,
  sfg_36_decayVolEnv,
  sfg_37_sustainVolEnv,
  sfg_38_releaseVolEnv,
  sfg_39_keynumToVolEnvHold,
  sfg_40_keynumToVolEnvDecay,
  sfg_41_instrument,
  sfg_42_reserved,
  sfg_43_keyRange,
  sfg_44_velRange,
  sfg_45_startloopAddrsCoarseOffset,
  sfg_46_keynum,
  sfg_47_velocity,
  sfg_48_initialAttenuation,
  sfg_49_reserved,
  sfg_50_endloopAddrsCoarseOffset,
  sfg_51_coarseTune,
  sfg_52_fineTune,
  sfg_53_sampleID,
  sfg_54_sampleModes,
  sfg_55_reserved,
  sfg_56_scaleTuning,
  sfg_57_exclusiveClass,
  sfg_58_overridingRootKey,
  sfg_59_unused,
  sfg_60_endOper
} sf_gen_enum_t;

typedef enum {
  sml_loop_none = 0,
  sml_loop_continuous,
  sml_loop_unused,
  sml_loop_release
} sample_modes_enum_t;

typedef enum {
  zt_instrument = 0,
  zt_preset
} zone_type_enum_t;

typedef struct {
  zone_type_enum_t zone_type;
  char *keyn;
  char *key;
  char *itemn;
  char *item;
  int item_max;
  int oper;
  char *bagc;
  char *genc;
  char *modc;
  char *hdrc;
} zone_type_t;
#endif
