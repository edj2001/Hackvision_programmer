
#include "TVoutSPI.h"

char selectedFileName[50] = "GAME.HEX";
void displayTitles()
{
  clrrect(0,0,3,40,' ');
  printstr("         Hackvision Programmer");
  printstr("\n             Version 1.0");

}

void displaySelection(){
  clrrect(24,0,24,40,' ');
  gotoPosition(24,0);
  printstr("  Press fire to burn ");
  printstr(selectedFileName);
}

void ui() {
  byte selectedIndex;
  byte newSelectedIndex;
  const byte numLinesForSelection = 20;
  const byte firstListLine = 5;
  TVsetup();
  //
  // Clear the video buffer and print out titles
  //
  chrout(12);
  displayTitles();
  printstr("\n       Please select a game:\n");
  
  while (1) {
    if (vblank) // only update every frame
    {
      
      vblank=0;
    }

  }
}

void ui_displayResults(){
    TVsetup();
  //
  // Clear the video buffer and print out titles
  //
  chrout(12);
  displayTitles();
 
}

