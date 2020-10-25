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

; Include MSX BIOS and system variables definitions
include 'headers/bios.asm'


org 0x4000
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

; Print string @DE
;PRINT_STR:
;    push af
;print_str_loop:
;	ld a,(de)
;	inc de
;	or a
;	jp z, print_str_ret
;	call CHPUT
;	jr print_str_loop
;print_str_ret:    
;    pop af
;    ret

;MESSAGE_CUIDADO_NOBUKADO:
;defb 0xAD ; Interrogación invertida
;defb "Cuidado, Nobukado!", 0


;ld bc, 0x1234 ; PC
;push bc
;ld bc, 0x1234 ; SP
;push bc

;ld SP, 0x5678
;JP 0x2345


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
    ld de, 0x50
    ld hl, start
    ld bc, 0x500 ; 0xff - 0x50 ; Make sure this is the size of our code
    ldir
    
    jp 0x63
    
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;    ORG 0x63

    ; Current memory configuration
    ;       33221100
    ;       ││││││└┴─ Page 0 (#0000-#3FFF) --> 0 (RAM)
    ;       ││││└┴─── Page 1 (#4000-#7FFF) --> 1 (CART CODE)
    ;       ││└┴───── Page 2 (#8000-#BFFF) --> 1 (CART SEGMENT)
    ;       └┴─────── Page 3 (#C000-#FFFF) --> 3 (RAM)
    
    ; Page X: segment 2 + 2*X
    
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
    

    ; Load VRAM
    ld a, 11 ; Segment 10: VDP registers
    ld (Seg_P8000_SW), a ; 0x8000 - 0x9FFF
    inc a
    ld (Seg_PA000_SW), a ; 0xA000 - 0xBFFF

    xor a
    out (0x99), a
    ld a, 14 + 128
    out (0x99), a
    xor a
    out (0x99), a
    or 64
    out (0x99), a
    
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
        
    


    

    ld a, 2 ; Segment 2-3: game's page 0
    ld (Seg_P8000_SW), a ; 0x8000 - 0x9FFF
    inc a
    ld (Seg_PA000_SW), a ; 0xA000 - 0xBFFF

    ; Restore first part of the Z80's interrupt table
    ld de, 0x0
    ld hl, 0x8000
    ld bc, 0x40
    ldir
    
    ; Copy page 0
    ; Don't overwrite our code! (we put 0x200 for the moment...)
    ld de, 0x200
    ld hl, 0x8200
    ld bc, 0x4000 - 0x200
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
    ld (Seg_P8000_SW), a
    inc a
    ld (Seg_PA000_SW), a
    
    ; Copy page 1
    ld de, 0x4000
    ld hl, 0x8000
    ld bc, 0x4000
    ldir
    
    ;;;
    
    ld a, 8 ; Segments 8-9 game's page 3
    ld (Seg_P8000_SW), a
    inc a
    ld (Seg_PA000_SW), a
    
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
    ;ld (Seg_P4000_SW), a ; This doesn't seem to work :(
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
    pop iy
    pop ix
    pop hl
    pop de
    pop bc
    pop af

    LD_SP_CODE:
    ld sp, 0x03e0 ; To overwrite
    ; IFF1, IM, I ; [ToDo]
    
    IM_CODE:
    im 2 ; Overwrite with NOPs if IM == 1
    
    EI_CODE:
    ei ; Overwrite with NOP if DI

    ; Jump to state's PC
    JUMP_TO_STATE_PC:
    jp 0x29bd; To overwrite


    REGISTERS:
    dw 0x5014
    dw 0x5098
    dw 0x01a7
    dw 0x7608
    dw 0xd967
    dw 0x5c3a
    ;ds 6*2, 0
    REGISTERS_END:


MAIN_CODE_END:
; Padding zeros to fill 0x4000 ... 0x7FFF
ds 0x8000 - MAIN_CODE_END, 0

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
