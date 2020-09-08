#!/bin/bash
#cp *.stt Fusion-C-v1.2/Working\ Folder/dsk_miguel/

cd "Fusion-C-v1.2/Working Folder"

sdcc --nogcse --noinvariant --noinduction --noloopreverse --no-peep      --code-loc 0x180 --data-loc 0x0 --disable-warning 196 -mz80 --no-std-crt0 fusion.lib -L fusion-c/lib/ fusion-c/include/crt0_msxdos_advanced.rel lstate.c && hex2bin -e com lstate.ihx && cp lstate.com dsk_miguel/l.com  #&& openmsx -machine turbor -diska dsk_miguel &

echo "openmsx -machine turbor -diska Fusion-C-v1.2/Working\ Folder/dsk_miguel/"
