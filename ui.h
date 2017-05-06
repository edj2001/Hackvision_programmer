
#include "TVoutSPI.h"

char selectedFileName[50] = "GAME.HEX";

void displayTitles()
{
  clrrect(0,0,40,3,' ');
  gotoPosition(0,0);
  printstr_P(PSTR(("         Hackvision Programmer\n")));
  printstr_P(PSTR(("\n             Version 1.0\n")));
  if (enableBurning())
  {
    printstr_P(PSTR(("      Program Burning Enabled \n")));
  }
  else 
  {
    printstr_P(PSTR(("      Program Burning Disabled \n")));
  }

}

void displaySelection(){
  //clear the bottom two lines on the screen
  clrrect(0,23,40,2,' ');
  //go to the second last line near the bottom
  gotoPosition(0,23);
  printstr_P(PSTR(("  Press FIRE to burn \n   ")));
  printstr(selectedFileName);
}

void ui() {
  byte selectedIndex;
  byte newSelectedIndex;
  const byte numLinesForSelection = 15;
  const byte firstListLine = 7;
  //
  // Clear the video buffer and print out titles
  //
  clrscr();
  displayTitles();
  printstr("\n       Please select a game:\n");
  
  while (!fireButtonPressed()) {
 
    if (vblank) // only update every frame
    {

      if (true) {
        gotoPosition(0,firstListLine);
        printstr_P(PSTR("         Fire Button "));
        if (fireButtonDown()) printstr("down."); else printstr("up.  ");
        printstr_P(PSTR("\n    Scroll Up Button "));
        if (scrollUpButtonDown()) printstr("down."); else printstr("up.  ");
        printstr_P(PSTR("\n  Scroll Down Button "));
        if (scrollDownButtonDown()) printstr("down."); else printstr("up.  ");
        printstr_P(PSTR("\n"));
      }
      displaySelection();
      vblank=0;
    }

  }
  //copy selected file name to file to burn
  
}

void ui_displayResults(){
  //
  // Clear the video buffer and print out titles
  //
  clrscr();
  displayTitles();
  printstr("\n");
  printstr(strBurnMessage);
  TVdelay_frame(300);
  
}

void ui_splash() {
  //show the splash screen
  clrscr();
  displayTitles();
  printstr("\n Please wait for file search\n");
  TVdelay_frame(300);
}

