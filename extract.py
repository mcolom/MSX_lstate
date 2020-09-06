#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# GNU General Public Licence (GPL)
#
# This program is free software; you can redistribute it and/or modify it under
# the terms of the GNU General Public License as published by the Free Software
# Foundation; either version 2 of the License, or (at your option) any later
# version.
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
# details.
# You should have received a copy of the GNU General Public License along with
# this program; if not, write to the Free Software Foundation, Inc., 59 Temple
# Place, Suite 330, Boston, MA  02111-1307  USA
#

import argparse
from xml.etree import ElementTree as ET
import base64, zlib
import os
import sys

def get_reg(root, name):
    '''
    Read a register's value given its name
    '''
    path = f"machine/cpu/z80/regs/{name}"
    val = root.findall(path)[0].text
    if val == 'true':
        return True
    if val == 'false':
        return True
    return int(val)

def print_regs(regs):
    for name, value in regs.items():
        if type(value) == int:
            print(f"{name}\t=\t0x{value:04x}")
        else:
            print(f"{name}\t=\t{value}")
            




parser = argparse.ArgumentParser()
parser.add_argument("filename", type=str, help="Input XML filename")
args = parser.parse_args()

tree = ET.parse(args.filename)
root = tree.getroot()

# Save RAM dump
ram = root.findall("machine/config/device[@type='Ram']/ram/ram[@encoding='gz-base64']")
ram_base64 = ram[0].text

decoded_data = zlib.decompress(base64.b64decode(ram_base64))
with open("ram.bin", "wb") as f:
    f.write(decoded_data)

# Save VDP registers
vregs = root.findall("machine/config/device[@type='VDP']/registers/")

with open("vregs.bin", "wb") as f:
    vregs_bytes = bytes([int(vreg.text) for vreg in vregs[0:8]])
    f.write(vregs_bytes)

# Save VRAM dump
vram = root.findall("machine/config/device[@type='VDP']/vram/data[@encoding='gz-base64']")
vram_base64 = vram[0].text

decoded_data = zlib.decompress(base64.b64decode(vram_base64))
with open("vram.bin", "wb") as f:
    f.write(decoded_data)

# Read and save CPU registers
reg_names = ['af', 'bc', 'de', 'hl', \
             'ix', 'iy', \
             'pc', 'sp', \
             'af2', 'bc2', 'de2', 'hl2']
regs = {}
for name in reg_names:
    regs[name] = get_reg(root, name)

print_regs(regs)

# Save registers
with open("regs.bin", "wb") as f:
    for name in reg_names:
        value = regs[name]
        z80_word = int.to_bytes(value, length=2, byteorder='little')
        f.write(z80_word)
