# Print information on an address

import sys

if len(sys.argv) == 1:
  sys.exit('usage: addr 012345')

addr = int(sys.argv[1], 8)

il = addr & 0x7f
il_b = '{0:07b}'.format(il)
p = addr >> 7
p_b = '{0:09b}'.format(p)

plane = p >> 2

strand = (plane % 12) + 1
rst = plane / 24
rst_str = ['R', 'S', 'T'][rst]
mod12 = ((plane / 12) % 2) + 1
print 'a:%6o x %4x b %s %s: %s %d str %d' % (addr, addr, p_b, il_b, rst_str, mod12, strand)
