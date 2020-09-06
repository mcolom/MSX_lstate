# MSX_load_state
Load MSX1 games in MSX2 machines obtained from openMSX savestates.

This is simple MSX-DOS program that allows to read a openMSX savestate and restore the state.
Many MSX1 games are only available on cassette tapes or they loading routines might not be compatible with the MSX2 or the so-called MSX3 as OCM.

There exist programs that allow to directly read CAS files like LOADCAS, CASLOAD, and others, but most of the time I didn't succeed to load my games in my FPGA.
This is a work in progress (with many improvements ahead) and so far it seems to works with about 70% of the games I tried.

Instructions:
1) Save a state in your emulated MSX1 machine with openMSX
In its terminal type "savestate madmix" (for example) to save the state.
2) Run the Python script. For example: ./extract.py ~/.openMSX/savestates/madmix.oms
This script will create four binary files that the .com MSX-DOS program will read to recover the state
3) Run the MSX-DOS program.

Have fun!
Miguel
