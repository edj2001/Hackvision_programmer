

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
//  return digitalRead(programmingModePin);
  return 1;  //temporary for debug
}

boolean enableBurning(void) {
    return digitalRead(enableBurningPin);
}

boolean fireButtonDown(void) {
    static boolean lastFireButtonPressed;
    return digitalRead(enableBurningPin);
}


boolean fireButtonPressed(void) {
    static boolean lastFireButtonPressed;
    return digitalRead(enableBurningPin);
}


boolean scrollUpButtonPressed(void) {
    
    static boolean lastscrollUpButtonPressed;
    return digitalRead(enableBurningPin);
}
    

boolean scrollDownButtonPressed(void) {
    static boolean lastScrollDownButtonPressed;
    return digitalRead(enableBurningPin);
}
    
void setup(){
  // configure inputs
  }

void loop(){
  if (inProgrammingMode()) findFiles();
  if (inProgrammingMode()) ui();
  while (1) {}
  if (inProgrammingMode()) burn();
  
  }
