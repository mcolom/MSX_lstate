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

//#define DEBUG
#define NUM_PUSHES 11
#define LEN_CODE 20

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
} Regs;

Regs regs;
unsigned char VDP_regs[8];
unsigned char slots;
unsigned char slot_p0, slot_p1;

int segment;
int fH;
unsigned int i, j;

unsigned char *ptr;
unsigned char *ptr_origin;
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
  
  // In Panasonic_FS-A1GT (Turbo-R) and OCM the RAM is in 3-0.
  // [ToDo] Use a RAM detection routine
  //ptr = (unsigned char*)0xFFFF;
  //*ptr = 0; // Choose subslot 0 everywhere
  
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

  //debug set_watchpoint read_io 0x2E
  //InPort(0x2E);
  
  Read(fH, &regs, sizeof(Regs));
  
  printf("af=");   PrintHex(regs.af);
  printf(", bc="); PrintHex(regs.bc);
  printf(", de="); PrintHex(regs.de);
  printf(", hl="); PrintHex(regs.hl);
  //
  printf(", ix="); PrintHex(regs.ix);
  printf(", iy="); PrintHex(regs.iy);
  printf(", pc="); PrintHex(regs.pc);
  printf(", sp="); PrintHex(regs.sp);
  //
  printf(", af2="); PrintHex(regs.af2);
  printf(", bc2="); PrintHex(regs.bc2);
  printf(", de2="); PrintHex(regs.de2);
  printf(", hl2="); PrintHex(regs.hl2);
  printf("\r\n");
  
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

  // Read RAM
  for (segment = 10; segment < 14; segment++) {
      printf("Filling segment %d\r", segment);
      OutPort(0xFE, 1); // Restore segment 1 in page 2 (#8000-#BFFF)
      
      to = (unsigned char *)0x8000;
      for (i = 0; i < 16*1024 / sizeof(buffer); i++) {
          Read(fH, buffer, sizeof(buffer));

          OutPort(0xFE, segment); // FE (write) Mapper segment for page 2 (#8000-#BFFF)

          MemCopy(to, buffer, sizeof(buffer));
          to += sizeof(buffer);
      }
  }

  // Zero VRAM
  #ifndef DEBUG
  unsigned char VRAM_Kb = GetVramSize();
  FillVram(0, 0, VRAM_Kb*1024);
  SetBorderColor(1);

  // Set the 8 VDP regs.
  Read(fH, VDP_regs, 8);
  for (i = 0; i < 8; i++)
      VDPwrite(i, VDP_regs[i]);

  // Dump 64 Kb of VRAM
  for (i = 0; i < 16*1024; i += 1024) {
      Read(fH, buffer, 1024);
      CopyRamToVram(buffer, i, 1024);
  }
  #endif
  
  
  Close(fH);

  // Put page 3 of the game (segment 13) in our page 3
  // Put page 2 of the game (segment 12) in our page 2
  // Put page 1 of the game (segment 11) in our page 1
  // Page 0 not yet, since it's where we're executing now!
  #ifndef DEBUG
  __asm
  di

  ld a, #13
  out (0xFF), a

  ld a, #12
  out (0xFE), a

  ld a, #11
  out (0xFD), a
 __endasm;
 #endif
 
  // Patch the original code on its page 3.
  // Be careful not to overwrite the stack!
  if (regs.sp >= 0xC000) {
      #ifdef DEBUG
      printf("A) Using ptr_origin = "); PrintHex((unsigned int)ptr_origin); printf("\r\n");
      #endif
      
      //ptr_origin = (unsigned char *)regs.sp - 38; // In page 3: perfect, just adjust with respect to SP to prevent overlapping
      ptr_origin = (unsigned char *)regs.sp - (NUM_PUSHES)*2 - LEN_CODE; // In page 3: perfect, just adjust with respect to SP to prevent overlapping
      
      #ifdef DEBUG
      printf("B) Using ptr_origin = "); PrintHex((unsigned int)ptr_origin); printf("\r\n");
      #endif
      
      // If game's SP is too close to our MSX-DOS1 SP = 0xDFC8, pick a different location for our code
      if (regs.sp - 0xDFC8 < 0x100) {
          #ifdef DEBUG
          printf("C) Using ptr_origin = "); PrintHex((unsigned int)ptr_origin); printf("\r\n");
          #endif
          
          ptr_origin = (unsigned char *)0xFFE0;
      }
  }
  else {
      ptr_origin = (unsigned char *)0xFFE0; // In a different page: we can't access it. Choose a high position in our page 3 and pray :D
  }
  
  //ptr_origin = (unsigned char *)0x3000; // DEBUG
  
  #ifdef DEBUG
  printf("D) Using ptr_origin = "); PrintHex((unsigned int)ptr_origin); printf("\r\n");
  #endif
  
  
  ptr = ptr_origin;
  
  // See: https://clrhome.org/table/  
  *ptr++ = 0; // NOP - to store A
  
  *ptr++ = 0x32;
  *ptr++ = (char)(((unsigned int)ptr_origin & 0x00FF));
  *ptr++ = (char)(((unsigned int)ptr_origin & 0xFF00) >> 8);
  // LD (ptr_origin), A
  
  
  // If ROM selected, choose slot 0 in the corresponding pages
  new_game_slots = InPort(0xA8);
  if (rom_selected_p0) new_game_slots &= 0b11111100;
  if (rom_selected_p1) new_game_slots &= 0b11110011;
  
  
  #ifdef DEBUG
  printf("new_game_slots = %d\r\n", new_game_slots);
  #endif
  
  #ifdef DEBUG
  return;
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
      // Set segment 10 for page 0
      *ptr++ = 0x3E;
      *ptr++ = 10; // LD A, 10
      //
      *ptr++ = 0xD3;
      *ptr++ = 0xFC; // OUT (0xFC), A
  }

  *ptr++ = 0x3A;
  *ptr++ = (char)(((unsigned int)ptr_origin & 0x00FF));
  *ptr++ = (char)(((unsigned int)ptr_origin & 0xFF00) >> 8);
  // LD A, (ptr_origin)

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
