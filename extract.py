#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# lstate (c) 2020 Miguel Colom
#
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


import argparse
from xml.etree import ElementTree as ET
import gzip
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
parser.add_argument("in_filename", type=str, help=".oms openMSX savestate. For example: ~/.openMSX/savestates/madmix.oms")
parser.add_argument("out_filename", type=str, help="Output filename. For example: madmix.stt")
args = parser.parse_args()

with gzip.open(args.in_filename) as f:
    tree = ET.parse(f)

root = tree.getroot()

with open(args.out_filename, "wb") as f:
    # Read and save CPU registers
    reg_names = ['af', 'bc', 'de', 'hl', \
                 'ix', 'iy', \
                 'pc', 'sp', \
                 'af2', 'bc2', 'de2', 'hl2',
                 'iff1']
    regs = {}
    for name in reg_names:
        regs[name] = get_reg(root, name)

    print_regs(regs)

    for name in reg_names:
        value = regs[name]
        z80_word = int.to_bytes(value, length=2, byteorder='little')
        f.write(z80_word)

    # Save primary slots configuration
    primary_slots = int(root.findall("machine/cpuInterface/primarySlots")[0].text)
    z80_byte = int.to_bytes(primary_slots, length=1, byteorder='little')
    f.write(z80_byte)

    # Save RAM
    ram = root.findall("machine/config/device[@type='Ram']/ram/ram[@encoding='gz-base64']")
    if not ram:
        ram = root.findall("machine/config/device[@type='MemoryMapper']/ram/ram[@encoding='gz-base64']")
    if not ram:
        raise NameError("Can't find RAM entry in savestate")

    ram_base64 = ram[0].text

    decoded_data = zlib.decompress(base64.b64decode(ram_base64))
    decoded_data = decoded_data.replace(b'\x31\x00\x00', b'\x31\xff\xff') # Patch LD SP, 0x0000 with LD SP, 0xFFFF
    f.write(decoded_data)
    
    subslot_p0 = (decoded_data[-1] & 0b00000011)
    subslot_p1 = (decoded_data[-1] & 0b00001100) >> 2
    
    # Check if PC or SP point to ROM
    PC = int(regs['pc'])
    SP = int(regs['sp'])
    if subslot_p0 == 0:
        if PC < 0x4000:
            print(f"Warning: ROM in page 0 and PC = 0x{PC:04x}")
        if SP < 0x4000:
            print(f"Warning: SP = 0x{SP:04x} in ROM page 0")
    if subslot_p1 == 0:
        if 0x4000 <= PC < 0x8000:
            print(f"Warning: ROM in page 1 and PC = 0x{PC:04x}")
        if 0x4000 <= SP < 0x8000:
            print(f"Warning: SP = 0x{SP:04x} in ROM page 1")

    # Save VDP registers
    vregs = root.findall("machine/config/device[@type='VDP']/registers/")
    vregs_bytes = bytes([int(vreg.text) for vreg in vregs[0:8]])
    f.write(vregs_bytes)

    # Save VRAM
    vram = root.findall("machine/config/device[@type='VDP']/vram/data[@encoding='gz-base64']")
    vram_base64 = vram[0].text

    decoded_data = zlib.decompress(base64.b64decode(vram_base64))
    f.write(decoded_data)
