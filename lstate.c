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

void main(void) 
{
  init();

  printf("Load MSX1 state\r\n\r\n");
  
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
  
  // Read registers    
  

  
  int fH = Open("regs.bin", O_RDONLY);

  //debug set_watchpoint read_io 0x2E
  //InPort(0x2E); // PC = 0x13B2, 0x02CF
  
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

  //getchar();

  Screen(2);
  SetBorderColor(1);

  // Open
  fH = Open("vram.bin", O_RDONLY);
  
  // Read to copy to VRAM
  for (int start_vram = 0; start_vram < 16*1024; start_vram += 1024) {
      Read(fH, buffer, 1024);      
      CopyRamToVram(buffer, start_vram, 1024);
  }
  
  // Close  
  Close(fH);
  
  //getchar();
  
  // Load page 0 (0000-3FFFh) of game in 0x8000, segment 10
  fH = Open("ram.bin", O_RDONLY);
  
  //InPort(0x2E); // DEBUG
  OutPort(0xFE, 10); // FE (write) Mapper segment for page 2 (#8000-#BFFF)
  Read(fH, (void*)0x8000, 16*1024);

  // Load page 1 (4000-7FFFh) of game in 0x8000, segment 11
  //InPort(0x2E); // DEBUG
  OutPort(0xFE, 11);
  Read(fH, (void*)0x8000, 16*1024);

  // Load page 2 (8000-BFFF) of game in 0x8000, segment 12
  //InPort(0x2E); // DEBUG
  OutPort(0xFE, 12);
  Read(fH, (void*)0x8000, 16*1024);
  
  // Load page 3 (C000-FFFF) of game in 0x8000, segment 13
  //InPort(0x2E); // DEBUG
  OutPort(0xFE, 13);
  Read(fH, (void*)0x8000, 16*1024);

  Close(fH);
  
  // Put page 3 of the game in our page 3, and so on.
  InPort(0x2E); // DEBUG

  __asm
  ld a, #13
  out (0xFF), a

  ld a, #12
  out (0xFE), a

  ld a, #11
  out (0xFD), a
 __endasm;

  // Patch the original code on its page 3
  ptr = (unsigned char*)0xFFE0;
  
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
  *ptr++ = 0xE0;
  *ptr++ = 0xFF; // LD A, (FFE0)

  *ptr++ = 0xC3;
  *ptr++ = 0xE3;
  *ptr++ = 0x5C; // JP 0x5CE3
  
  __asm
  ld sp, #0x0ff7
  
  ld bc, #0xFFE0 // return address
  push bc

  ld bc, #0x0044 // af
  push bc
  ld bc, #0x0107 // bc
  push bc
  ld bc, #0x94b3 // de
  push bc
  ld bc, #0x3058 // hl
  push bc
  ld bc, #0x8e92 // ix
  push bc
  ld bc, #0x85c1 // iy
  push bc
  
  // Shadow registers
  exx
  ex af,af'
  //
  ld bc, #0xb1b4 // af'
  push bc
  ld bc, #0x0000 // bc'
  push bc
  ld bc, #0x0f00 // de'
  push bc
  ld bc, #0x0000 // hl'
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
 
