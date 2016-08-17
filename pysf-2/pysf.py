#!/usr/bin/python
import aifc, array, chunk, datetime, logging, math, os, os.path
import struct, sys, tempfile, wave, xml.dom.minidom

class SfChunkReader(chunk.Chunk):
    Item = 0
    Form = 'NONE'

    def __init__(self, Handle):
        chunk.Chunk.__init__(self, Handle, True, False, False)
        if self.getsize() > 3:
            self.seek(0)
            self.Form = self.read(4)
            self.seek(0)

    def HeaderTell(self):
        return self.offset - 8

    def DataRead(self):
        self.seek(0)
        return self.read()

    def FormSkip(self):
        self.seek(4)

    def IsEnd(self):
        Retval = False
        if self.tell() >= self.getsize():
            Retval = True
        return Retval

    def SubChunk(self):
        Pos = self.tell()
        Retval = SfChunkReader(self.file)
        Size = Retval.getsize()
        self.seek(Pos + 8 + Size)
        return Retval

class SfTreeItem:
    Level = None
    CkId = None
    Form = None
    Chunk = None

    def __init__(self, Level, CkId, Form, Chunk):
        (self.Level, self.CkId, self.Form, self.Chunk) = (Level, CkId,
            Form, Chunk)

    def ChunkAssign(self, Chunk):
        self.Chunk = Chunk

class SfTree:
    Prefix = None
    Items = None
    Containers = None
    Parent = None
    Out = None
    WtPrefix = None

    def __init__(self, Items, Containers, Parent, Out, WtPrefix):
        self.Prefix = None
        self.Items = Items
        self.Containers = Containers
        self.Parent = Parent
        self.Out = Out
        self.WtPrefix = WtPrefix

    def ChunkFind(self, Chunk, Level):
        Retval = 'Not Found'
        for Item in self.Items:
            if Level != Item.Level:
                continue
            if Chunk.getname() != Item.CkId:
                continue
            if Item.Form != None and Chunk.Form != Item.Form:
                continue
            if Item.Chunk != None:
                Retval = 'Duplicate'
                break
            Chunk.Item = Item
            Item.ChunkAssign(Chunk)
            Retval = 'Found'
            break
        return Retval

    def ChunkIsContainer(self, Chunk):
        Retval = ListHas(self.Containers, Chunk.getname())
        if Retval:
            Chunk.FormSkip()
        return Retval

    def CkId(self, CkId, Form, Level):
        Retval = None
        for Item in self.Items:
            if Item.Chunk == None:
                continue
            if Level != -1 and Level != Item.Level:
                continue
            if CkId != None and CkId != Item.CkId:
                continue
            if Form != None and Form != Item.Form:  continue
            Retval = Item
            break
        return Retval

    def CkIdStr(self, CkId, Form, Level):
        Retval = None
        Item = self.CkId(CkId, Form, Level)
        if Item != None:
            Retval = Item.Chunk.DataRead().split('\0', 1)[0]
        return Retval

    def Read(self, Chunk, Level):
        Found = self.ChunkFind(Chunk, Level)
        Fpos = Chunk.HeaderTell()
        CkId = Chunk.getname()
        Form = Chunk.Form
        PFmtStr = "Ck %s, pos %ld, id %c%c%c%c, frm %c%c%c%c, lvl %d, len %d"
        logging.info(PFmtStr % (Found, Fpos,
            CkId[0], CkId[1], CkId[2], CkId[3],
            Form[0], Form[1], Form[2], Form[3], Level, Chunk.getsize()))
        if Found != 'Found':
            logging.warn("Chunk %s" % (Found))
            Chunk.close()
        else:
            if self.ChunkIsContainer(Chunk) == True:
                while Chunk.IsEnd() == False:
                    SubChunk = Chunk.SubChunk()
                    self.Read(SubChunk, Level + 1)

class SfZoneType:
    KeyN = None
    ItemN = None
    ItemMax = None
    Oper = None
    Bag = None
    Gen = None
    Mod = None
    Hdr = None

    def __init__(self, ZoneTypeStr):
        if ZoneTypeStr == 'instrument':
            self.KeyN = 'instrument'
            self.ItemN = 'wavetable'
            self.Oper = 53
            self.Bag = 'ibag'
            self.Gen = 'igen'
            self.Mod = 'imod'
            self.Hdr = 'inst'
        elif ZoneTypeStr == 'preset':
            self.KeyN = 'preset'
            self.ItemN = 'instrument'
            self.Oper = 41
            self.Bag = 'pbag'
            self.Gen = 'pgen'
            self.Mod = 'pmod'
            self.Hdr = 'phdr'
        else:
            raise ValueError

def PrintUsage():
    UsageStr = 'pysf version ' + str(PysfVersion) + '\n\n\n' + """
Usage: pysf [conversion] [infile] [outfile]
    conversion := --sf2xml | --xml2sf
    conversion := --aif2xml | --xml2aif
    conversion := --wav2xml | --xml2wav
"""
    print UsageStr
    sys.exit(0)

def ustr(Arg):
    return unicode(str(Arg), 'utf-8')

def LogDie(Msg):
    logging.error(Msg)
    sys.exit(1)

def DateAsciiGet():
    Retval = datetime.date.today().strftime("%b %d, %Y")
    return Retval

def Def(Variable, Default):
    if Variable == None:
        Variable = Default
    return Variable

def Val(Dict, Key):
    if Dict == None:
        Retval = None
    elif ListHas(Dict.keys(), Key):
        Retval = Dict[Key]
    else:
        Retval = None
    return Retval

def ListHas(List, Item):
    return len(filter(lambda x: x == Item, List)) > 0

def LdFind(List, Key, Value):
    Retval = None
    Results = filter(lambda x: x[Key] == Value, List)
    if len(Results) > 0:
        Retval = Results[0]
    return Retval

def DataSwap(DataString):
    DataArray = array.array('H', DataString)
    DataArray.byteswap()
    Retval = DataArray.tostring()
    return Retval

def ChannelFilter(DataString, Channel):
    Retval = ''
    while len(DataString) > 0:
        Retval = Retval + DataString[Channel * SrcWidth:SrcWidth]
        DataString = DataString[SrcWidth * 2:]
    return Retval

def DataSplit24(DataString, SplitPart):
    Retval = ''
    if SplitPart == 'part16':
        (RangeBegin, RangeEnd) = (1, 3)
    elif SplitPart == 'part24':
        (RangeBegin, RangeEnd) = (0, 1)
    while len(DataString) > 0:
        Retval = Retval + DataString[RangeBegin:RangeEnd]
        DataString = DataString[3:]
    return Retval

def DataJoin24(Data16, Data24):
    # This is little-endian because we always export as wave
    Retval = ''
    while len(Data16) > 0:
        Retval = Retval + Data24[0:1] + Data16[0:2]
        Data16 = Data16[2:]
        Data24 = Data24[1:]
    return Retval

def DataCopy(Src, Dst, SrcWidth, FramesLeft, Byteswap = None, \
    Channel = -1, SplitPart = 'all'):
    if Byteswap == None:
       Byteswap = False
       if sys.byteorder == 'big':
            Byteswap = True
    S24 = None
    if type(Src) == tuple:
        (Src, S24) = Src
    if Src.__class__ == wave.Wave_read or Src.__class__ == aifc.Aifc_read:
        ReadFunc = Src.readframes
        SrcWidth = 1
    else:
        ReadFunc = Src.read
    if Dst.__class__ == wave.Wave_write or Dst.__class__ == aifc.Aifc_write:
        WriteFunc = Dst.writeframesraw
    else:
        WriteFunc = Dst.write
    while FramesLeft > 0:
        DataSize = min(FramesLeft, 1024)
        DataString = ReadFunc(DataSize * SrcWidth)
        if Byteswap == True:
            DataString = DataSwap(DataString)
        if Channel == 0 or Channel == 1:
            DataString = ChannelFilter(DataString, Channel)
        if SplitPart != 'all':
            DataString = DataSplit24(DataString, SplitPart)
        if S24 != None:
            Data24 = S24.read(DataSize)
            DataString = DataJoin24(DataString, Data24)
        WriteFunc(DataString)
        FramesLeft = FramesLeft - DataSize

def DictToXml(Xml, XmlEl, Dict):
    KeyList = Dict.keys()
    KeyList.sort()
    for Key in KeyList:
        if type(Dict[Key]) == dict:
            XmlSubEl = Xml.createElementNS(None, Key)
            XmlEl.appendChild(XmlSubEl)
            DictToXml(Xml, XmlSubEl, Dict[Key])
        elif type(Dict[Key]) == list:
            for SubDict in Dict[Key]:
                XmlSubEl = Xml.createElementNS(None, Key)
                XmlEl.appendChild(XmlSubEl)
                DictToXml(Xml, XmlSubEl, SubDict)
        else:
            XmlSubEl = Xml.createElementNS(None, Key)
            XmlEl.appendChild(XmlSubEl)
            XmlTextEl = Xml.createTextNode(ustr(Dict[Key]))
            XmlSubEl.appendChild(XmlTextEl)

def DictToXmlStr(Dict):
    Xml = xml.dom.minidom.parseString(XmlRootStr.encode('UTF-8'))
    XmlEl = Xml.documentElement
    DictToXml(Xml, XmlEl, Dict)
    R = ''
    for L in Xml.toprettyxml('  ').split('\n'):
        if len(L) > 0 and len(R) > 0 and L[-1] == '>' and R[-1] == '>':
            R = R + '\n' + L
        else:
            R = R + L.strip()
    R = R + '\n'
    Xml.unlink()
    return R

def XmlToDict(Xml):
    CTags = ['gen', 'instrument', 'preset', 'wavetable', 'zone']
    Dict = {}
    for Node in Xml.childNodes:
        if Node.nodeType == Node.ELEMENT_NODE:
            NewDict = XmlToDict(Node)
            if ListHas(CTags, Node.nodeName):
                if ListHas(Dict.keys(), Node.nodeName):
                    Dict[Node.nodeName].append(NewDict)
                else:
                    Dict[Node.nodeName] = [NewDict]
            else:
                Dict[Node.nodeName] = NewDict
        elif Node.nodeType == Node.TEXT_NODE:
            Str = Node.data.strip()
            if len(Str) > 0:
                if Str.isdigit():
                    Dict = int(Str)
                elif Str[0:2] == '0x':
                    Dict = int(Str[2:], 16)
                else:
                    Dict = Str
        else:
            raise TypeError
    return Dict

def XmlFileToDict(FileName):
    Xml = xml.dom.minidom.parse(FileName)
    Retval = XmlToDict(Xml)
    Xml.unlink()
    return Retval

def LikeFile(Obj):
    Retval = False
    if type(Obj) == file or Obj.__class__ == tempfile._TemporaryFileWrapper:
        Retval = True
    return Retval

def ListToIff(List, OutHandle):
    while len(List) > 0:
        ChunkPos = OutHandle.tell()
        (Key, Data) = List[0:2]
        List = List[2:]
        if type(Key) == list:
            (Id, Form) = Key
            FormData = struct.pack('4s', Form)
        else:
            (Id, Form) = (Key, None)
            FormData = ''
        OutHandle.write(struct.pack('<4sI', Id, 0))
        DataPos = OutHandle.tell()
        OutHandle.write(FormData)
        if LikeFile(Data):
            Data.seek(0, 2)
            FramesLeft = Data.tell() / 2
            Data.seek(0)
            DataCopy(Data, OutHandle, 2, FramesLeft, False)
            Data.close()
        elif type(Data) == str:
            OutHandle.write(Data)
        elif type(Data) == list:
            ListToIff(Data, OutHandle)
        else:
            raise TypeError
        Pos = OutHandle.tell()
        ChunkSize = Pos - DataPos
        if ChunkSize % 2 > 0:
            raise ValueError
        OutHandle.seek(ChunkPos + 4)
        OutHandle.write(struct.pack('<I', ChunkSize))
        OutHandle.seek(Pos)

def AudOpen(FileName, Mode, Format):
    if Format == 'wav':
        AudOpenFunc = wave.open
    elif Format == 'aif':
        AudOpenFunc = aifc.open
    else:
        LogDie('unsupported format')
    return AudOpenFunc(FileName, Mode)

def AudToXml(Src, Dst, Format):
    RawFile = os.path.splitext(Dst)[0] + '.raw'
    Aud = AudOpen(Src, 'rb', Format)
    Xml = open(Dst, 'wb')
    Dict = { ustr(Format): { u'channels': Aud.getnchannels(), 
         u'sampleSize': Aud.getsampwidth() * 8,
         u'sampleRate': Aud.getframerate(), u'file': RawFile } }
    Xml.write(DictToXmlStr(Dict))
    Raw = open(RawFile, 'wb')
    DataCopy(Aud, Raw, Aud.getsampwidth(), Aud.getnframes())
    Raw.close()
    Aud.close()

def XmlToAud(Src, Dst, Format):
    Aud = AudOpen(Dst, 'wb', Format)
    try:
        Dict = XmlFileToDict(Src)[u'sf:pysf'][ustr(Format)]
    except KeyError:
        LogDie('Invalid input format.')
    Channels = Def(Val(Dict, u'channels'), 0)
    FileName = Def(Val(Dict, u'file'), 'noname.wav')
    SampleSize = Def(Val(Dict, u'sampleSize'), 0)
    SampleRate = Def(Val(Dict, u'sampleRate'), 44100)
    Byteswap = Val(Dict, u'byteswap')
    if Byteswap != None:
        Byteswap = Byteswap == 1
    if Channels < 1:
        LogDie('unsupported number of channels')
    if SampleSize < 1 or SampleSize % 8 != 0:
        LogDie('unsupported sampleSize')
    SampleSizeBytes = SampleSize / 8
    Raw = open(FileName, 'rb')
    Raw.seek(0, 2)
    FileSize = Raw.tell()
    if FileSize < 1 or FileSize % SampleSizeBytes != 0:
        LogDie("unsupported raw data size %d" % (FileSize))
    Raw.seek(0, 0)
    NumSampleFrames = FileSize / SampleSizeBytes
    Aud.setnchannels(Channels)
    Aud.setsampwidth(SampleSize / 8)
    Aud.setframerate(SampleRate)
    Aud.setnframes(NumSampleFrames)
    DataCopy(Raw, Aud, SampleSize / 8, NumSampleFrames, Byteswap)
    Aud.close()
    Raw.close()

def SfStr(Str, MaxLen = 256):
    if Str == None:
        Retval = None
    else:
        StrLen = len(Str) + 1
        if StrLen % 2 > 0:
            StrLen = StrLen + 1
        if StrLen > MaxLen:
            NewStr = Str[0:MaxLen]
            logging.warn("truncating string\nOLD: %s\nNEW: %s" % (Str, NewStr))
            (Str, StrLen) = (NewStr, MaxLen)
        FmtStr = "%ds" % (StrLen)
        Retval = struct.pack(FmtStr, str(Str))
    return Retval

def SfWavetableList(Tree):
    Smpl = Tree.CkId('smpl', None, -1)
    if Smpl == None:
        LogDie('no wavetable data')
    Ifil = Tree.CkId('ifil', None, -1)
    if Ifil != None:
        IfilD = Ifil.Chunk.DataRead()
        (Major, Minor) = struct.unpack('<2H', IfilD)
    else:
        (Major, Minor) = (2, 1)
    if Major == 2 and Minor >= 4:
        Sm24 = Tree.CkId('sm24', None, -1)
    else:
        Sm24 = None
    if Sm24 != None:
        ExpectedSize = Smpl.Chunk.getsize() / 2
        if ExpectedSize % 2 > 0:
            ExpectedSize = ExpectedSize + 1
        if Sm24.Chunk.getsize() != ExpectedSize:
            PFmtStr = "ignoring sm24, size %d, expected %d"
            logging.warn(PFmtStr % (Sm24.Chunk.getsize(), ExpectedSize))
            Sm24 = None
    Shdr = Tree.CkId('shdr', None, -1)
    if Shdr == None:
        LogDie('no wavetable header')
    Data = Shdr.Chunk.DataRead()
    FmtStr = '<20s5IbB2H'
    FmtLen = struct.calcsize(FmtStr)
    Order = 0
    List = []
    while len(Data) > 46:
        (AchSampleName, DwStart, DwEnd, DwStartLoop, DwEndLoop,
            DwSampleRate, ByOriginalPitch, ChPitchCorrection,
            WSampleLink, SfSampleType) = struct.unpack(FmtStr, Data[0:FmtLen])
        AchSampleName = AchSampleName.split('\0', 1)[0]
        FileName = "%s%d.wav" % (Tree.WtPrefix, Order + 1)
        WDict = {u'id': Order + 1, u'file': FileName, u'name': AchSampleName}
        if SfSampleType == 0:
            logging.warn("wavetable %d, sampleType=0, default to mono" % (
                Order + 1))
        elif SfSampleType == 1:
            pass
        elif SfSampleType == 2:
            WDict[u'channel'] = 'right'
            WDict[u'link'] = WSampleLink + 1
        elif SfSampleType == 4:
            WDict[u'channel'] = 'left'
            WDict[u'link'] = WSampleLink + 1
        else:
            LogDie("wavetable %d, can't use %s SampleType %d" % (Order + 1,
                Def(Val(SfStNames, SfSampleType), 'unknown'), SfSampleType))
        if DwStartLoop != DwStart or DwEndLoop != DwStart:
            WDict[u'loop'] = {u'begin': DwStartLoop - DwStart,
                u'end': DwEndLoop - DwStart}
        if ByOriginalPitch != 60:
            WDict[u'pitch'] = ByOriginalPitch
        if ChPitchCorrection != 0:
            WDict[u'pitchcorr'] = ChPitchCorrection
        List.append(WDict)
        SampleCount = DwEnd - DwStart
        Aud = wave.open(FileName, 'wb')
        Aud.setnchannels(1)
        Aud.setframerate(DwSampleRate)
        Aud.setnframes(SampleCount)
        Smpl.Chunk.seek(DwStart * 2, 0)
        if Sm24 == None:
            Aud.setsampwidth(2)
            DataCopy(Smpl.Chunk, Aud, 2, SampleCount)
        else:
            Sm24F = open(Sm24.Chunk.file.name, 'rb')
            Sm24F.seek(Sm24.Chunk.offset + DwStart)
            Aud.setsampwidth(3)
            DataCopy((Smpl.Chunk, Sm24F), Aud, 2, SampleCount)
            Sm24F.close()
        Aud.close()
        Order = Order + 1
        Data = Data[FmtLen:]
    return List

def SfZoneList(Tree, Zt):
    AchName = 'ZORKMID'
    WBagNdx = -999
    WBank = -999
    Order = 0
    Bag = Tree.CkId(Zt.Bag, None, -1)
    if Bag == None:
        LogDie("no %s section" % (Zt.Bag))
    BagD = Bag.Chunk.DataRead()
    BagFmtStr = '<2H'
    BagRecLen = struct.calcsize(BagFmtStr)
    Gen = Tree.CkId(Zt.Gen, None, -1)
    if Gen == None:
        LogDie("no %s section" % (Zt.Gen))
    GenD = Gen.Chunk.DataRead()
    GenFmtStr = '<2H'
    GenRecLen = struct.calcsize(GenFmtStr)
    Hdr = Tree.CkId(Zt.Hdr, None, -1)
    if Hdr == None:
        LogDie("no %s section" % (Zt.Gen))
    HdrD = Hdr.Chunk.DataRead()
    HdrFmtStr = '<20sH'
    if Zt.KeyN == 'preset':
        HdrFmtStr = HdrFmtStr + '2H3I'
    HdrFmtLen = struct.calcsize(HdrFmtStr)
    List = []
    while len(HdrD) > 0:
        logging.info("reading %s %d" % (Zt.KeyN, Order + 1))
        (ZoneIndex, LastAchName, LastWBagNdx, LastWBank) = (1, AchName,
            WBagNdx, WBank)
        if Zt.KeyN == 'instrument':
            (AchName, WBagNdx) = struct.unpack(HdrFmtStr, HdrD[0:HdrFmtLen])
        elif Zt.KeyN == 'preset':
            (AchName, WPreset, WBank, WBagNdx, DwLibrary, DwGenre,
                DwMorphology) = struct.unpack(HdrFmtStr, HdrD[0:HdrFmtLen])
        AchName = AchName.split('\0', 1)[0]
        HdrD = HdrD[HdrFmtLen:]
        if Order > 0:
            IPDict = {'id': Order, u'name': LastAchName, u'zones': {}}
            if Zt.KeyN == 'preset':
                IPDict[u'bank'] = LastWBank
            ZList = []
            for I in range(LastWBagNdx, WBagNdx):
                Base = I * BagRecLen
                J = WGenNdx = struct.unpack('<H', BagD[Base:Base + 2])[0]
                ZDict = {}
                Generators = []
                while True:
                    OBase = J * GenRecLen
                    ABase = OBase + 2
                    SfGenOper = struct.unpack('<H', GenD[OBase:OBase + 2])[0]
                    (RangeBegin, RangeEnd) = struct.unpack('2B',
                        GenD[ABase:ABase + 2])
                    ShAmount = struct.unpack('<h', GenD[ABase:ABase + 2])[0]
                    WAmount = struct.unpack('<H', GenD[ABase:ABase + 2])[0]
                    try:
                        Name = SfGenNames[SfGenOper].split('_', 1)[1]
                    except KeyError:
                        Name = 'unknown'
                    if ListHas([43, 44], SfGenOper) == True:
                        ZDict[ustr(Name)] = {u'begin': RangeBegin,
                            u'end': RangeEnd}
                    elif ListHas([33, 34, 35, 36, 38], SfGenOper) == True:
                        Dt = 0
                        if ShAmount != SHMIN:
                            Dt = pow(2.0, ShAmount / 1200.0)
                        ZDict[ustr(Name)] = u'%0.04f' % (Dt)
                    elif ListHas([37, 39, 40], SfGenOper) == True:
                        ZDict[ustr(Name)] = ShAmount
                    elif ListHas([53, 41], SfGenOper) == True:
                        if SfGenOper != Zt.Oper:
                            LogDie("%s operator found in %s context" % (
                                Name, Zt.KeyN))
                        ZDict[ustr(Zt.ItemN) + u'Id'] = WAmount + 1
                    elif SfGenOper == 58:
                        if Zt.KeyN == 'preset':
                            logging.warn('ignoring overridingRootKey in preset')
                        else:
                            ZDict[ustr(Name)] = WAmount
                    elif SfGenOper == 57:
                        if Zt.KeyN == 'preset':
                            logging.warn('ignoring exclusiveClass in preset')
                        else:
                            ZDict[ustr(Name)] = WAmount
                    elif SfGenOper == 54:
                        pass
                    else:
                        Generators.append({u'comment': Name,
                            u'hexAmount': "0x%x" % WAmount, u'oper': SfGenOper})
                    if SfGenOper == Zt.Oper:
                        break
                    J = J + 1
                if len(Generators) > 0:
                    ZDict[u'gens'] = {u'gen': Generators}
                ZList.append(ZDict)
                ZoneIndex = ZoneIndex + 1
            IPDict[u'zones'][u'zone'] = ZList
            List.append(IPDict)
        Order = Order + 1
    return List

def SfItems():
    return [SfTreeItem(0, 'RIFF', 'sfbk', None),
        SfTreeItem(1, 'LIST', 'INFO', None),
        SfTreeItem(1, 'LIST', 'sdta', None),
        SfTreeItem(1, 'LIST', 'pdta', None),
        SfTreeItem(2, 'ifil', None, None),
        SfTreeItem(2, 'isng', None, None),
        SfTreeItem(2, 'INAM', None, None),
        SfTreeItem(2, 'irom', None, None),
        SfTreeItem(2, 'iver', None, None),
        SfTreeItem(2, 'ICRD', None, None),
        SfTreeItem(2, 'IENG', None, None),
        SfTreeItem(2, 'IPRD', None, None),
        SfTreeItem(2, 'ICOP', None, None),
        SfTreeItem(2, 'ICMT', None, None),
        SfTreeItem(2, 'ISFT', None, None),
        SfTreeItem(2, 'smpl', None, None),
        SfTreeItem(2, 'sm24', None, None),
        SfTreeItem(2, 'phdr', None, None),
        SfTreeItem(2, 'pbag', None, None),
        SfTreeItem(2, 'pmod', None, None),
        SfTreeItem(2, 'pgen', None, None),
        SfTreeItem(2, 'inst', None, None),
        SfTreeItem(2, 'ibag', None, None),
        SfTreeItem(2, 'imod', None, None),
        SfTreeItem(2, 'igen', None, None),
        SfTreeItem(2, 'shdr', None, None)]

def SfToXml(Src, Dst):
    WtPrefix = os.path.splitext(Dst)[0]
    InHandle = open(Src, 'rb')
    OutHandle = open(Dst, 'wb')
    Chunk = SfChunkReader(InHandle)
    Tree = SfTree(SfItems(), SfContainers, None, OutHandle, WtPrefix)
    Tree.Read(Chunk, 0)
    Ifil = Tree.CkId('ifil', None, -1)
    if Ifil != None:
        IfilD = Ifil.Chunk.DataRead()
        (Major, Minor) = struct.unpack('<2H', IfilD)
    else:
        (Major, Minor) = (2, 1)
    Dict = {u'wavetables': {u'wavetable': SfWavetableList(Tree)},
        u'instruments': {u'instrument': SfZoneList(Tree, \
            SfZoneType('instrument'))},
        u'presets': {u'preset': SfZoneList(Tree, SfZoneType('preset'))},
        u'ISNG': Def(Tree.CkIdStr('isng', None, -1), u'pysf song'),
        u'INAM': Def(Tree.CkIdStr('INAM', None, -1), u'pysf instruments'),
        u'ICRD': Def(Tree.CkIdStr('ICRD', None, -1), ustr(DateAsciiGet())),
        u'IPRD': Def(Tree.CkIdStr('IPRD', None, -1), u'SBAWE32'),
        u'IFIL': {u'major': Major, u'minor': Minor},
        u'ISFT': Def(Tree.CkIdStr('ISFT', None, -1), \
            u'pysf %d:pysf %d' % (PysfVersion, PysfVersion))}
    OutHandle.write(DictToXmlStr({u'sf2': Dict}))
    InHandle.close()
    OutHandle.close()

def SfIfil(Dict):
    try:
        Retval = (Dict[u'IFIL'][u'major'], Dict[u'IFIL'][u'minor'])
    except KeyError:
        Retval = None
    return Retval

def SfInfo(Dict):
    (SfMajor, SfMinor) = Def(SfIfil(Dict), (2, 1))
    List = [['LIST', 'INFO'], ['ifil', struct.pack('<2H', SfMajor, SfMinor),
        'isng', SfStr(Def(Val(Dict, u'ISNG'), 'EMU8000')),
        'INAM', SfStr(Def(Val(Dict, u'INAM'), 'noname')),
        'ICRD', SfStr(Def(Val(Dict, u'ICRD'), DateAsciiGet()))]]
    Ieng = SfStr(Val(Dict, u'IENG'))
    if Ieng != None:
        List[1].append('IENG', Ieng)
    map(List[1].append,
        ['IPRD', SfStr(Def(Val(Dict, u'IPRD'), 'SBAWE32')),
        'ISFT', SfStr(Def(Val(Dict, u'ISFT'), 'SFEDT v1.28'))])
    return List

def StereoSampleCheck(Wavetables, Id, Channel, WSampleLink):
    if Channel == 'right':
       (RightId, LeftId) = (Id, WSampleLink)
    else:
       (RightId, LeftId) = (WSampleLink, Id)
    Left = LdFind(Wavetables, u'id', LeftId)
    if Left == None:
        LogDie("Wavetable %d: Can't find left channel" % (Id))
    Right = LdFind(Wavetables, u'id', RightId)
    if Right == None:
        LogDie("Wavetable %d: Can't find right channel" % (Id))
    if Left[u'link'] != RightId:
        LogDie("Wavetable %d: Left channel not linked to right" % (Id))
    if Right[u'link'] != LeftId:
        LogDie("Wavetable %d: Right channel not linked to Left" % (Id))

def SfSdtaShdr(Dict):
    ShdrFmtStr = '<20sIIIIIBbHH'
    ShdrD = ''
    SmplD = tempfile.TemporaryFile()
    Sm24D = tempfile.TemporaryFile()
    Id = -1
    Order = 0
    (Major, Minor) = Def(SfIfil(Dict), (2, 1))
    GlobalSampWidth = -1
    Wavetables = Dict[u'wavetables'][u'wavetable']
    for Wavetable in Wavetables:
        Id = Wavetable[u'id']
        if Id != Order + 1:
            LogDie("Wavetable %d: id=%d, expected %d" % (Order + 1, Id,
                Order + 1))
        FileName = Wavetable[u'file']
        Ext = os.path.splitext(FileName)[1][1:4].lower()
        if Ext == 'wav':
            Aud = wave.open(str(FileName), 'rb')
            DataOrder = 'little'
        elif Ext == 'aif':
            Aud = aifc.open(str(FileName), 'rb')
            DataOrder = 'big'
        else:
            LogDie("Wavetable %d: Unknown format" % (Id))
        Byteswap = True
        if DataOrder == sys.byteorder:
            Byteswap = False
        WtName = SfStr(Def(Val(Wavetable, u'name'), ''), 20)
        try:
            WtLoopstart = Wavetable[u'loop'][u'begin']
            WtLoopend = Wavetable[u'loop'][u'end']
        except KeyError:
            (WtLoopstart, WtLoopend) = (0, 0)
        if WtLoopstart < 0 or WtLoopstart > Aud.getnframes():
            logging.warn("Wavetable %d: Loopstart out of range" % (Id))
            WtLoopstart = 0
        if WtLoopend < 0 or WtLoopend > Aud.getnframes():
            logging.warn("Wavetable %d: Loopend out of range" % (Id))
            WtLoopend = 0
        if WtLoopstart > 0 or WtLoopend > 0:
            WLoopMid = WtLoopend - WtLoopstart
            WLoopEnd = Aud.getnframes() - WtLoopend
            if WtLoopstart < 8 or WLoopMid < 31 or WLoopEnd < 7:
                logging.warn("Wavetable %d: Insufficient loop margin" % (Id))
                (WtLoopstart, WtLoopend) = (0, 0)
        ByOriginalPitch = Def(Val(Wavetable, u'pitch'), 60)
        SfSampleType = 1
        WSampleLink = 0
        try:
            Channel = Wavetable[u'channel']
            if Channel == 'right':
                SfSampleType = 2
                WSampleLink = Wavetable[u'link'] - 1
                StereoSampleCheck(Wavetables, Id, Channel, WSampleLink + 1)
            elif Channel == 'left':
                SfSampleType = 4
                WSampleLink = Wavetable[u'link'] - 1
                StereoSampleCheck(Wavetables, Id, Channel, WSampleLink + 1)
        except KeyError:
            pass
        if ByOriginalPitch > 127:
            if ByOriginalPitch != 255:
                logging.warn("Wavetable %d: Pitch out of range" % (Id))
            ByOriginalPitch = 60 # MIDI C-5
        ChPitchCorrection = Def(Val(Wavetable, u'pitchcorr'), 0)
        AudChannel = -1 # no filter
        if Aud.getnchannels() == 2:
            if SfSampleType == 2:
                AudChannel = 1 # right, filter out left
            elif SfSampleType == 4:
                AudChannel = 0 # left, filter out right
        WtStart = SmplD.tell() / 2
        WtEnd = WtStart + Aud.getnframes()
        WtLoopstart = WtLoopstart + WtStart
        WtLoopend = WtLoopend + WtStart
        WtRate = Aud.getframerate()
        SmplCksize = (WtEnd + 46) * 2
        if Major == 2 and Minor >= 4:
            if GlobalSampWidth == -1:
                GlobalSampWidth = Aud.getsampwidth()
            if GlobalSampWidth != Aud.getsampwidth():
                LogDie("Wavetable %d: %d bit, other are %d bit" % (Order + 1,
                    Aud.getsampwidth() * 8, GlobalSampWidth * 8))
        else:
            if Aud.getsampwidth() == 3:
                LogDie("Wavetable %d: 24 bit, but ifil 2.1" % (Order + 1))
        if Aud.getsampwidth() == 2:
            DataCopy(Aud, SmplD, 2, Aud.getnframes(), Byteswap, AudChannel)
        elif Aud.getsampwidth() == 3:
            DataCopy(Aud, Sm24D, 3, Aud.getnframes(), Byteswap,
                AudChannel, 'part24')
            Sm24D.write(struct.pack('46s', '')) # 46 sample Pad
            Aud.rewind()
            DataCopy(Aud, SmplD, 3, Aud.getnframes(), Byteswap,
                AudChannel, "part16")
        else:
            LogDie("Wavetable %d: can't use %d bit sample width" % (Order + 1,
                Aud.getsampwidth() * 8))
        SmplD.write(struct.pack('92s', '')) # 46 sample Pad
        Aud.close()
        ShdrD = ShdrD + struct.pack(ShdrFmtStr, WtName, WtStart,
            WtEnd, WtLoopstart, WtLoopend, WtRate, ByOriginalPitch,
            ChPitchCorrection, WSampleLink, SfSampleType)
        Order = Order + 1
    WtName = 'EOS'
    ShdrD = ShdrD + struct.pack(ShdrFmtStr, 'EOS', 0, 0, 0, 0, 0, 0,
        0, 0, 0)
    Shdr = ['shdr', ShdrD]
    if Major == 2 and Minor >= 4:
        Sdta = [['LIST', 'sdta'], ['smpl', SmplD, 'sm24', Sm24D]]
    else:
        Sdta = [['LIST', 'sdta'], ['smpl', SmplD]]
    return (Sdta, Shdr)

def SfRange(Item, Key, Min, Max, DefaultVal, Msg, Warn):
    try:
        Begin = Item[Key][u'begin']
        End = Item[Key][u'end']
        if Begin < Min or End < Min or Begin > Max or End > Max:
            LogDie("%s: invalid %s" % (Msg, Key))
    except KeyError:
        Begin = End = DefaultVal
        if Warn == True:
            logging.warn("%s: no %s" % (Msg, Key))
    return (Begin, End)

def SfLog(Item, Key, DefaultVal):
    Value = float(Def(Val(Item, Key), DefaultVal))
    if Value == 0.0:
        Value = SHMIN
    elif Value == SHOOBVAL:
        pass
    else:
        Value = math.floor(1200.0 * (math.log(Value) / math.log(2)))
    return Value

def SfZone(Dict, Zt):
    Order = 0
    LastNBag = 0
    Loopstart = 0
    Loopend = 0
    IopsCount = 0
    ItemMax = len(Dict[Zt.ItemN + u's'][Zt.ItemN])
    GenC = 0
    ModC = 0
    BagC = 0
    HdrC = 0
    GenD = ''
    ModD = ''
    BagD = ''
    HdrD = ''

    for InPr in Dict[Zt.KeyN + u's'][Zt.KeyN]:
        logging.info("reading %s %d" % (Zt.KeyN, Order + 1))
        Name = SfStr(Def(Val(InPr, u'name'), ''), 20)
        if Zt.KeyN == 'preset':
            WBank = Def(Val(InPr, u'bank'), 0)
        ZoneIndex = 0
        if len(InPr[u'zones'][u'zone']) == 0:
            GenD = GenD + struct.pack('<HH', 60, 0)
            BagD = BagD + struct.pack('<HH', GenC, ModC)
            GenC = GenC + 1
            ModC = ModC + 1
        for Zone in InPr[u'zones'][u'zone']:
            Wstr = "%s %d zone %d" % (Zt.KeyN, Order + 1, ZoneIndex + 1)
            logging.info("reading %s" % (Wstr))
            IopsCount = 1
            ItemRef = Zone[Zt.ItemN + u'Id'] - 1
            if ItemRef < 0 or ItemRef > ItemMax:
                LogDie("%s: invalid %s index %d (range 0,%d)" % (
                    Wstr, Zt.ItemN, ItemRef, ItemMax))
            (KeyRangeBegin, KeyRangeEnd) = SfRange(Zone, u'keyRange',
                0, 127, -1, Wstr, True)
            if KeyRangeBegin > -1 and KeyRangeEnd > -1:
                IopsCount = IopsCount + 1
                GenD = GenD + struct.pack('<HBB', 43, KeyRangeBegin,
                    KeyRangeEnd)
            (VelRangeBegin, VelRangeEnd) = SfRange(Zone, u'velRange',
                0, 127, -1, Wstr, False)
            if VelRangeBegin > -1 and VelRangeEnd > -1:
                IopsCount = IopsCount + 1
                GenD = GenD + struct.pack('<HBB', 44, VelRangeBegin,
                    VelRangeEnd)
            OverridingRootKey = Def(Val(Zone, u'overridingRootKey'), -1)
            if OverridingRootKey > -1:
                IopsCount = IopsCount + 1
                GenD = GenD + struct.pack('<HH', 58, OverridingRootKey)
            ExclusiveClass = Def(Val(Zone, u'exclusiveClass'), -1)
            if ExclusiveClass > -1:
                IopsCount = IopsCount + 1
                GenD = GenD + struct.pack('<HH', 57, ExclusiveClass)
            if Zt.KeyN == 'instrument':
                try:
                    Wavetable = LdFind(Dict[u'wavetables'][u'wavetable'],
                        u'id', ItemRef + 1)
                    Loopstart = Wavetable[u'loop'][u'begin']
                    Loopend = Wavetable[u'loop'][u'end']
                except (IndexError, KeyError):
                    (Loopstart, Loopend) = (0, 0)
                if Loopstart != 0 or Loopend != 0:
                    LoopType = 3 # Release
                else:
                    LoopType = 0 # None
                IopsCount = IopsCount + 1
                GenD = GenD + struct.pack('<HH', 54, LoopType)
            EnvDelay = SfLog(Zone, u'delayVolEnv', SHOOBVAL)
            if EnvDelay > SHOOBVAL:
                IopsCount = IopsCount + 1
                GenD = GenD + struct.pack('<HH', 33, EnvDelay)
            EnvAttack = SfLog(Zone, u'attackVolEnv', SHOOBVAL)
            if EnvAttack > SHOOBVAL:
                IopsCount = IopsCount + 1
                GenD = GenD + struct.pack('<HH', 34, EnvAttack)
            EnvHold = SfLog(Zone, u'holdVolEnv', SHOOBVAL)
            if EnvHold > SHOOBVAL:
                IopsCount = IopsCount + 1
                GenD = GenD + struct.pack('<HH', 35, EnvHold)
            EnvDecay = SfLog(Zone, u'decayVolEnv', SHOOBVAL)
            if EnvDecay > SHOOBVAL:
                IopsCount = IopsCount + 1
                GenD = GenD + struct.pack('<HH', 36, EnvDecay)
            EnvSustain = Def(Val(Zone, u'sustainVolEnv'), SHOOBVAL)
            if EnvSustain > SHOOBVAL:
                IopsCount = IopsCount + 1
                GenD = GenD + struct.pack('<HH', 37, EnvSustain)
            EnvRelease = SfLog(Zone, u'releaseVolEnv', SHOOBVAL)
            if EnvRelease > SHOOBVAL:
                IopsCount = IopsCount + 1
                GenD = GenD + struct.pack('<HH', 38, EnvRelease)
            KtveHold = Def(Val(Zone, u'keynumToVolEnvHold'), SHOOBVAL)
            if KtveHold > SHOOBVAL:
                IopsCount = IopsCount + 1
                GenD = GenD + struct.pack('<HH', 39, KtveHold)
            KtveDecay = Def(Val(Zone, u'keynumToVolEnvDecay'), SHOOBVAL)
            if KtveDecay > SHOOBVAL:
                IopsCount = IopsCount + 1
                GenD = GenD + struct.pack('<HH', 40, KtveDecay)
            try:
                for Generator in Zone[u'gens'][u'gen']:
                    IopsCount = IopsCount + 1
                    GenD = GenD + struct.pack('<HH', Generator[u'oper'],
                        Generator[u'hexAmount'])
            except KeyError:
                pass
            GenD = GenD + struct.pack('<HH', Zt.Oper, ItemRef)
            BagD = BagD + struct.pack('<HH', GenC, ModC)
            GenC = GenC + IopsCount
            BagC = BagC + 1
            ZoneIndex = ZoneIndex + 1
        if Zt.KeyN == 'instrument':
            HdrD = HdrD + struct.pack('<20sH', Name, LastNBag)
        elif Zt.KeyN == 'preset':
            WPreset = HdrC
            HdrD = HdrD + struct.pack('<20sHHHIII', Name, WPreset,
                WBank, LastNBag, 0, 0, 0)
        HdrC = HdrC + 1
        LastNBag = BagC
        Order = Order + 1
    return (GenD, ModD, BagD, HdrD, GenC, ModC, BagC, HdrC)

def SfPdta(Dict, Shdr):
    (IgenD, ImodD, IbagD, InstD,
     IgenC, ImodC, IbagC, InstC) = SfZone(Dict, SfZoneType('instrument'))
    (PgenD, PmodD, PbagD, PhdrD,
     PgenC, PmodC, PbagC, PhdrC) = SfZone(Dict, SfZoneType('preset'))
    InstD = InstD + struct.pack('<20sH', 'EOI', IbagC)
    IbagD = IbagD + struct.pack('<HH', IgenC, 0)
    IgenD = IgenD + struct.pack('<HH', 0, 0)
    ImodD = ImodD + struct.pack('<HHhHH', 0, 0, 0, 0, 0)
    PbagD = PbagD + struct.pack('<HH', PgenC, PmodC)
    PgenD = PgenD + struct.pack('<HH', 0, 0)
    PmodD = PmodD + struct.pack('<HHhHH', 0, 0, 0, 0, 0)
    PhdrD = PhdrD + struct.pack('<20sHHHIII', 'EOP', 0, 0, PbagC, 0, 0, 0)
    Pdta = [['LIST', 'pdta'], ['phdr', PhdrD, 'pbag', PbagD,
        'pmod', PmodD, 'pgen', PgenD, 'inst', InstD, 'ibag', IbagD,
        'imod', ImodD, 'igen', IgenD, Shdr[0], Shdr[1]]]
    return Pdta

def XmlToSf(Src, Dst):
    OutHandle = open(Dst, 'wb')
    try:
        Dict = XmlFileToDict(Src)[u'sf:pysf'][u'sf2']
    except KeyError:
        LogDie('Invalid input format.')
    Info = SfInfo(Dict)
    [Sdta, Shdr] = SfSdtaShdr(Dict)
    Pdta = SfPdta(Dict, Shdr)
    List = [['RIFF', 'sfbk'], [Info[0], Info[1], Sdta[0], Sdta[1],
        Pdta[0], Pdta[1]]]
    ListToIff(List, OutHandle)
    OutHandle.close()

logging.getLogger().setLevel(logging.WARN)
PysfVersion = 2
SfContainers = ('RIFF', 'LIST')
SfGenNames = ['0_startAddrsOffset',
    '1_endAddrsOffset',
    '2_startloopAddrsOffset',
    '3_endloopAddrsOffset',
    '4_startAddrsCoarseOffset',
    '5_modLfoToPitch',
    '6_vibLfoToPitchi',
    '7_modEnvToPitch',
    '8_initialFilterFc',
    '9_initialFilterQ',
    '10_modLfoToFilterFc',
    '11_modEnvToFilterFc',
    '12_endAddrsCoarseOffset',
    '13_modLfoToVolume',
    '14_unused',
    '15_chorusEffectsSend',
    '16_reverbEffectsSend',
    '17_pan',
    '18_unused',
    '19_unused',
    '20_unused',
    '21_delayModLFO',
    '22_freqModLFO',
    '23_delayVibLFO',
    '24_freqVibLFO',
    '25_delayModEnv',
    '26_attackModEnv',
    '27_holdModEnv',
    '28_decayModEnv',
    '29_sustainModEnv',
    '30_releaseModEnv',
    '31_keynumToModEnvHold',
    '32_keynumToModEnvDecay',
    '33_delayVolEnv',
    '34_attackVolEnv',
    '35_holdVolEnv',
    '36_decayVolEnv',
    '37_sustainVolEnv',
    '38_releaseVolEnv',
    '39_keynumToVolEnvHold',
    '40_keynumToVolEnvDecay',
    '41_instrument',
    '42_reserved',
    '43_keyRange',
    '44_velRange',
    '45_startloopAddrsCoarseOffset',
    '46_keynum',
    '47_velocity',
    '48_initialAttenuation',
    '49_reserved',
    '50_endloopAddrsCoarseOffset',
    '51_coarseTune',
    '52_fineTune',
    '53_sampleId',
    '54_sampleModes',
    '55_reserved',
    '56_scaleTuning',
    '57_exclusiveClass',
    '58_overridingRootKey',
    '59_unused',
    '60_endOper']
SfStNames = {1: 'monoSample',
    2: 'rightSample',
    4: 'leftSample',
    8: 'linkedSample',
    32769: 'RomMonoSample',
    32770: 'RomRightSample',
    32772: 'RomLeftSample',
    32776: 'RomLinkedSample'}
SfModIndexDescs = {0: 'No Controller',
    2: 'Note-On Velocity',
    3: 'Note-On Key Number',
    10: 'Poly Pressure',
    13: 'Channel Pressure',
    14: 'Pitch Wheel',
    16: 'Pitch Wheel Sensitivity'}
SfModTypeDescs = ('Linear', 'Concave', 'Convex', 'Switch')
XmlHeaderStr = u'<sf:pysf version="' + ustr(PysfVersion) + u'" xmlns:sf="http://terrorpin.net/~ben/docs/alt/music/soundfont/pysf" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xsi:schemaLocation="http://terrorpin.net/~ben/docs/alt/music/soundfont/pysf pysf.xsd">'
XmlRootStr = XmlHeaderStr + u'</sf:pysf>'
SHMIN = -32768
SHOOBVAL = -32769

if __name__ == '__main__':
    if len(sys.argv) != 4:             PrintUsage()
    if (sys.argv[1] == '--sf2xml'):    SfToXml(sys.argv[2], sys.argv[3])
    elif (sys.argv[1] == '--xml2sf'):  XmlToSf(sys.argv[2], sys.argv[3])
    elif (sys.argv[1] == '--aif2xml'): AudToXml(sys.argv[2], sys.argv[3], 'aif')
    elif (sys.argv[1] == '--xml2aif'): XmlToAud(sys.argv[2], sys.argv[3], 'aif')
    elif (sys.argv[1] == '--wav2xml'): AudToXml(sys.argv[2], sys.argv[3], 'wav')
    elif (sys.argv[1] == '--xml2wav'): XmlToAud(sys.argv[2], sys.argv[3], 'wav')
