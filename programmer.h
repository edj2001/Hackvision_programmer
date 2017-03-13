
#ifndef HACKVISIONPROGRAMMER_H
#define HACKVISIONPROGRAMMER_H

// file system object from burn.cpp
#include <SdFat.h>
SdFat sd;

//TVout object from ui.h
#include <TVout.h>
TVout TV;

int zOff = 150;
int xOff = 0;
int yOff = 0;
int cSize = 50;

float cube3d[8][3] = {
  {xOff - cSize,yOff + cSize,zOff - cSize},
  {xOff + cSize,yOff + cSize,zOff - cSize},
  {xOff - cSize,yOff - cSize,zOff - cSize},
  {xOff + cSize,yOff - cSize,zOff - cSize},
  {xOff - cSize,yOff + cSize,zOff + cSize},
  {xOff + cSize,yOff + cSize,zOff + cSize},
  {xOff - cSize,yOff - cSize,zOff + cSize},
  {xOff + cSize,yOff - cSize,zOff + cSize}
};

#endif
