#!/bin/bash
cd "Fusion-C-v1.2/Working Folder"

sdcc --code-loc 0x106 --data-loc 0x0 --disable-warning 196 -mz80 --no-std-crt0 --opt-code-size fusion.lib -L fusion-c/lib/ fusion-c/include/crt0_msxdos.rel lstate.c && hex2bin -e com lstate.ihx && cp lstate.com dsk_miguel/l.com && openmsx -machine msx2 -diska dsk_miguel &
exit

# Compile C code and convert to MSX-DOS COM program
sdcc --code-loc 0x106 --data-loc 0x0 --disable-warning 196 -mz80 --no-std-crt0 --opt-code-size fusion.lib -L fusion-c/lib/ fusion-c/include/crt0_msxdos.rel lstate.c && hex2bin -e com lstate.ihx

# Copy COM program to openMSX's dir-as-disk file
cp lstate.com dsk_miguel/l.com

# Launch emulator
openmsx -machine msx2 -diska dsk_miguel &

