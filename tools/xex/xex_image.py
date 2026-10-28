import struct, subprocess, sys
d = open(sys.argv[1], 'rb').read()
pe = struct.unpack('>I', d[8:12])[0]; sec = struct.unpack('>I', d[0x10:0x14])[0]
n = struct.unpack('>I', d[0x14:0x18])[0]
hdr = {}
for i in range(n):
    k, v = struct.unpack('>II', d[0x18+8*i:0x20+8*i]); hdr[k] = v
ff = hdr[0x3FF]
size = struct.unpack('>I', d[ff:ff+4])[0]
enc, comp = struct.unpack('>HH', d[ff+4:ff+8])
assert comp == 1
enc_key = d[sec+0x150:sec+0x160]
RETAIL = bytes.fromhex('20B185A59D28FDC340583FBB0896BF91')
def aes(mode, key, data, iv=None):
    args = ['openssl', 'enc', '-d', '-'+mode, '-K', key.hex(), '-nopad'] + (['-iv', iv.hex()] if iv else [])
    return subprocess.run(args, input=data, capture_output=True, check=True).stdout
key = aes('aes-128-ecb', RETAIL, enc_key)
body = d[pe:]
if enc == 1:
    body = aes('aes-128-cbc', key, body[:len(body)//16*16], b'\0'*16)
out = bytearray(); p = 0
blocks = [(struct.unpack('>II', d[ff+8+8*i:ff+16+8*i])) for i in range((size-8)//8)]
for ds, zs in blocks:
    out += body[p:p+ds]; p += ds; out += b'\0'*zs
open(sys.argv[2], 'wb').write(out)
print(len(out), out[:2])
