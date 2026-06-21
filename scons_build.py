#!/bin/env python

import os
from pathlib import Path

Import("env")

if env['BOARD_MCU'] in [ "esp32", "esp32s3" ]:
    target = "xtensa-esp32s3"
    cross_cflags = [
        '-mlongcalls', # Xtensa fix for far calls
        #'-Os', # small code
        #'-flto', # link time optimisation
        #'-ffunction-sections',
        #'-fdata-sections',
        #'-Wl,--gc-sections',
    ]
elif env['BOARD_MCU'] in [ "esp32c3", "esp32c6" ]:
    target = "riscv32-esp"
    cross_cflags = [ ]

print("Building library for", target)

toolchain = f"toolchain-{target}"

pio_path = f"~/.platformio/packages/{toolchain}/bin"
tool_prefix = f"{pio_path}/{target}-elf-"

env.SConscript("third_party/SConscript.opus", exports="env tool_prefix cross_cflags")

#   FIN
