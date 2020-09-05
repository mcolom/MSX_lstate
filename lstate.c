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

// Warning: execution fails when the buffer is put inside main (stack overflow?)
char buffer[1024];

void main(void) 
{

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
  unsigned char *ptr = 0xFFFF;
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
  


  getchar();

  Screen(2);
  
  // It's mandatory to do this to use files!
  FCBlist *FCB = FCBs();
  
  // Open
  int fH = Open("vram.bin", O_RDONLY);
  
  // Read
  //int start_vram = 0;
  for (int start_vram = 0; start_vram < 16*1024; start_vram += 1024) {
      printf("start_vram=%d\r\n", start_vram);
      Read(fH, buffer, 1024);
      printf("Done read\r\n");
      
      printf("Copy to start_vram=%d\r\n", start_vram);
      CopyRamToVram(buffer, start_vram, 1024);
      printf("Done CopyRamToVram\r\n");
      
      printf("\r\n");
  }
  
  //ensamblador();
  printf("El valor es fH=%d\r\n", fH);
  
  //CopyRamToVram(void *SrcRamAddress, unsigned int DestVramAddress, unsigned int Length);
  
  
  printf("Cerrando...\r\n");
  Close(fH);
  printf("Fichero cerrado\r\n");
  
  getchar();
  Screen(1);
  
  //Exit(0);
}
 
