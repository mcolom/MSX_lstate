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

; Compile and test:
; z80asm rom.asm --label=symbols.txt -o rom.rom && openmsx -machine devel rom.rom

; z80asm rom.asm --label=symbols.txt -o rom.rom && ./stt2rom.py ~/.openMSX/savestates/bounder.oms && openmsx -machine devel rom.rom

; Include MSX BIOS and system variables definitions
include 'headers/bios.asm'


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

INTERRUPT_COPY: equ 0x50 ; Copy INTERRUPT_COPY bytes into the Z80's interrupt table
INTERRUPT_COPY_BYTES: equ 0x500 ; 0xff - 0x50 ; Make sure this is the size of our code

start:
    di
    ; Let's copy ourselves to page 0 (RAM)

    ; Choose this memory configuration
    ;       33221100
    ;       ││││││└┴─ Page 0 (#0000-#3FFF) --> 0 (RAM)
    ;       ││││└┴─── Page 1 (#4000-#7FFF) --> 1 (CART CODE)
    ;       ││└┴───── Page 2 (#8000-#BFFF) --> 1 (CART SEGMENT)
    ;       └┴─────── Page 3 (#C000-#FFFF) --> 3 (RAM)
    ld a, &b11010111 ; 3 1 1 3
    out (0xa8), a

    ; Copy ourselves to 0x50, in the Z80 interrupt table
    ld de, INTERRUPT_COPY
    ld hl, start
    ld bc, INTERRUPT_COPY_BYTES
    ldir
    
    jp START_REUBICATED_CODE
END_NON_REUBICATED_CODE: ; 4037

; Don't more with equ from here: the compiler needs to see the other variables before
START_REUBICATED_CODE: equ END_NON_REUBICATED_CODE - start + INTERRUPT_COPY

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;    ORG 0x63 = START_REUBICATED_CODE
ORG START_REUBICATED_CODE
    ; Current memory configuratset_two_segments_P8000ion
    ;       33221100
    ;       ││││││└┴─ Page 0 (#0000-#3FFF) --> 0 (RAM)
    ;       ││││└┴─── Page 1 (#4000-#7FFF) --> 1 (CART CODE)
    ;       ││└┴───── Page 2 (#8000-#BFFF) --> 1 (CART SEGMENT)
    ;       └┴─────── Page 3 (#C000-#FFFF) --> 3 (RAM)
    
    ; Page X: segment 2 + 2*X
    
    ; Load VRAM
    ld a, 11 ; Segment 11: VRAM
    call set_two_segments_P8000

    xor a
    ld b, a
    ld a, 14 + 128 ; Write 0 to reg_14
    
    ld c, 0x99
    out (c), b ; value
    out (c), a ; reg + 128

    ; a: reg + 128
    ; b: value

    ld c, 0x99
    xor a
    ld b, 64
    out (c), a
    out (c), b
    
    ;ld c, 0x98
    ld hl, 0x8000
    ld bc, 0x4000
    
    write_vram:
        ld a, (hl)
        out (0x98), a
        inc hl
        
        dec bc
        ld a, b
        or c
        jr nz, write_vram
        
    ; Load VDP registers
    ld a, 10 ; Segment 10: VDP registers
    ld (Seg_P8000_SW), a ; 0x8000 - 0x9FFF

    ld b, 8
    ld hl, 0x8000
    ld c, 128
    regs_write_loop:
        ld a, (hl)
        
        out (0x99), a
        ld a, c
        out (0x99), a
        
        inc c
        inc hl
        djnz regs_write_loop


    ld a, 2 ; Segment 2-3: game's page 0
    call set_two_segments_P8000

    ; Restore first part of the Z80's interrupt table
    ld de, 0x0
    ld hl, 0x8000
    ld bc, 0x40
    ldir
    
    ; Copy page 0
    CODE_SIZE: equ 0x200
    ; Don't overwrite our code! (we put 0x200 for the moment...)
    ld de, CODE_SIZE
    ld hl, 0x8000 + CODE_SIZE
    ld bc, 0x4000 - CODE_SIZE
    ldir
    
    ;;;

    ; Set this memory configuration
    ;       33221100
    ;       ││││││└┴─ Page 0 (#0000-#3FFF) --> 3 (RAM)
    ;       ││││└┴─── Page 1 (#4000-#7FFF) --> 3 (RAM)
    ;       ││└┴───── Page 2 (#8000-#BFFF) --> 1 (CART SEGMENT)
    ;       └┴─────── Page 3 (#C000-#FFFF) --> 3 (RAM)
    ld a, 11011111b
    out (0xa8), a
    
    ld a, 4 ; Segments 4-5: game's page 1
    call set_two_segments_P8000
    
    ; Copy page 1
    ld de, 0x4000
    ld hl, 0x8000
    ld bc, 0x4000
    ldir
    
    ;;;
    
    ld a, 8 ; Segments 8-9 game's page 3
    call set_two_segments_P8000
    
    ; Copy page 1
    ld de, 0xC000
    ld hl, 0x8000
    ld bc, 0x4000
    ldir
    
    ; Set this memory configuration
    ;       33221100
    ;       ││││││└┴─ Page 0 (#0000-#3FFF) --> 3 (RAM)
    ;       ││││└┴─── Page 1 (#4000-#7FFF) --> 1 (CART SEGMENT)
    ;       ││└┴───── Page 2 (#8000-#BFFF) --> 3 (RAM)
    ;       └┴─────── Page 3 (#C000-#FFFF) --> 3 (RAM)
    ld a, 11110111b
    out (0xa8), a

    ld a, 6 ; Segment 6-8 game's page 2
    ;ld (Seg_P4000_SW), a ; This doesn't seem to work :/
    ;inc a
    ld (Seg_P6000_SW), a

    ; Copy page 2
    ld de, 0x8000
    ld hl, 0x6000
    ld bc, 0x2000
    ldir
    
    inc a
    ld (Seg_P6000_SW), a

    ld de, 0xa000
    ld hl, 0x6000
    ld bc, 0x2000
    ldir


    ; Set this memory configuration
    ;       33221100
    ;       ││││││└┴─ Page 0 (#0000-#3FFF) --> 3 (RAM)
    ;       ││││└┴─── Page 1 (#4000-#7FFF) --> 3 (RAM)
    ;       ││└┴───── Page 2 (#8000-#BFFF) --> 3 (RAM)
    ;       └┴─────── Page 3 (#C000-#FFFF) --> 3 (RAM)
    ld a, 0xff
    out (0xa8), a

    ld sp, REGISTERS    
    pop af
    pop bc
    pop de
    pop hl
    pop ix
    pop iy

    LD_SP_CODE:
    ld sp, 0x03e0 ; To overwrite
    ; IFF1, IM, I ; [ToDo]
    
    IM_CODE:
    im 1 ; Overwrite to get the right IM
    
    EI_CODE:
    ei ; Overwrite with NOP if DI (IFF1 = 0)

    ; Jump to state's PC
    JUMP_TO_STATE_PC:
    jp 0x0000; To overwrite

    REGISTERS:
    ds 6*2, 0


write_vdp_reg:
    ; a: reg + 128
    ; b: value
    ld c, 0x99
    out (c), b ; value
    nop
    out (c), a ; reg + 128
    ret

set_two_segments_P8000:
    ; A: segment. It'll also choose segment A+1
    ld (Seg_P8000_SW), a ; 0x8000 - 0x9FFF
    inc a
    ld (Seg_PA000_SW), a ; 0xA000 - 0xBFFF
    ret


MAIN_CODE_END:
; Padding zeros to fill 0x4000 ... 0x7FFF
ds 0x4000 - (END_NON_REUBICATED_CODE - CART_START + MAIN_CODE_END - START_REUBICATED_CODE), 0

; Cart segments
org 0x8000

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
