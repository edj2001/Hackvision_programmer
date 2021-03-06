

//#include <Arduino.h>
//#include "programmer.h"
#include <avr/pgmspace.h>
#include <TVout.h>
TVout TV;

// for SDFat library see: https://github.com/greiman/SdFat
// modify #define USE_MULTIPLE_CARDS 1 in SDFatConfig.h to get rid of static cache definitions
#include <SdFat.h>

#define UseUnion
#ifdef UseUnion
  union uvar { 
    SdFat sd; 
    unsigned char TVBuffer[640];
    uvar::uvar() {
      }
    };
    
   uvar usdc;
#else
  struct uvar {
  // file system object
  SdFat sd;
  unsigned char TVBuffer[640];
  };
  uvar usdc;
#endif


#include "find_files.h"

#include "ui.h"

#include "burn.h"


boolean inProgrammingMode() {
  return digitalRead(1);
}

void setup(){
  
  }

void loop(){
  if (inProgrammingMode()) findFiles();
  if (inProgrammingMode()) ui();
  if (inProgrammingMode()) burn();
  
  }
