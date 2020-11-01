; lstate (c) 2020 Miguel Colom
;
; GNU General Public Licence (GPL)
;
; This program is free software; you can redistribute it and/or modify it under
; the terms of the GNU General Public License as published by the Free Software
; Foundation; either version 2 of the License, or (at your option) any later
; version.
; This program is distributed in the hope that it will be useful, but WITHOUT
; ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
; FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
; details.
; You should have received a copy of the GNU General Public License along with
; this program; if not, write to the Free Software Foundation, Inc., 59 Temple
; Place, Suite 330, Boston, MA  02111-1307  USA

; Include MSX BIOS and system variables definitions
include 'headers/bios.asm'

; z80asm rom_with_system.asm --label=symbols_with_system.txt -o rom.rom && openmsx -machine turbor rom.rom

org 0x4000
CART_START:

; MSX Cartridge header
defb     $41
defb     $42
defw     start
defw     $0000
defw     $0000
defw     $0000
defw     $0000
defw     $0000
defw     $0000

include 'memory.asm'

start:
    di
    
    call set_subslots
    
    ; Set the following configuration:
    ;
    ; Page 0: RAM segments
    ; Page 1: CART code
    ; Page 2: CART segments
    ; page 3: RAM segments
    call prepare_slots
    
    ; Copy page 1 (ROM) to page 0 (RAM), segment 2
    ld de, 0x0
    ld hl, 0x4000
    ld bc, 0x4000
    ldir
    ;
    call rom2ram + 0x4000 ; + 0x4000 to execute in page 3
    
    ld sp, MY_STACK_END
    
    ; Now page 2 (our code) is RAM
    
    ; Page 0: RAM segments
    ; Page 1: CART segments
    ; Page 2: CART code
    ; page 3: RAM segments
    



    ; *** Page 0: RAM segment 4
    
    ld a, (SLOTS)
    and 00000011b
    
    ld a, 1
    or a ; DEBUG
    
    jr nz, no_rom_p0
    
    ; ROM in page 0. Copy our own ROM
    ld a, 4
    out (0xfc + 3), a ; Segment 4 in page 3

    in a, (0xa8)
    push af ; Backup original slot config

    and 11111100b
    out (0xa8), a
    
    ld hl, 0
    ld de, 0xc000
    ld bc, 0x4000 - 1
    ldir
    
    pop af ; Recover original slot config
    out (0xa8), a 

no_rom_p0:
    ; No ROM selected in page 0. Copy game's page 0
    ld a, 4
    out (0xfc + 0), a ; Segment 4 in page 0


    ld a, 2
    ld de, 0x0
    call ldir_two_segments
    

no_rom_p1:
    ; No ROM selected in page 1. Copy game's page 1
    ld a, 
    out (0xfc + 0), a ; Segment 4 in page 0


    ld a, 2
    ld de, 0x0
    call ldir_two_segments

    jp $
    
    
    
    ;in a, (0xa8)
    ;and 00111111b
    ;or  01000000b
    ;out (0xa8), a; Set page 3 to ROM also
    
    
    
    
    ; Choose this memory configuration
    ;       33221100
    ;       ││││││└┴─ Page 0 (#0000-#3FFF) --> 3 (RAM)
    ;       ││││└┴─── Page 1 (#4000-#7FFF) --> 1 (CART SEGMENT)
    ;       ││└┴───── Page 2 (#8000-#BFFF) --> 1 (CART CODE)
    ;       └┴─────── Page 3 (#C000-#FFFF) --> 3 (RAM)
    in a, (0xa8)
    and 11110000b ; Keep only pages 3 and 2
    ld b, a       ; B contains only original pages 3 and 2
    
    ld a, (SLOTS)
    and 00001111b ; A contains only chosen pages 1 and 0
    
    or b ; A = original pages 3 and 2 + chosen pages 1 and 0
    
    out (0xa8), a
    
    ; Now the ROM(s) is selected in page 0 and perhaps also page 1.
    ; Page 2 is the cartridge, and page 3 is RAM
    
    ; We need to convert page 2 (ROM) into RAM, with the same contents

    
    
    
rom2ram:
    in a, (0xa8)
    
    and 11000000b
    srl a
    srl a
    srl a
    srl a
    ld b, a ; B = Slot of page 3 moved to page 1 bits

    in a, (0xa8)
    and 11110011b
    or b ; Put slot of page 3 in page 1
    out (0xa8), a
    
    ; Copy data back, now in RAM
    ld de, 0x4000
    ld hl, 0x0
    ld bc, 0x4000
    ldir
    ret

set_subslots:
    ; Obtain which subslot is selected in page 3, and set the same for the
    ; rest of the sublots.

    ld a, (SLTTBL + 3) ; Bits 7-6 = Extended slot on page 3 (C000h~FFFFh)
    
    or a ; Clear carry
    
    srl a
    srl a
    srl a
    srl a
    srl a
    srl a ; 6 bits to the right
    
    ld b, a
    
    sla a
    sla a
    or b
    
    sla a
    sla a
    or b
    
    sla a
    sla a
    or b
    
    
    ;push af
    ;ld a, (SLOTS)
    ;and 00001111b ; Consider only pages 1 and 0
    ;ld b, a
    ;pop af
    ;and b
    
    ;and 11110000b ; Subslot 0 in pages 0 and 1        SLOTS: db 10100000b
    
    ld (0xFFFF), a
    ret
    
prepare_slots:
    ; Set the following configuration:
    ;
    ; Page 0: RAM segments
    ; Page 1: CART code
    ; Page 2: CART segments
    ; page 3: RAM segments 
    in a, (0xa8) ; 33.22.11.00
    srl a
    srl a
    srl a
    srl a
    srl a
    srl a ; 6 bits to the right
    ld b, a ; B = 000000SS, slot of page 3

    in a, (0xa8)
    and 11111100b    
    or b ; A = ooooooSS (o = original)
    out (0xa8), a ; Page 0 has now the same RAM slot than page 3
    
    ; Now put page 2 in the same ROM slot
    
    in a, (0xa8)
    and 00001100b
    srl a
    srl a
    
    ld b, a ; B = 000000SS, slot of page 1
    
    in a, (0xa8)
    and 11001111b
    sla b
    sla b
    sla b
    sla b
    or b ; A = ooSSoooo (o = original)
    out (0xa8), a
    ret
    
set_two_segments_P8000:
    ; A: segment. It'll also choose segment A+1
    ld (Seg_P8000_SW), a ; 0x4000 - 0x5FFF
    inc a
    ld (Seg_P8000_SW), a ; 0x6000 - 0x7FFF
    ret

ldir_two_segments:
    ; a, a+1: segments
    ; de: destination
    ; hl: origin
    ; bc: number of bytes
    call set_two_segments_P8000
    ld hl, Seg_P8000_SW
    ld bc, 0x4000
    ldir
    ret

    ds 10, 0
    MY_STACK_END:
    
    SLOTS: db 10100000b
    
    BUFFER_0: db 0



MAIN_CODE_END:
; Padding zeros to fill 0x8000 ... 0xBFFF
ds 0xA000 - $, 0

; Cart segments
org 0xA000

; Each segment is 8 Kb

; segment 0: 0x4000 - 0x5FFF
; segment 1: 0x6000 - 0x7FFF
; segment 2
ds 0x2000, 2 ; Game's page 0
; segment 3
ds 0x2000, 3 ; Game's page 0
; segment 4
ds 0x2000, 4 ; Game's page 1
; segment 5
ds 0x2000, 5 ; Game's page 1
; segment 6
ds 0x2000, 6 ; Game's page 2
; segment 7
ds 0x2000, 7 ; Game's page 2
; segment 8
ds 0x2000, 8 ; Game's page 3
; segment 9
ds 0x2000, 9 ; Game's page 3
; segment 10
ds 0x2000, 0x0a ; VDP registers
; segment 11
ds 0x2000, 0x0b ; VRAM
; segment 12
ds 0x2000, 0x0c ; VRAM
