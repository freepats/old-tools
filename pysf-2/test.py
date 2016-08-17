#!/usr/bin/python
import logging, math, md5, os, os.path, shutil, sys, struct
import pysf

def ChecksumGenerate(FileName):
    Checksum = md5.new()
    BufferSize = 1024
    File = open(FileName, 'rb')
    while True:
        Buffer = File.read(BufferSize)
        if len(Buffer) == 0:
            break
        Checksum.update(Buffer)
    File.close()
    Retval = Checksum.hexdigest()
    return Retval

def ChecksumVerify(Source, FileName, Control):
    Checksum = ChecksumGenerate(FileName)
    if Checksum != Control:
        logging.error("%s:%s expected %s but see %s" % (Source, FileName,
            Control, Checksum))
        sys.exit(1)

def fpart(Number):
    return Number - math.floor(Number)

def SineGenerate(FileName, SampleFormat, Frequency=440):
    SampleRate = 44100.0
    SampleCount = 2205
    Amplitude = 10000
    SamplePeriod = SampleRate / Frequency
    File = open(FileName, 'wb')
    for I in range(0, SampleCount):
        S = int(math.sin(2 * math.pi * fpart(I / SamplePeriod)) * Amplitude)
        File.write(struct.pack(SampleFormat, (~S) & 0xFFFF))
    File.close()
        
def AudConfGenerate(FileName, RawFileName):
    File = open(FileName, 'wb')
    File.write("""<?xml version="1.0" ?>
""" + pysf.XmlHeaderStr + """
  <wav>
    <channels>1</channels>
    <file>""" + RawFileName + """</file>
    <sampleRate>44100</sampleRate>
    <sampleSize>16</sampleSize>
  </wav>
</sf:pysf>
""")
    File.close()

def SfConfGenerate(FileName):
    File = open(FileName, 'wb')
    File.write("""<?xml version="1.0" ?>
""" + pysf.XmlHeaderStr + """
  <sf2>
    <ICRD>Nov 08, 2005</ICRD>
    <IFIL>
      <major>2</major>
      <minor>1</minor>
    </IFIL>
    <INAM>sine bank</INAM>
    <IPRD>SBAWE32</IPRD>
    <ISFT>Compressed ImpulseTracker 2.14:mod2cs-0.2</ISFT>
    <ISNG>EMU8000</ISNG>
    <instruments>
      <instrument>
        <id>1</id>
        <name>stereo sine</name>
        <zones>
          <zone>
            <keyRange>
              <begin>12</begin>
              <end>126</end>
            </keyRange>
            <overridingRootKey>69</overridingRootKey>
            <wavetableId>1</wavetableId>
          </zone>
          <zone>
            <keyRange>
              <begin>12</begin>
              <end>126</end>
            </keyRange>
            <overridingRootKey>69</overridingRootKey>
            <wavetableId>2</wavetableId>
          </zone>
        </zones>
      </instrument>
      <instrument>
        <id>2</id>
        <name>mono sine</name>
        <zones>
          <zone>
            <keyRange>
              <begin>12</begin>
              <end>120</end>
            </keyRange>
            <overridingRootKey>69</overridingRootKey>
            <wavetableId>3</wavetableId>
          </zone>
          <zone>
            <keyRange>
              <begin>121</begin>
              <end>126</end>
            </keyRange>
            <overridingRootKey>69</overridingRootKey>
            <wavetableId>3</wavetableId>
          </zone>
        </zones>
      </instrument>
    </instruments>
    <presets>
      <preset>
        <bank>0</bank>
        <id>1</id>
        <name>sine preset</name>
        <zones>
          <zone>
            <instrumentId>1</instrumentId>
            <keyRange>
              <begin>0</begin>
              <end>127</end>
            </keyRange>
          </zone>
        </zones>
      </preset>
    </presets>
    <wavetables>
      <wavetable>
        <channel>right</channel>
        <file>test1.wav</file>
        <id>1</id>
        <link>2</link>
        <loop>
          <begin>100</begin>
          <end>200</end>
        </loop>
        <name>A4 sine right</name>
        <pitch>69</pitch>
      </wavetable>
      <wavetable>
        <channel>left</channel>
        <file>test2.wav</file>
        <id>2</id>
        <link>1</link>
        <loop>
          <begin>100</begin>
          <end>200</end>
        </loop>
        <name>A5 sine left</name>
        <pitch>69</pitch>
      </wavetable>
      <wavetable>
        <file>test3.wav</file>
        <id>3</id>
        <loop>
          <begin>100</begin>
          <end>200</end>
        </loop>
        <name>A4 sine mono</name>
        <pitch>69</pitch>
      </wavetable>
    </wavetables>
  </sf2>
</sf:pysf>
""")
    File.close()

def AudTest(Aud, Frequency, CsAud, CsConf, CsRaw):
    Base = os.path.splitext(Aud)[0]
    Raw = Base + '.raw'
    Xml = Base + '.xml'
    SineGenerate(Raw, '<H', Frequency)
    ChecksumVerify('Audtest 1', Raw, CsRaw)
    AudConfGenerate(Xml, Raw)
    ChecksumVerify('Audtest 2', Xml, CsConf) 
    pysf.XmlToAud(Xml, Aud, 'wav')
    ChecksumVerify('Audtest 3', Aud, CsAud)
    os.remove(Xml)
    os.remove(Raw)
    pysf.AudToXml(Aud, Xml, 'wav')
    ChecksumVerify('Audtest 4', Xml, CsConf)
    ChecksumVerify('Audtest 5', Raw, CsRaw)

def SfTest(Sf2, CsSf, CsConf, CsA4Wav, CsA5Wav):
    Xml = 'test.xml'
    Sf2 = 'test.sf2'
    SfConfGenerate(Xml)
    ChecksumVerify('SfTest 1', Xml, CsConf)
    pysf.XmlToSf(Xml, Sf2)
    ChecksumVerify('SfTest 2', Sf2, CsSf)
    os.remove(Xml)
    os.remove('test1.wav')
    os.remove('test2.wav')
    pysf.SfToXml(Sf2, Xml)
    ChecksumVerify('SfTest 3', 'test1.wav', CsA4Wav)
    ChecksumVerify('SfTest 4', 'test2.wav', CsA5Wav)
    ChecksumVerify('SfTest 5', Xml, CsConf)
    os.remove(Sf2)
    pysf.XmlToSf(Xml, Sf2)
    ChecksumVerify('SfTest 6', Sf2, CsSf)


os.chdir('tmp')
AudTest('test1.wav', 440, 'f3f1fd78ee2b4476ed03b90b56aeff16',
  'cbf1dd8542ee1c8af63fdf1d3c851d9a', '5f2d52ecf007dde81a28197bde8fdc2e')
AudTest('test2.wav', 880, '960702e0641e17cef647f41efc0e714f',
  '697a61dee0e982a1d138ab38b3109dcd', 'b9a8c77bc409787188923f9785f1ff24')
shutil.copyfile('test1.wav', 'test3.wav')
SfTest('test.sf2', 'c5da17317905c65c4d6cc78f6ca62903',
  'aa7446da6fe34fce65b6239380bcb523', 'f3f1fd78ee2b4476ed03b90b56aeff16',
  '960702e0641e17cef647f41efc0e714f')
os.chdir('..')
