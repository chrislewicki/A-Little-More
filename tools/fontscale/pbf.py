"""Reader/writer for Pebble .pbf (font v3) files, mirroring the SDK's fontgen.py layout."""
import struct

WILDCARD = 0x25AF
FEATURE_OFFSET_16 = 0x01
FEATURE_RLE4 = 0x02
HASH_TABLE_SIZE = 255

class Glyph:
    __slots__ = ("width","height","left","top","advance","bits")
    def __init__(self, width, height, left, top, advance, bits):
        self.width, self.height, self.left, self.top, self.advance, self.bits = width, height, left, top, advance, bits
    def rows(self):
        return [self.bits[r*self.width:(r+1)*self.width] for r in range(self.height)]

class PBF:
    def __init__(self):
        self.max_height = 0
        self.glyphs = {}   # codepoint -> Glyph (shared Glyph objects allowed)

def _rle4_decode(data, units):
    out=[]
    i=0
    while units>0:
        b=data[i]; i+=1
        for _ in range(min(units,2)):
            colour=(b>>3)&1; length=(b&7)+1
            out.extend([colour]*length)
            b>>=4; units-=1
    return out

def read(path):
    d=open(path,"rb").read()
    version,max_height,nglyphs,wildcard,hts,cpb=struct.unpack_from("<BBHHBB",d,0)
    assert version==3, version
    size,features=struct.unpack_from("<BB",d,8)
    assert size==10
    off16=bool(features&FEATURE_OFFSET_16); rle=bool(features&FEATURE_RLE4)
    hash_off=size
    hash_tbl=[struct.unpack_from("<BBH",d,hash_off+4*i) for i in range(hts)]
    ot_off=hash_off+4*hts
    entry_fmt="<"+("L" if cpb==4 else "H")+("H" if off16 else "L")
    entry_sz=struct.calcsize(entry_fmt)
    total_entries=sum(b[1] for b in hash_tbl)
    gt_off=ot_off+total_entries*entry_sz
    f=PBF(); f.max_height=max_height
    cache={}
    for (hv,bsize,boff) in hash_tbl:
        for k in range(bsize):
            cp,goff=struct.unpack_from(entry_fmt,d,ot_off+boff+k*entry_sz)
            if goff in cache:
                f.glyphs[cp]=cache[goff]; continue
            p=gt_off+goff
            w,h,left,top,adv=struct.unpack_from("<BBbbb",d,p); p+=5
            if w and h:
                if rle:
                    nbytes=(h+1)//2
                    bits=_rle4_decode(d[p:p+nbytes],h)
                    # true height = len(bits)/w
                    assert len(bits)%w==0,(cp,len(bits),w)
                    h=len(bits)//w
                else:
                    nbits=w*h; nbytes=(nbits+7)//8
                    bits=[]
                    for i in range(nbits):
                        bits.append((d[p+i//8]>>(i%8))&1)
                bits=bits[:w*h]
            else:
                bits=[]
            g=Glyph(w,h,left,top,adv,bits)
            cache[goff]=g; f.glyphs[cp]=g
    assert len(f.glyphs)==nglyphs,(len(f.glyphs),nglyphs)
    return f

def write(path, font):
    """Write uncompressed v3 pbf (16-bit offsets if it fits), like fontgen without compression."""
    cps=sorted(font.glyphs)
    assert WILDCARD in cps
    cpb=4 if max(cps)>0xFFFF else 2
    glyph_table=[struct.pack("<I",0)]
    offsets={}; next_off=4
    entries=[]
    # wildcard first (fontgen does), then others; dedupe shared glyph objects
    order=[WILDCARD]+[c for c in cps if c!=WILDCARD]
    for cp in order:
        g=font.glyphs[cp]
        key=id(g)
        if key not in offsets:
            assert g.width<256 and g.height<256 and -128<=g.left<128 and -128<=g.top<128 and -128<=g.advance<128,(cp,g.width,g.height,g.left,g.top,g.advance)
            hdr=struct.pack("<BBbbb",g.width,g.height,g.left,g.top,g.advance)
            words=[]
            nbits=g.width*g.height
            for i in range(0,nbits,32):
                w=0
                for j,bit in enumerate(g.bits[i:i+32]):
                    w|=bit<<j
                words.append(struct.pack("<I",w))
            blob=hdr+b"".join(words)
            offsets[key]=next_off
            glyph_table.append(blob)
            next_off+=len(blob)
        entries.append((cp,offsets[key]))
    glyph_bytes=sum(len(b) for b in glyph_table)
    features=0
    off16 = glyph_bytes<65536
    if off16: features|=FEATURE_OFFSET_16
    entry_fmt="<"+("L" if cpb==4 else "H")+("H" if off16 else "L")
    buckets=[[] for _ in range(HASH_TABLE_SIZE)]
    for cp,off in sorted(entries):
        buckets[cp%HASH_TABLE_SIZE].append(struct.pack(entry_fmt,cp,off))
    hash_tbl=[]; acc=0
    for i in range(HASH_TABLE_SIZE):
        hash_tbl.append(struct.pack("<BBH",i,len(buckets[i]),acc))
        acc+=len(buckets[i])*struct.calcsize(entry_fmt)
    info=struct.pack("<BBHHBBBB",3,font.max_height,len(entries),WILDCARD,HASH_TABLE_SIZE,cpb,10,features)
    out=info+b"".join(hash_tbl)+b"".join(b"".join(b) for b in buckets)+b"".join(glyph_table)
    open(path,"wb").write(out)
    return len(out)

def render(font, text, scale=1):
    """Render text to a PIL image (black on white) using the font's metrics."""
    from PIL import Image
    x=0; pen=[]
    for ch in text:
        g=font.glyphs.get(ord(ch)) or font.glyphs[WILDCARD]
        pen.append((x,g)); x+=g.advance
    W=max(1,x+8); H=font.max_height+4
    im=Image.new("L",(W,H),255); px=im.load()
    for x0,g in pen:
        for r,row in enumerate(g.rows()):
            for c,bit in enumerate(row):
                if bit:
                    xx=x0+g.left+c; yy=g.top+r
                    if 0<=xx<W and 0<=yy<H: px[xx,yy]=0
    if scale!=1: im=im.resize((W*scale,H*scale),Image.NEAREST)
    return im

def read_bytes(data):
    import tempfile, os
    fd, p = tempfile.mkstemp(suffix=".pbf"); os.write(fd, data); os.close(fd)
    try:
        return read(p)
    finally:
        os.unlink(p)
