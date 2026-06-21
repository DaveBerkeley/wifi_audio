#!/bin/env python

import time
import sys

print("log")

chans_range = [ 2 ]
bit_range = [ 16, 24, 32 ]
slot_range = [ 16, 24, 32 ]
freq_range = [ 48000, 96000 ]

for chan in chans_range:
    for bits in bit_range:
        for slots in slot_range:
            if slots < bits:
                continue
            for freq in freq_range:
                print (f"test {bits} {slots} {freq} {chan}")
                sys.stdout.flush()
                time.sleep(5)

print("exit")

# FIN
