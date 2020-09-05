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



parser = argparse.ArgumentParser()
#parser.add_argument("-i", "--integration", help="Use the Integration environment", action="store_true")
#parser.add_argument("-l", "--local", help="Use the Local ({}) environment".format(LOCAL_IP), action="store_true")
#parser.add_argument("command", nargs='+')
args = parser.parse_args()

tree = ET.parse('madmix.xml')
root = tree.getroot()

#ram = root.findall("./machine/config/device[@type='Ram']")
ram = root.findall("./machine/config/device[@type='Ram']/ram/ram[@encoding='gz-base64']")
ram = ram[0]
ram_base64 = ram.text

decoded_data = zlib.decompress(base64.b64decode(ram_base64))

with open("ram.bin", "wb") as f:
    f.write(decoded_data)

##########

vram = root.findall("./machine/config/device[@type='VDP']/vram/data[@encoding='gz-base64']")
vram = vram[0]
vram_base64 = vram.text

decoded_data = zlib.decompress(base64.b64decode(vram_base64))

with open("vram.bin", "wb") as f:
    f.write(decoded_data)

