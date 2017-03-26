
#include "TVoutSPI.h"

void displayTitles()
{
  clrrect(0,0,3,40,' ');
  printstr("         Hackvision Programmer");
  printstr("\n             Version 1.0");
  printstr("\n       Please select a game:\n");
  
}
void ui() {
  byte selectedIndex;
  byte newSelectedIndex;
  const byte numLinesForSelection = 20;
  const byte firstListLine = 5;
  TVsetup();
  //
  // Clear the video buffer and print out sample text
  //
  chrout(12);
  displayTitles();
  while (1) {
    if (vblank) // only update every frame
    {
      
      vblank=0;
    }

  }
}
