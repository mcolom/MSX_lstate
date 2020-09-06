// 
// Load MSX1 state from openMSX in MSX2 machines
// (c) Miguel Colom, 2020
// GNU GPL license

#include "fusion-c/header/msx_fusion.h"
#include "fusion-c/header/io.h"
#include <stdio.h>

void ensamblador(void)
{
__asm

ld h,#0
ld l,#4
ld b,#0x12
ld c,#0x16
//call 0x1234
jp 0
ret

__endasm;
}

void init() {
    // It's mandatory to do this to use files!
    FCBs();
}

int open_file(const char *filename) {
    int fH = Open(filename, O_RDONLY);
    if (fH < 0) {
        printf("File not found: %s\r\n", filename);
    }
    return fH;
}

void delay() {
    printf("START delay...\r\n");
    for (unsigned int i = 0; i < 60000; i++) {
        for (unsigned int j = 0; j < 1; j++) {
          __asm
          nop
         __endasm;
        }
    }
    printf("END delay...\r\n");
}

// Panasonic_FS-A1ST BAD
// Panasonic_FS-A1GT OK

// Warning: execution fails when the buffers are put inside main (stack overflow?)
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

int segment;
int fH;
unsigned int i, j;

unsigned char *ptr;
unsigned char *ptr_origin;
unsigned char *to;

void main(void) 
{
  //init();
  FCBs();

  printf("Load MSX1 state\r\n\GNU GPL By Miguel Colom, 2020\r\n\r\n");
  
  // Primary slot reg
  unsigned char b = InPort(0xA8);
  printf("Primary slot reg (0xA8)\r\n");
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

  // In Panasonic_FS-A1GT (Turbo-R) and OCM the RAM is in 3-0.
  // [ToDo] Use a RAM detection routine
  //ptr = (unsigned char*)0xFFFF;
  //*ptr = 0; // Choose subslot 0 everywhere
  
  // Read registers
  fH = open_file("regs.bin");

  //debug set_watchpoint read_io 0x2E
  //InPort(0x2E);
  
  Read(fH, &regs, sizeof(Regs));
  Close(fH);
  
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
  
  fH = open_file("ram.bin");
  for (segment = 10; segment < 14; segment++) {
      printf("Filling segment %d\r", segment);
      OutPort(0xFE, 1); // Restore segment 1 in page 2 (#8000-#BFFF)
      
      to = (unsigned char *)0x8000;
      for (i = 0; i < 16; i++) {
          Read(fH, buffer, sizeof(buffer));

          OutPort(0xFE, segment); // FE (write) Mapper segment for page 2 (#8000-#BFFF)

          for (j = 0; j < sizeof(buffer); j++) {
              to[j] = buffer[j];
          }
          to += sizeof(buffer); // sdcc seems to wrongly compile *to++ = buffer[i];
      }
  }
  printf("\n");
  Close(fH);  

  Screen(2);
  SetBorderColor(1);
  
  // Zero VRAM
  unsigned char VRAM_Kb = GetVramSize();
  FillVram(0, 0, VRAM_Kb*1024);
  
  // Dump 64 Kb of VRAM
  fH = open_file("vram.bin");
  for (int start_vram = 0; start_vram < 16*1024; start_vram += 1024) {
      Read(fH, buffer, 1024);      
      CopyRamToVram(buffer, start_vram, 1024);
  }
  Close(fH);
  
  //getchar();
  
  // Put page 3 of the game (segment 13) in our page 3
  // Put page 2 of the game (segment 12) in our page 2
  // Put page 1 of the game (segment 11) in our page 1
  // Page 0 not yet, since it's where we're executing now!

  __asm
  ld a, #13
  out (0xFF), a

  ld a, #12
  out (0xFE), a

  ld a, #11
  out (0xFD), a
 __endasm;

  // Patch the original code on its page 3.
  // Be careful not to go beyond 0XFFFE!
  ptr_origin = (unsigned char*)0xFFE0;
  ptr = ptr_origin;
  
  // See: https://clrhome.org/table/  
  *ptr++ = 0; // NOP
  
  *ptr++ = 0x32;
  *ptr++ = 0xE0;
  *ptr++ = 0xFF; // LD (FFE0), A

  *ptr++ = 0x3E;
  *ptr++ = 10; // LD A, 10
  
  *ptr++ = 0xD3;
  *ptr++ = 0xFC; // OUT (0xFC), A

  *ptr++ = 0x3A;
  *ptr++ = (char)(((unsigned int)ptr_origin & 0x00FF));
  *ptr++ = (char)(((unsigned int)ptr_origin & 0xFF00) >> 8);
  // LD A, ptr_origin

  *ptr++ = 0xFB; // EI

  *ptr++ = 0xC3;
  *ptr++ = (char)(((unsigned int)regs.pc & 0x00FF));
  *ptr++ = (char)(((unsigned int)regs.pc & 0xFF00) >> 8);
  // JP to the game's original PC

  __asm
  di

  ld sp, (_regs + 7*2)  // SP

  ld bc, (_ptr_origin) // our ret address to the second step in page 3
  push bc

  ld bc, (_regs + 0*2) // AF
  push bc
  ld bc, (_regs + 1*2) // BC
  push bc
  ld bc, (_regs + 2*2) // DE
  push bc
  ld bc, (_regs + 3*2) // HL
  push bc
  ld bc, (_regs + 4*2) // IX
  push bc
  ld bc, (_regs + 5*2) // IY
  push bc
  
  // Shadow registers
  exx
  ex af,af'
  //
  ld bc, (_regs + 8*2) // AF'
  push bc
  ld bc, (_regs + 9*2) // BC'
  push bc
  ld bc, (_regs + 10*2) // DE'
  push bc
  ld bc, (_regs + 11*2) // HL'
  push bc
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

  // Exit
  Screen(1);
  //Exit(0);
}
 
