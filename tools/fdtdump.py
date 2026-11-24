import struct, sys, re
data = open(sys.argv[1], 'rb').read()
pat = re.compile(sys.argv[2]) if len(sys.argv) > 2 else None
magic, tot, off_struct, off_str = struct.unpack('>IIII', data[:16])
assert magic == 0xd00dfeed
def s(o): return data[off_str+o:data.index(b'\0', off_str+o)].decode()
p = off_struct; path = []; out = []
def fmt(name, v):
    if v and v[-1:] == b'\0' and all(32 <= c < 127 or c == 0 for c in v) and v[0] != 0:
        return repr(v.rstrip(b'\0').replace(b'\0', b'|').decode())
    if len(v) % 4 == 0:
        return '<' + ' '.join('%#x' % x for x in struct.unpack('>%dI' % (len(v)//4), v)) + '>'
    return v.hex()
while True:
    tok, = struct.unpack('>I', data[p:p+4]); p += 4
    if tok == 1:
        e = data.index(b'\0', p); path.append(data[p:e].decode()); p = (e + 4) & ~3
        full = '/'.join(path) or '/'
        if not pat or pat.search(full): print('NODE', full)
    elif tok == 2: path.pop()
    elif tok == 3:
        ln, no = struct.unpack('>II', data[p:p+8]); p += 8
        v = data[p:p+ln]; p = (p + ln + 3) & ~3
        full = '/'.join(path)
        if not pat or pat.search(full): print('   ', s(no), '=', fmt(s(no), v))
    elif tok == 4: continue
    elif tok == 9: break
