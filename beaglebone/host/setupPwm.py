# Setup using Adafruit for PWM, direct for GPIO
from collections import defaultdict
import time
import Adafruit_BBIO.PWM as PWM

pins = [
  ["sense1", "P8_18", 65],
  ["sense2", "P8_7", 66],
  ["sense3", "P8_8", 67],
  ["sense4", "P8_10", 68],
  ["sense5", "P8_9", 69],
  ["sense6", "P8_45", 70],
  ["sense7", "P8_46", 71],
  ["sense8", "P8_43", 72],
  ["sense9", "P8_44", 73],
  ["sense10", "P8_41", 74],
  ["sense11", "P8_42", 75],
  ["sense12", "P8_39", 76],
  ["sense13", "P8_40", 77],
  ["sense14", "P8_37", 78],
  ["sense15", "P8_38", 79],
  ["sense16", "P8_36", 80],
  ["il1", "P8_12", 44],
  ["il2", "P8_11", 45],
  ["il3", "P8_16", 46],
  ["il4", "P8_15", 47],
  ["il5", "P9_15", 48],
  ["il6", "P9_23", 49],
  ["il7", "P9_14", 50],
  ["plane0", "P9_16", 51],
  ["plane1", "P8_35", 8],
  ["plane2", "P8_33", 9] ,
  ["plane3", "P8_31", 10],
  ["plane4", "P8_32", 11],
  ["plane5", "P9_20", 12],
  ["plane6", "P9_19", 13],
  ["plane7", "P9_26", 14],
  ["plane8", "P9_24", 15],
  ["addr_okay", "P9_31", 110],
  ["clear", "P9_29", 111],
  ["red", "P9_30", 112],
  ["r", "P9_22", 0],
  ["g", "P9_21", 0],
  ["b", "P8_34", 0],
]

def writefile(fname, val):
  with open(fname, 'w') as f:
    print >>f, val

# Setup directions
for label, pin, gpio in pins:
  try:
    if label in ['r', 'g', 'b']:
      PWM.start(pin, 0, 1000)
    elif label.startswith('sense') or label == 'red':
      writefile('/sys/class/gpio/gpio%d/direction' % gpio, 'out')
    else:
      writefile('/sys/class/gpio/gpio%d/direction' % gpio, 'in')
  except Exception, e:
    print pin, e
