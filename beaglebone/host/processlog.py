# Process a log, finding regions of address access

import re
import sys

if len(sys.argv) < 2:
  sys.exit('Usage: python processlog.py file.log')


lastaddr = None
for line in open(sys.argv[1]).readlines():
  if 'FAULT' in line and lastaddr is not None:
    print
    print '%06o' % lastaddr,
    print line.strip()
  m = re.match('ADDR.*a:.*a:([0-7]+)', line)
  if m:
    newaddr = int(m.group(1), 8)
    if lastaddr is not None and newaddr != lastaddr + 1:
      print 'to %06o' % lastaddr
      print '%6o' % newaddr,

    lastaddr = newaddr

if lastaddr:
  print 'to %06o' % lastaddr
