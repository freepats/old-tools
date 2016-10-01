# pysf - Python SoundFont utility

## Intro

The pysf utility is a way to create and edit SoundFont 2 files
at the command line.  It does this using XML configuration
files and basic audio files (strict sub-set of .wav and .aif).

Each SoundFont2 file has one or more presets, each preset
containing one or more preset zones, each preset zone mapping
to an instrument, each instrument containing one or more
instrument zones, each instrument zone mapping to a wavetable.

The pysf utility is released into the public domain.  For
countries without a public domain, pysf is released under
the [WTF](href="http://sam.zoy.org/wtfpl/) public license.

My email address is collver@peak.org

Many terms will be defined in the SoundFont 2 Standard.

* http://freepats.opensrc.org/sf2/sfspec24.pdf

MPEG4-SA is more free, and includes much of the same information.

* http://sound.media.mit.edu/mpeg4/

The SoundFont 2 standard refers to the MIDI standard.

* http://www.ibiblio.org/emusic-l/info-docs-FAQs/MIDI-doc/
* http://www.borg.com/~jglatt/tech/midispec.htm


## Usage and Examples

    Usage: pysf.py [conversion] [infile] [outfile]
        conversion := --sf2xml | --xml2sf
        conversion := --aif2xml | --xml2aif
        conversion := --wav2xml | --xml2wav

Create conf.xml based on old.sf2.  Will also create conf%d.wav for
included audio samples

    pysf.py --sf2xml old.sf2 conf.xml

Create new.sf2 based on conf.xml

    pysf.py --xml2sf conf.xml new.sf2

Validate conf.xml using xmllint utility from libxml.

    xmllint --noout --schema pysf.xsd conf.xml


## Brief explaination of configuration tags

tag                | description
------------------ | --------------------------------------
ISNG               | mandatory, optimal target sound engine
INAM               | mandatory, name of sound bank
ICRD               | file creation date
IPRD               | target product
ISFT               | software used to create/modify the file
IFIL               | contains major and minor version of spec
major              | Major version of soundfont specfication used
minor              | Minor version of soundfont specfication used
instruments        | mandatory, instrument list
instrument         | mandatory, instrument section
presets            | mandatory, preset list
preset             | mandatory, preset section
wavetables         | mandatory, wavetable list
wavetable          | mandatory, wavetable section
zones              | mandatory, zone list for instrument or preset section
zone               | mandatory, zone section
id                 | mandatory, id for instrument or wavetable, referenced by other sections
name               | mandatory, descriptive name for instrument preset, or wavetable
keyRange           | range of MIDI notes for zone
velRange           | range of MIDI velocities for zone
overridingRootKey  | base MIDI note for zone
wavetableId        | wavetable used by instrument zone
instrumentId       | instrument used by preset zone
delayVolEnv        | time in seconds for delay section of volume envelope
attackVolEnv       | time in seconds for attack section of volume envelope
holdVolEnv         | time in seconds for hold section of volume envelope
decayVolEnv        | time in seconds for decay section of volume envelope
releaseVolEnv      | time in seconds for release section of volume envelope
sustainVolEnv      | volume decrease in centibels for decay section
keynumToVolEnvHold | degree, in timecents per KeyNumber units, to which the hold time is degreased.  The value of KeyNumber units is found by the difference between the MIDI note number and 60.
exclusiveClass     | new notes will silence currently playing notes if the instruments are in the same exclusiveClass
gens               | generic generator operator list in zone
gen                | generic generator operator
comment            | comment describing generator, not used by pysf
hexAmount          | hexadecimal unsigned word argument
oper               | decimal generator operator number
file               | mandatory, name of aif or wav file used by wavetable
loop               | loop begin and end point for wavetable
pitch              | MIDI note that the wavetable was recorded at
pitchcorr          | pitch correction in cents for wavetable
channel            | select channel for instrument and/or from stereo audio file
link               | link to other wavetable in a stereo instrument


## 24-bit sample width wavetables

To use 24-bit sample width wavetables, you must set ifil to major 2, minor 4.
If you use 24-bit wavetables, then all wavetables used must be 24-bit.
You can use audacity to convert between 24-bit and other sample widths.

Theoretically, old software should be able to play 24-bit soundfonts
at lower quality by simply ignoring the sm24 chunk.  However, some programs
will refuse to use 24-bit soundfonts.  Timidity and fluidsynth do not
support 24-bit soundfonts, and apparently neither does quicktime.
Therefore, this feature is not well tested in pysf.


## Stereo wavetables

To use a stereo .wav file for a mono wavetable, use the channel tag to
select a channel.  If the wavetable is linked, then the channel tag also
determines whether the wavetable is the left or the right channel in a
stereo instrument.  A stereo instrument requires two linked wavetables.
Use link tags to link wavetables.  Example:

    <wavetable>
      <channel>right</channel>
      <id>1</id>
      <file>stereo.wav</file>
      <link>2</link>
    </wavetable>
    <wavetable>
      <channel>left</channel>
      <id>2</id>
      <file>stereo.wav</file>
      <link>1</link>
    </wavetable>

A stereo instrument also requires a zone for each channel.  Example:

    <instrument>
      <zones>
        <zone>
          <keyRange><begin>12</begin><end>127</end></keyRange>
          <wavetableId>1</wavetableId>
        </zone>
        <zone>
          <keyRange><begin>12</begin><end>127</end></keyRange>
          <wavetableId>2</wavetableId>
        </zone>
      </zones>
    </instrument>


## History

* Version 1, 2007-Mar-4
  * added support for stereo wavetables
  * tested on Python 2.3.5 and Python 2.4.4
* Version 2, 2007-Mar-10
  * added IFIL configuration tag. for specifying soundfont 2.04
  * added support for 24-bit sample width
  * added version attribute to XML schema and output files
  * tested on Python 2.5
  * tested on big-endian system
  * using md5 instead of zlib.adler32 for portability reasons.
    http://sourceforge.net/tracker/?func=detail&atid=105470&aid=1678102&group_id=5470


## TODO

* add a gui front-end
