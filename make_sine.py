#!/bin/env python

import math
import struct
import wave

name = "sine.wav"

f = wave.open(name, "wb")

f.setnchannels(2)
f.setsampwidth(2)
f.setframerate(48000)

gain = 0x4000

for i in range(10000000):
    angle = i * 0.1
    s = math.sin(angle) * gain
    sample = int(s)

    # Little endian. of course it is.
    b = struct.pack("<hh", sample, sample)
    f.writeframes(b)

f.close()

i#print(f.getparams())

# FIN
