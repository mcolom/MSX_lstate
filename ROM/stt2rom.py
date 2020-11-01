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

# run stt2rom.py ~/.openMSX/savestates/bounder.oms

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
    '''
    Print registers
    '''
    for name, value in regs.items():
        if type(value) == int:
            print(f"{name}\t=\t0x{value:04x}")
        else:
            print(f"{name}\t=\t{value}")

def get_segment_offset(seg):
    '''
    Obtain the offset of the given segment,
    from the start of the ROM file.
    '''
    return seg*0x2000

def read_all_symbols(filename):
    '''
    Read all the symbols produced by the assember
    '''
    symbols = {}

    with open(filename, "rt") as f:
        lines = f.readlines()
        for line in lines:
            name, _, addr_str = line.split()
            
            name = name.strip().replace(":", "")

            addr_str = addr_str.replace("$", "0x")
            addr = int(addr_str, 16)
            
            symbols[name] = addr
    return symbols

def get_reubicated_offset(position, END_NON_REUBICATED_CODE, CART_START, START_REUBICATED_CODE):
    '''
    Return the offset in the cartridge from a reubicated symbol
    '''
    return END_NON_REUBICATED_CODE - CART_START + position - START_REUBICATED_CODE

def create_all_RAM_cart(compile_rom):
    if compile_rom:
        os.system("z80asm rom_all_RAM.asm --label=symbols_all_RAM.txt -o rom.rom")
        
    symbols = read_all_symbols("symbols_all_RAM.txt")

    END_NON_REUBICATED_CODE = symbols["END_NON_REUBICATED_CODE"]
    CART_START = symbols["CART_START"]
    MAIN_CODE_END = symbols["MAIN_CODE_END"]
    START_REUBICATED_CODE = symbols["START_REUBICATED_CODE"]

    with open("rom.rom", "r+b") as f:
        # Read and save CPU registers
        reg_names = ['af', 'bc', 'de', 'hl', \
                     'ix', 'iy', \
                     'pc', 'sp', \
                     'af2', 'bc2', 'de2', 'hl2',
                     'iff1', 'im', 'i']
        regs = {}
        for name in reg_names:
            regs[name] = get_reg(root, name)

        print_regs(regs)
        registers_seek = get_reubicated_offset(symbols["REGISTERS"], END_NON_REUBICATED_CODE, CART_START, START_REUBICATED_CODE)
        f.seek(registers_seek)
        for register in ['af', 'bc', 'de', 'hl', 'ix', 'iy']:
            z80_word = int.to_bytes(regs[register], length=2, byteorder='little')
            f.write(z80_word)
            
        # Overwrite PC
        pc_seek = 1 + get_reubicated_offset(symbols["JUMP_TO_STATE_PC"], END_NON_REUBICATED_CODE, CART_START, START_REUBICATED_CODE)
        f.seek(pc_seek)
        z80_word = int.to_bytes(regs['pc'], length=2, byteorder='little')
        f.write(z80_word)
        
        # Overwrite SP
        sp_seek = 1 + get_reubicated_offset(symbols["LD_SP_CODE"], END_NON_REUBICATED_CODE, CART_START, START_REUBICATED_CODE)
        f.seek(sp_seek)
        z80_word = int.to_bytes(regs['sp'], length=2, byteorder='little')
        f.write(z80_word)
        

        # Save VDP registers
        vregs = root.findall("machine/config/device[@type='VDP']/registers/")
        vregs_bytes = bytes([int(vreg.text) for vreg in vregs[0:8]])
        
        vdp_registers_offset = get_segment_offset(10)
        f.seek(vdp_registers_offset)
        f.write(vregs_bytes)

        # Save VRAM
        vram = root.findall("machine/config/device[@type='VDP']/vram/data[@encoding='gz-base64']")
        vram_base64 = vram[0].text

        decoded_data = zlib.decompress(base64.b64decode(vram_base64))

        vram_segment = get_segment_offset(11)
        f.seek(vram_segment)
        f.write(decoded_data)

        # Save RAM
        ram = root.findall("machine/config/device[@type='Ram']/ram/ram[@encoding='gz-base64']")
        if not ram:
            ram = root.findall("machine/config/device[@type='MemoryMapper']/ram/ram[@encoding='gz-base64']")
        if not ram:
            raise NameError("Can't find RAM entry in savestate")

        ram_base64 = ram[0].text

        decoded_data = zlib.decompress(base64.b64decode(ram_base64))
        #decoded_data = decoded_data.replace(b'\x31\x00\x00', b'\x31\xff\xff') # Patch LD SP, 0x0000 with LD SP, 0xFFFF

        #if slot0 == 0 or slot1 == 0:
        #    with open("/usr/share/openmsx/systemroms/hb-20p_basic-bios1.rom", "rb") as rf:
        #        length = 16*1024 if slot0 else 32*1024
        #        rom_data = rf.read(length)
        #        decoded_data = rom_data + decoded_data[length:]

        f.seek(get_segment_offset(2))
        f.write(decoded_data)
        
        
        # Overwrite IM code
        im_seek = get_reubicated_offset(symbols["IM_CODE"], END_NON_REUBICATED_CODE, CART_START, START_REUBICATED_CODE)
        f.seek(im_seek)
        
        #if regs['im'] == 2:
        #    raise ValueError("IM 2 not yet supported")
        
        if regs['im'] == 0:
            f.write(b"\xed\x46")
        elif regs['im'] == 1:
            f.write(b"\xed\x56")
        elif regs['im'] == 2:
            f.write(b"\xed\x5e")
        else:
            raise ValueError(regs['im'])
        
        # Overwrite EI code
        ei_seek = get_reubicated_offset(symbols["EI_CODE"], END_NON_REUBICATED_CODE, CART_START, START_REUBICATED_CODE)
        f.seek(ei_seek)
        if regs['iff1']:
            f.write(b"\xfb") # EI
        else:
            f.write(b"\xf3") # DI
        
        print("openmsx -machine turbor rom.rom")


parser = argparse.ArgumentParser()
parser.add_argument("in_filename", type=str, help=".oms openMSX savestate. For example: ~/.openMSX/savestates/madmix.oms")
#parser.add_argument("out_filename", type=str, help="Output filename. For example: madmix.stt")
args = parser.parse_args()

compile_rom = True

with gzip.open(args.in_filename) as f:
    tree = ET.parse(f)

root = tree.getroot()

# Get primary slots configuration
primary_slots = int(root.findall("machine/cpuInterface/primarySlots")[0].text)
slot0 =  primary_slots & 0b00000011
slot1 = (primary_slots & 0b00001100) >> 2
slot2 = (primary_slots & 0b00110000) >> 4
slot3 = (primary_slots & 0b11000000) >> 6

rom_in_slots_01 = (slot0 == 0 or slot1 == 0)

if rom_in_slots_01:
    raise NotImplementedError("ROM is slot 0-1")
else:
    create_all_RAM_cart(compile_rom)



