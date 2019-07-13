# Hex dump a bin file, doing the bit switching

import sys

FILE = 'Luminary099.bin'


if len(sys.argv) != 2:
  f = open(FILE, 'rb')
  data = f.read()
  for addr in range(0, 36 * 1024):
    hw = (ord(data[2 * addr]) << 8) | ord(data[2 * addr + 1])
    agc_value = ((hw & 0x8000) >> 1) | (hw & 0x3fff)
    print '%06o: %06o hw, %05o agc' % (addr, hw, agc_value)
  sys.exit(0)

f = open(FILE, 'rb')
addr = int(sys.argv[1], 8)
f.seek(addr * 2)
data = f.read(2)
hw = (ord(data[0]) << 8) | ord(data[1])
agc_value = ((hw & 0x8000) >> 1) | (hw & 0x3fff)
print '%06o: %06o hw, %05o agc' % (addr, hw, agc_value)
