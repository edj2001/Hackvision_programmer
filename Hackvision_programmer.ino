

//#include <Arduino.h>
//#include "programmer.h"
#include <avr/pgmspace.h>
//#include <TVout.h>
//TVout TV;

// for SDFat library see: https://github.com/greiman/SdFat
// modify #define USE_MULTIPLE_CARDS 1 in SDFatConfig.h to get rid of static cache definitions
#include <SdFat.h>

#define enableBurningPin A2
#define programmingModePin A3
#define fireButton 2
#define scrollUpButton 3
#define scrollDownButton 4


#define UseUnion
#ifdef UseUnion
  union uvar { 
    SdFat sd; 
    unsigned char TVBuffer[25*40];
    uvar::uvar() {
      }
    };
    
   uvar usdc;
#else
  struct uvar {
  // file system object
  SdFat sd;
  unsigned char TVBuffer[25*40];
  };
  uvar usdc;
#endif


#include "find_files.h"

#include "ui.h"

#include "burn.h"


boolean inProgrammingMode() {
//  return !digitalRead(programmingModePin);
  return 1;  //temporary for debug
}

boolean enableBurning(void) {
    return !digitalRead(enableBurningPin);
}

boolean fireButtonDown(void) {
    return !digitalRead(fireButton);
}

boolean scrollDownButtonDown(void) {
    return !digitalRead(scrollDownButton);
}
boolean scrollUpButtonDown(void) {
    return !digitalRead(scrollUpButton);
}

boolean fireButtonPressed(void) {
    static boolean lastFireButtonPressed;
    boolean result;
    result = fireButtonDown()&&!lastFireButtonPressed;
    lastFireButtonPressed = fireButtonDown();
    return result;}


boolean scrollUpButtonPressed(void) {
    
    static boolean lastscrollUpButtonPressed;
    return digitalRead(enableBurningPin);
}
    

boolean scrollDownButtonPressed(void) {
    static boolean lastScrollDownButtonPressed;
    boolean result;
    result = scrollDownButtonDown()&&!lastScrollDownButtonPressed;
    lastScrollDownButtonPressed = scrollDownButtonDown();
    return result;
}
    
void setup(){
  // configure inputs
  pinMode(enableBurningPin, INPUT_PULLUP);
  pinMode(programmingModePin, INPUT_PULLUP);
  pinMode(fireButton, INPUT_PULLUP);
  pinMode(scrollUpButton, INPUT_PULLUP);
  pinMode(scrollDownButton, INPUT_PULLUP);
  }

void loop(){
  if (inProgrammingMode()) findFiles();
  if (inProgrammingMode()) ui();
  while (1) {}
  if (inProgrammingMode()) burn();
  if (inProgrammingMode()) ui_displayResults();
  //if (!inProgrammingMode()) sleep();
  
  }
