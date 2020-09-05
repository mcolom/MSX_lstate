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
 
