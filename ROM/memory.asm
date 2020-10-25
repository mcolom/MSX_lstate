; ¡Cuidado, Nobukado!
; Under GNU-GPL by Miguel Colom. See LICENSE file for details.

; Memory routines


Seg_P4000_SW: equ 0x4000
Seg_P6000_SW: equ 0x6000
;
Seg_P8000_SW: equ 0x8000
Seg_PA000_SW: equ 0xA000

; ********************************
; * Init memory slots            *
; ********************************
INIT_MEMORY:
    ; Choose this memory configuration
    ;       33221100
    ;       ││││││└┴─ Page 0 (#0000-#3FFF) --> 0 (BIOS)
    ;       ││││└┴─── Page 1 (#4000-#7FFF) --> 1 (CART CODE)
    ;       ││└┴───── Page 2 (#8000-#BFFF) --> 1 (CART SEGMENT)
    ;       └┴─────── Page 3 (#C000-#FFFF) --> 3 (RAM)
    push af
    ld a, &b11010100 ; 3 1 1 0
    call WSLREG
    pop af
    ret

; ****************************************************
; * Change segments at 0x8000-9FFF and 0xA000-0xBFFF *
; * Input A: segment (2, 3, ...).                    *
; *                                                  *
; * Segments 0 and 1 correspond to 0x4000-0x5FFF and *
; * 0x6000-0x7FFF of the main code.                  *
; ****************************************************
CHANGE_SEGMENTS:
    di
    push af
    ld (Seg_P8000_SW), a ; 0x8000 - 0x9FFF
    inc a
    ld (Seg_PA000_SW), a ; 0xA000 - 0xBFFF
    pop af
    ei
    ret
