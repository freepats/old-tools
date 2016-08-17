#!/bin/sh

program_seek()
{
  program="$1"
  fatal="$2"
  type $program >/dev/null 2>&1
  retval=$?
  if [ $retval -ne 0 ]
  then
    if [ $fatal -ne 0 ]
    then
      echo ERROR: could not find $program
      echo Test failed.
      exit
    else
      echo WARNING: $program not found, a test will be skipped.
    fi
  fi
  return $retval
}

checksum_verify()
{
  source="$1"
  filename="$2"
  control="$3"

  checksum=$(cksum <"$filename")
  if [ "$checksum" != "$control" ]
  then
    echo ERROR: $source generated unexpected output in $filename
    echo Test failed.
    exit
  fi
}

raw_create()
{
  program_seek ./gensine 1
  if [ $? -eq 0 ]
  then
    ./gensine >tmp/raw.raw
  fi
  checksum_verify raw_create tmp/raw.raw "3233638796 4410"
}

aif_conf_create()
{
  cat >tmp/aif.txt <<__EOF__
FORM.FVER.timestamp=2726318400
FORM.COMM.numChannels=1
FORM.COMM.sampleSize=16
FORM.COMM.sampleRate=44100
FORM.COMM.compressionType.str=NONE
FORM.COMM.compressionName.str=not compressed
FORM.MARK.1.id=91
FORM.MARK.1.pos=100
FORM.MARK.1.name=start sustain loop
FORM.MARK.2.id=92
FORM.MARK.2.pos=200
FORM.MARK.2.name=end sustain loop
FORM.INST.baseNote=69
FORM.INST.detune=0
FORM.INST.lowNote=0
FORM.INST.highNote=127
FORM.INST.lowVelocity=1
FORM.INST.highVelocity=127
FORM.INST.gain=0
FORM.INST.sustainLoop.playMode=1
FORM.INST.sustainLoop.beginLoop=91
FORM.INST.sustainLoop.endLoop=92
FORM.INST.releaseLoop.playMode=0
FORM.INST.releaseLoop.beginLoop=0
FORM.INST.releaseLoop.endLoop=0
FORM.SSND.file=tmp/aif.raw
FORM.SSND.byteswap=1
__EOF__
  checksum_verify aif_conf_create tmp/aif.txt "1417919088 732"
}

wav_conf_create()
{
  cat >tmp/wav.txt <<__EOF__
RIFF.fmt.wFormatTag=1
RIFF.fmt.wChannels=1
RIFF.fmt.dwSamplesPerSec=44100
RIFF.fmt.dwAvgBytesPerSec=88200
RIFF.fmt.wBlockAlign=2
RIFF.fmt.wBitsPerSample=16
RIFF.data.size=4410
RIFF.data.file=tmp/wav.raw
__EOF__
  checksum_verify wav_conf_create tmp/wav.txt "873361169 203"
}

wav_create()
{
  cp tmp/raw.raw tmp/wav.raw
  wav_conf_create
  ./sfubar --txt2wav tmp/wav.txt tmp/wav.wav
  checksum_verify wav_create tmp/wav.wav "656389750 4454"
  mv tmp/wav.txt tmp/wav.txt.orig
  ./sfubar --wav2txt tmp/wav.wav tmp/wav.txt
  checksum_verify sfubar_wav tmp/wav.txt "873361169 203"
  checksum_verify sfubar_wav tmp/wav.raw "3233638796 4410"
}

aif_create()
{
  cp tmp/raw.raw tmp/aif.raw
  aif_conf_create
  ./sfubar --txt2aif tmp/aif.txt tmp/aif.aif
  checksum_verify aif_create tmp/aif.aif "1957549582 4584"
  mv tmp/aif.txt tmp/aif.txt.orig
  ./sfubar --aif2txt tmp/aif.aif tmp/aif.txt
  checksum_verify aif_create tmp/aif.txt "4209874723 711"
  checksum_verify aif_create tmp/aif.raw "2290835127 4410"
}

sfubar_conf_cat()
{
  filename="$1"
  cat <<__EOF__
RIFF.LIST.isng.zstr=EMU8000
RIFF.LIST.INAM.zstr=sine bank
RIFF.LIST.ICRD.zstr=Nov 08, 2005
RIFF.LIST.IPRD.zstr=SBAWE32
RIFF.LIST.ISFT.zstr=Compressed ImpulseTracker 2.14:mod2cs-0.2
SF2.wt.1.$filename
SF2.wt.1.name=sine wavetable
SF2.wt.1.loopstart=100
SF2.wt.1.loopend=200
SF2.wt.1.pitch=69
SF2.in.1.name=sine instrument
SF2.in.1.zone.1.keyRange=12,120
SF2.in.1.zone.1.overridingRootKey=69
SF2.in.1.zone.1.wt=1
SF2.in.1.zone.2.keyRange=121,126
SF2.in.1.zone.2.overridingRootKey=69
SF2.in.1.zone.2.wt=1
SF2.in.2.name=sine instrument 2
SF2.in.2.zone.1.keyRange=12,126
SF2.in.2.zone.1.overridingRootKey=69
SF2.in.2.zone.1.wt=1
SF2.ps.1.name=sine preset
SF2.ps.1.bank=0
SF2.ps.1.zone.1.keyRange=0,127
SF2.ps.1.zone.1.in=1
__EOF__
}

timidity_conf_create()
{
  cat >tmp/test.cfg <<__EOF__
dir .
bank 40
soundfont tmp/test.sf2
__EOF__
  checksum_verify timidity_conf_create tmp/test.cfg "3204808837 37"
}

abc_score_create()
{
  cat >tmp/test.abc <<__EOF__
%%abc

X:1 
T:test tune
C:Trad
Z:sfubar
M:4/4
L:1/4
K:C
%%MIDI program 0
| D/>D/D E/D//D// C | D/>D/ F/G/ A2 |
__EOF__
    checksum_verify abc_score_create tmp/test.abc "2596554308 111"
}

csound_csd_create()
{
  cat >tmp/test.csd <<__EOF__
<CsoundSynthesizer>
<CsOptions>
</CsOptions>
<CsInstruments>
sr = 44100
kr = 4410
ksmps = 10
nchnls = 1
gifont sfload "tmp/test.sf2"
instr 1 
  inote notnum
  kpitch cpsmidib
  iamp ampmidi .5

  iinstr = 0
  ioffset = 0
  ivel = 0
  ixamp = 1
  iflag = 1
  asig sfinstrm ivel, inote, ixamp, kpitch, iinstr, gifont, iflag, ioffset
  out asig * iamp
endin
</CsInstruments>
<CsScore>
s
f0 4
e
</CsScore>
</CsoundSynthesizer>
__EOF__
  checksum_verify csound_csd_create tmp/test.csd "2914206408 423"
}

sfubar_sf_test()
{
  echo Testing sfubar soundfont code

  raw_create

  wav_create
  cp tmp/wav.wav tmp/test1.wav
  sfubar_conf_cat wav=tmp/test1.wav >tmp/test.txt
  checksum_verify sfubar_sf_wav tmp/test.txt "3532180799 726"
  ./sfubar --txt2sf tmp/test.txt tmp/test.sf2
  checksum_verify sfubar_sf_wav tmp/test.sf2 "1086717969 5106"
  mv tmp/test.txt tmp/test.txt.orig
  ./sfubar --sf2txt tmp/test.sf2 tmp/test.txt
  checksum_verify sfubar_sf_wav2 tmp/test1.wav "656389750 4454"
  checksum_verify sfubar_sf_wav2 tmp/test.txt "3532180799 726"

  aif_create
  sfubar_conf_cat aif=tmp/aif.aif >tmp/test3.txt
  checksum_verify sfubar_sf_aif tmp/test3.txt "2753237313 724"
  ./sfubar --txt2sf tmp/test3.txt tmp/test3.sf2
  checksum_verify sfubar_sf_aif tmp/test3.sf2 "1086717969 5106"
  ./sfubar --sf2txt tmp/test3.sf2 tmp/test.txt
  checksum_verify sfubar_sf_aif2 tmp/test1.wav "656389750 4454"
  checksum_verify sfubar_sf_wav2 tmp/test.txt "3532180799 726"
}

midi_test()
{
  echo Testing example SoundFont 2 file with timidity
  program_seek abc2midi 0
  if [ $? -ne 0 ]
  then
    cp test-success/test.mid tmp/test.mid
    checksum_verify script tmp/test.mid "1881877690 181"
  else
    abc_score_create
    abc2midi tmp/test.abc -o tmp/test.mid >/dev/null 2>&1
    checksum_verify abc2midi tmp/test.mid "1881877690 181"
  fi

  program_seek timidity 0
  if [ $? -ne 0 ]
  then
    return
  fi
  timidity_conf_create
  timidity -L . -c tmp/test.cfg --force-bank=40 -A440 -OwMs1l -EFns=0 \
    -EFresamp=d -o tmp/song1.wav tmp/test.mid >/dev/null 2>&1
  checksum_verify timidity tmp/song1.wav "2854065874 358580"
}

csound_test()
{
  echo Testing example SoundFont 2 file with csound
  program_seek csound 0
  if [ $? -ne 0 ]
  then
    return
  fi
  csound_csd_create
  csound -W -K -o tmp/song2.wav -F tmp/test.mid tmp/test.csd >/dev/null 2>&1
  checksum_verify csound tmp/song2.wav "3837075942 352844"
}

ieee80_test()
{
  uname -s | grep CYGWIN_95 >/dev/null 2>&1
  if [ $? -eq 0 ]
  then
    echo Windows 95 has printf format bugs.
    echo Skipping IEEE80 conversion routine test.
    return
  fi
  echo Testing IEEE80 conversion routines
  program_seek ./bin/ieee80-test 1
  ./bin/ieee80-test >tmp/ieee80.tst
  checksum_verify ieee80-test tmp/ieee80.tst "3195791153 770"
}

rm -f tmp/*
program_seek ./sfubar 1
ieee80_test
sfubar_sf_test
midi_test
#csound_test
echo Test passed.
