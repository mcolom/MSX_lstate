// 
// 
// Load MSX1 state from openMSX in MSX2 machines
// (c) Miguel Colom, 2020
// GNU GPL license

#include "fusion-c/header/msx_fusion.h"
#include "fusion-c/header/io.h"
#include <stdio.h>

/*
lstate (c) 2020 Miguel Colom

GNU General Public Licence (GPL)

This program is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free Software
Foundation; either version 2 of the License, or (at your option) any later
version.
This program is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
details.
You should have received a copy of the GNU General Public License along with
this program; if not, write to the Free Software Foundation, Inc., 59 Temple
Place, Suite 330, Boston, MA  02111-1307  USA
*/

// Panasonic_FS-A1ST BAD
// Panasonic_FS-A1GT OK

//#define DEBUG_LSTATE

#define NUM_PUSHES 11
#define LEN_CODE 20

#define H_KEYI 0xFD9A
#define H_TIMI 0xFD9F

#define EXTBIO 0xFFCA

#define CALL_HL jp (hl)

// For some unknown reason, with the original PrintHex some games won't work
//#define PrintHex(s) printf("%x", s)

// Warning: execution fails when the buffers are put inside main.
// In main they're in the stack space, and here it's global.
char buffer[1024];

typedef struct {
    unsigned int af;
    unsigned int bc;
    unsigned int de;
    unsigned int hl;
    unsigned int ix;
    unsigned int iy;
    unsigned int pc;
    unsigned int sp;
    unsigned int af2;
    unsigned int bc2;
    unsigned int de2;
    unsigned int hl2;
    unsigned int iff1;
    unsigned int im;
    unsigned int i;
} Regs;

unsigned int initial_SP;

Regs regs;
unsigned char VDP_regs[8];
unsigned char slots;
unsigned char slot_p0, slot_p1;

unsigned char segment;
unsigned char segments[4];
int fH;
unsigned int i, j;

unsigned char *ptr;
unsigned char *ptr_origin;
unsigned char *from;
unsigned char *to;

unsigned char rom_selected_p0, rom_selected_p1;
unsigned char new_game_slots;

void init_files() {
    // It's mandatory to do this to use files!
    FCBs();
}

void print_slot_config() {
    // Memory mapper regs
    // https://www.msx.org/wiki/Memory_Mapper
    /*
    The MSX2 BIOS initializes memory mappers by setting up the following configuration:

    Segment 3 is set on page 0 (0000-3FFFh).
    Segment 2 is set on page 1 (4000-7FFFh).
    Segment 1 is set on page 2 (8000-BFFFh).
    Segment 0 is set on page 3 (C000-FFFFh).
    For software unaware of memory mappers, the default configuration above appears like a regular 64 KiB block of RAM.

        #FC (write)	Mapper segment for page 0 (#0000-#3FFF)
        #FD (write)	Mapper segment for page 1 (#4000-#7FFF)
        #FE (write)	Mapper segment for page 2 (#8000-#BFFF)
        #FF (write)	Mapper segment for page 3 (#C000-#FFFF)
    */

    // Primary slot reg
    unsigned char b = InPort(0xA8);
    printf("Primary slot reg (0xA8): %d\r\n", b);
    printf("Page 0, 0000-3FFF: %d\r\n",  b & 0b00000011      );
    printf("Page 1, 4000-7FFF: %d\r\n", (b & 0b00001100) >> 2);
    printf("Page 2, 8000-BFFF: %d\r\n", (b & 0b00110000) >> 4);
    printf("Page 3, C000-FFFF: %d\r\n", (b & 0b11000000) >> 6);
    printf("\r\n");

    // Secondary slot reg
    unsigned char *ptr = (unsigned char*)0xFFFF;
    unsigned char val = *ptr;
    unsigned char val_inv = val ^ 255;

    // http://map.grauw.nl/resources/msx_io_ports.php#subslot
    printf("Secondary slot reg (0xFFFF)\r\n");
    printf("Page 0, 0000-3FFF: subslot %d\r\n",  val_inv & 0b00000011      );
    printf("Page 1, 4000-7FFF: subslot %d\r\n", (val_inv & 0b00001100) >> 2);
    printf("Page 2, 8000-BFFF: subslot %d\r\n", (val_inv & 0b00110000) >> 4);
    printf("Page 3, C000-FFFF: subslot %d\r\n", (val_inv & 0b11000000) >> 6);
    printf("\r\n");
}

void main(char *argv[], int argc) {
  printf("Load MSX1 state\r\nGNU GPL by mcolom, 2020\r\n\r\n");
  
  if (argc != 1) {
      printf("Please specify the state file\r\n");
      Exit(0);
      return;
  }
  
  // Copy argv to buffer. It can't be done after init()
  NStrCopy(buffer, argv[0], sizeof(buffer)-1);
  
  init_files();
  fH = Open(buffer, O_RDONLY);
  
  // Read registers
  if (fH < 0) {
      printf("Can't open input file\r\n");
      Exit(1);
      return;
  }
  
  print_slot_config();
  
  // get current SP
  __asm
      ld (_initial_SP), SP
  __endasm;
  
  printf("Current SP=%x\r\n", initial_SP);


  //debug set_watchpoint read_io 0x2E
  //InPort(0x2E);
  
  Read(fH, &regs, sizeof(Regs));
  
  printf("af=%x, bc=%x, de=%x, hl=%x, ix=%x, iy=%x, pc=%x, sp=%x, ",
      regs.af, regs.bc, regs.de, regs.hl, regs.ix, regs.iy, regs.pc, regs.sp);

  printf("af2=%x, bc2=%x, de2=%x, hl2=%x, ",
      regs.af2, regs.bc2, regs.de2, regs.hl2);

  printf("iff1=%x, im=%x, i=%d\r\n",
      regs.iff1, regs.im, regs.i);
  
  // Read primary slots config
  Read(fH, &slots, sizeof(slots));
  rom_selected_p0 = (slots & 0b00000011) < 2;
  rom_selected_p1 = ((slots & 0b00001100) >> 2) < 2;
  
  printf("slots = %d\r\n", slots);
  printf("rom_selected_p0 = %d\r\n", rom_selected_p0);
  printf("rom_selected_p1 = %d\r\n", rom_selected_p1);
  
  // Don't attempt to put the ROM - DEBUG
  //rom_selected_p0 = 0;
  //rom_selected_p1 = 0;
  
  // Allocate segment if MSX DOS 2 or higher
  // Hardcode them otherwise

  #ifdef DEBUG_LSTATE
  printf("GetOSVersion() == %d\r\n", GetOSVersion());
  #endif

  if (GetOSVersion() >= 2) {
      // Allocate segments
      /*	Parameter:	A = 0
                D = 4 (device number of mapper support)
                E = 1
        Result:		A = slot address of primary mapper
                DE = reserved
                HL = start address of mapper variable table
      */
      __asm
          push af
          push de
          push hl
          push iy
          
          xor a // A=0
          ld de, #0x0402 // D=4, E=2
          call EXTBIO
          // Now we have in HL the address of the mapper call table
          
          xor a
          ld b, a // A=0, B=0
          CALL_HL // ALL_SEG
          
          ld iy, #_segments
          ld 0 (iy), a
          //
          xor a
          ld b, a // A=0, B=0
          CALL_HL // ALL_SEG
          
          ld iy, #_segments
          ld 1 (iy), a
          //
          xor a
          ld b, a // A=0, B=0
          CALL_HL // ALL_SEG
          
          ld iy, #_segments
          ld 2 (iy), a
          //
          xor a
          ld b, a // A=0, B=0
          CALL_HL // ALL_SEG

          ld iy, #_segments
          ld 3 (iy), a

          pop iy
          pop hl
          pop de
          pop af
      __endasm;
  }
  else {
      for (i = 0; i < 4; i++)
          segments[i] = i + 4;
  }

  // Read RAM
  for (i = 0; i < 4; i++) {
      segment = segments[i];
      
      #ifdef DEBUG_LSTATE
      printf("\r\n\r\n");
      #endif
      
      printf("Filling segment %d\r", segment);

      #ifdef DEBUG_LSTATE
      printf("\r\n");
      #endif

      to = (unsigned char *)0x8000;

      for (j = 0; j < 16*1024 / sizeof(buffer); j++) {
          Read(fH, buffer, sizeof(buffer));

          // Don't overwritte MSX-DOS variables area
          if (rom_selected_p0 && regs.im != 2 && segment == segments[3] && j >= 14) {
              from = (unsigned char *)(0xC000 + j*sizeof(buffer));
              #ifdef DEBUG_LSTATE
              printf("A");
              #endif
          }
          else {
              from = buffer; // If IM = 2 actually we don't care about overwritting
              #ifdef DEBUG_LSTATE
              printf("B");
              #endif
             }

          OutPort(0xFE, segment); // FE (write) Mapper segment for page 2 (#8000-#BFFF)
          MemCopy(to, from, sizeof(buffer));
          
          // If rom_selected_p0, we need to copy the
          // H.KEYI and H.TIMI hooks the game configured
          if (rom_selected_p0 && segment == segments[3] && j == 15) {
              MemCopy((unsigned char*)(to + H_TIMI % sizeof(buffer)), (unsigned char*)(buffer + H_TIMI % sizeof(buffer)), 3);
              MemCopy((unsigned char*)(to + H_KEYI % sizeof(buffer)), (unsigned char*)(buffer + H_KEYI % sizeof(buffer)), 3);
              #ifdef DEBUG_LSTATE
              printf("C");
              #endif

          }
          else
              #ifdef DEBUG_LSTATE
              printf("D");
              #endif
          
          to += sizeof(buffer);
      }
  }
  
  printf("\r\n");

  #ifdef DEBUG_LSTATE
  printf("\r\nPress a key...\r\n");
  Getche();
  #endif
  
  
  
  
  
  // Set the 8 VDP regs.
  Read(fH, VDP_regs, 8);
  for (i = 0; i < 8; i++)
      VDPwrite(i, VDP_regs[i]);

  // Zero VRAM
  unsigned char VRAM_Kb = GetVramSize();
  FillVram(0, 0, VRAM_Kb*1024);

  // Dump 64 Kb of VRAM
  for (i = 0; i < 16*1024; i += sizeof(buffer)) {
      Read(fH, buffer, sizeof(buffer));
      CopyRamToVram(buffer, i, sizeof(buffer));
  }

  Close(fH);

  // Put page 3 of the game (segments[3]]) in our page 3
  // Put page 2 of the game (segments[2]) in our page 2
  // Put page 1 of the game (segments[1]) in our page 1
  // Page 0 not yet, since it's where we're executing now!
  
  __asm
  di
  
  ld iy, #_segments
  ld a, 3 (iy)
  out (0xFF), a

  ld a, 2 (iy)
  out (0xFE), a

  ld a, 1 (iy)
  out (0xFD), a
 __endasm;
 
  // From here we SHOULD NOT use any functions which call MSXDOS.
  // Use the prints only for debugging!
 
  // Patch the original code on its page 3.
  // Be careful not to overwrite the stack!
  if (regs.sp >= 0xC000) {
      ptr_origin = (unsigned char *)regs.sp - (NUM_PUSHES)*2 - LEN_CODE; // In page 3: perfect, just adjust with respect to SP to prevent overlapping
      
      #ifdef DEBUG_LSTATE
      printf("A) Using ptr_origin = "); PrintHex((unsigned int)ptr_origin); printf("\r\n");
      #endif
      
      // If game's SP is too close to our MSX-DOS1 SP = initial_SP (= 0xDFC8 in tests), pick a different location for our code
      // In FPGA: D9F6
      if (regs.sp - initial_SP < 0x100) {
          #ifdef DEBUG_LSTATE
          printf("B) Using ptr_origin = "); PrintHex((unsigned int)ptr_origin); printf("\r\n");
          #endif
          
          ptr_origin = (unsigned char *)0xF000;
      }
  }
  else {
      ptr_origin = (unsigned char *)0xF000; // In a different page: we can't access it. Choose a high position in our page 3 and pray :D
      #ifdef DEBUG_LSTATE
      printf("PRAY, LOL\r\n");
      #endif

  }
  
  #ifdef DEBUG_LSTATE
  printf("C) Using ptr_origin = "); PrintHex((unsigned int)ptr_origin); printf("\r\n");
  #endif
  
  
  ptr = ptr_origin;
  
  // See: https://clrhome.org/table/  

  *ptr++ = 0xF5; // PUSH AF

  // If ROM selected, choose slot 0 in the corresponding pages
  new_game_slots = InPort(0xA8);
  if (rom_selected_p0) new_game_slots &= 0b11111100;
  if (rom_selected_p1) new_game_slots &= 0b11110011;
  
  
  #ifdef DEBUG_LSTATE
  printf("new_game_slots = %d\r\n", new_game_slots);
  #endif

  if (rom_selected_p0 || rom_selected_p1) {
      *ptr++ = 0x3E;
      *ptr++ = new_game_slots;
      // LD A, new_game_slots

      // Set primary slot selector
      *ptr++ = 0xD3;
      *ptr++ = 0xA8;
      // OUT (0xA8), A

      // Set segment 0 for page 0 and 1
      *ptr++ = 0xAF; // XOR A
      //
      
      // Set segment 0 for page 0
      if (rom_selected_p0) {
          *ptr++ = 0xD3;
          *ptr++ = 0xFC; // OUT (0xFC), A
      }
      // Set segment 0 for page 1
      if (rom_selected_p1) {
          *ptr++ = 0xD3;
          *ptr++ = 0xFD; // OUT (0xFD), A
      }
  } else {
      // Set segments[0] for page 0
      *ptr++ = 0x3E;
      *ptr++ = segments[0]; // LD A, START_SEGMENT
      //
      *ptr++ = 0xD3;
      *ptr++ = 0xFC; // OUT (0xFC), A
  }
  
  *ptr++ = 0xF1; // POP AF
  
  if (regs.im == 2) {
      *ptr++ = 0xED;
      *ptr++ = 0x5E; ; // IM 2
  }
  
  if (regs.iff1 == 1)
      *ptr++ = 0xFB; // EI

  *ptr++ = 0xC3;
  *ptr++ = (char)(((unsigned int)regs.pc & 0x00FF));
  *ptr++ = (char)(((unsigned int)regs.pc & 0xFF00) >> 8);
  // JP to the game's original PC
  

// Prepare registers and jump to our code in game's page.
// There are NUM_PUSHES pushes
InPort(0x2E); // DEBUG
__asm
  ld sp, (_regs + 7*2)  // SP
  
  ld a, (_regs + 14*2) // I
  ld i, a

  ld bc, (_ptr_origin) // our ret address to the second step in page 3
  push bc // fffb --> fff9

  ld bc, (_regs + 0*2) // AF
  push bc // --> fff7
  ld bc, (_regs + 1*2) // BC
  push bc  // --> fff5
  ld bc, (_regs + 2*2) // DE
  push bc // --> fff3
  ld bc, (_regs + 3*2) // HL
  push bc  // --> fff1
  ld bc, (_regs + 4*2) // IX
  push bc  // --> ffef
  ld bc, (_regs + 5*2) // IY
  push bc  // --> ffed
  
  // Shadow registers
  exx
  ex af,af'
  //
  ld bc, (_regs + 8*2) // AF'

  push bc  // --> ffeb
  ld bc, (_regs + 9*2) // BC'
  push bc  // --> ffe9
  ld bc, (_regs + 10*2) // DE'
  push bc  // --> ffe7
  ld bc, (_regs + 11*2) // HL'
  push bc  // --> ffe5
  //
  pop hl
  pop de
  pop bc
  pop af
  //
  ex af,af'
  exx
  
  pop iy
  pop ix
  pop hl
  pop de
  pop bc
  pop af
  
  ret
 __endasm;
}
