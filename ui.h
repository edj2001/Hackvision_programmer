
#include "TVoutSPI.h"

char selectedFileName[50] = "GAME.HEX";
void displayTitles()
{
  clrrect(0,0,3,40,' ');
  printstr("         Hackvision Programmer");
  printstr("\n             Version 1.0");
  if (enableBurning())
  {
    printstr(" Program Burning Enabled \n");
  }
  else 
  {
    printstr(" Program Burning Disabled \n");
  }

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
  clrscr();
  displayTitles();
  printstr("\n       Please select a game:\n");
  
  while (!fireButtonPressed()) {
    if (vblank) // only update every frame
    {

      displaySelection();
      vblank=0;
    }

  }
  //copy selected file name to file to burn
  
}

void ui_displayResults(){
    TVsetup();
  //
  // Clear the video buffer and print out titles
  //
  clrscr();
  displayTitles();
  printstr("\n");
  printstr(strBurnMessage);
}

void ui_splash() {
  //show the splash screen
  TVsetup();
  clrscr();
  displayTitles();
  printstr("\n Please wait for file search\n");
  TVdelay_frame(300);
}

