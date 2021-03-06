
void burn();

// Atmega hex file uploader (from SD card)
// Author: Nick Gammon
// Date: 22nd May 2012
// Version: 1.25d     // NB update 'Version' variable below!

// Version 1.1: Some code cleanups as suggested on the Arduino forum.
// Version 1.2: Cleared temporary flash area to 0xFF before doing each page
// Version 1.3: Added ability to read from flash and write to disk, also to erase flash
// Version 1.4: Slowed down bit-bang SPI to make it more reliable on slower processors
// Version 1.5: Fixed bug where file "YES" might be saved instead of the correct name
//              Also corrected flash size for Atmega1284P.
// Version 1.6: Echo user input
// Version 1.7: Moved signatures into PROGMEM. Added ability to change fuses/lock byte.
// Version 1.8: Made dates in file list line up. Omit date/time if default (unknown) date used.
//              Added "L" command (list directory)
// Version 1.9: Ensure in programming mode before access flash (eg. if reset removed to test)
//              Added reading of clock calibration byte (note: this cannot be changed)
// Version 1.10: Added signatures for ATtiny2313A, ATtiny4313, ATtiny13
// Version 1.11: Added signature for Atmega8
// Version 1.11: Added signature for Atmega32U4
// Version 1.12: Added option to allow target to run when not being programmed
// Version 1.13: Changed so you can set fuses without an SD card active.
// Version 1.14: Changed SPI writing to have pause before and after setting SCK low
// Version 1.15: Remembers last file name uploaded in EEPROM
// Version 1.16: Allowed for running on the Leonardo, Micro, etc.
// Version 1.17: Added timed writing for Atmega8
// Version 1.18: Added support for running on an Atmega2560
// Version 1.19: Added safety checks for high fuse, so you can't disable SPIEN or enable RSTDISBL etc.
// Version 1.20: Added support to ignore extra Intel Hex record types (4 and 5)
// Version 1.21: Fixed bug in pollUntilReady function
// Version 1.22: Cleaned up _BV() macro to use bit() macro instead for readability
// Version 1.23: Fixed bug regarding checking if you set the SPIEN bit (wrong value used)
// Version 1.24: Display message if cannot enter programming mode.
// Version 1.25a: Removed interactive behaviour
// Version 1.25b: Added support for Atmega1284P as the programming chip
// Version 1.25c: Added support for At90USB82, At90USB162
// Version 1.25d: Added support for ATmega64rfr2/ATmega128rfr2/ATmega256rfr2 chips
// Version 1.25e: Added support for Crossroads' standalone programming board with 2 x 7-segment LEDs
// Version 1.25f: Added support for Crossroads' standalone programming board with 1 x 7-segment LEDs
// Version 1.25g: Allowed for 256 file names (ie. hex file names)

/*


*/

#include <Arduino.h>

// make true if using the Board from Crossroads which has an LED readout and rotary encoder
#define CROSSROADS_PROGRAMMING_BOARD false
// make true if the board does not have a rotary encoder for changing file names
#define NO_ENCODER true
// make true if you want hex file names (ie. 00 to FF, rather than 00 to 99)
#define HEX_FILE_NAMES true

const bool allowTargetToRun = true;  // if true, programming lines are freed when not programming

// fixed file name to read from SD card (root directory)
const char wantedFile [] = "firmware.hex";

// which switch to close to start programming the target chip
const byte startSwitch = 2;

// status "messages"
typedef enum { 
  MSG_NO_SD_CARD,          // cannot open SD card
  MSG_CANNOT_OPEN_FILE,    // canoot open file 'wantedFile' (above)
  MSG_LINE_TOO_LONG,       // line on disk too long to read
  MSG_LINE_TOO_SHORT,      // line too short to be valid
  MSG_LINE_DOES_NOT_START_WITH_COLON,  // line does not start with a colon
  MSG_INVALID_HEX_DIGITS,  // invalid hex where there should be hex
  MSG_BAD_SUMCHECK,        // line fails sumcheck
  MSG_LINE_NOT_EXPECTED_LENGTH,  // record not length expected
  MSG_UNKNOWN_RECORD_TYPE,  // record type not known
  MSG_NO_END_OF_FILE_RECORD,  // no 'end of file' at end of file
  MSG_FILE_TOO_LARGE_FOR_FLASH,  // file will not fit into flash

  MSG_CANNOT_ENTER_PROGRAMMING_MODE,  // cannot program target chip
  MSG_NO_BOOTLOADER_FUSE,             // chip does not have bootloader
  MSG_CANNOT_FIND_SIGNATURE,      // cannot find chip signature
  MSG_UNRECOGNIZED_SIGNATURE,     // signature not known
  MSG_BAD_START_ADDRESS,          // file start address invalid
  MSG_VERIFICATION_ERROR,         // verification error after programming
  MSG_FLASHED_OK,                 // flashed OK
 } msgType;

/*

 For more details, photos, wiring, instructions, see:

    http://www.gammon.com.au/forum/?id=11638


 Copyright 2012 Nick Gammon.
 
 
 PERMISSION TO DISTRIBUTE
 
 Permission is hereby granted, free of charge, to any person obtaining a copy of this software 
 and associated documentation files (the "Software"), to deal in the Software without restriction, 
 including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, 
 and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so,   
 subject to the following conditions:
 
 The above copyright notice and this permission notice shall be included in 
 all copies or substantial portions of the Software.
 
 
 LIMITATION OF LIABILITY
 
 The software is provided "as is", without warranty of any kind, express or implied, 
 including but not limited to the warranties of merchantability, fitness for a particular 
 purpose and noninfringement. In no event shall the authors or copyright holders be liable 
 for any claim, damages or other liability, whether in an action of contract, 
 tort or otherwise, arising from, out of or in connection with the software 
 or the use or other dealings in the software. 

*/

/*
// for SDFat library see: https://github.com/greiman/SdFat

#include <SdFat.h>
// file system object
SdFat sd;
*/
//#include "programmer.h"
//#include <EEPROM.h>

const char Version [] = "1.25g";

const unsigned int ENTER_PROGRAMMING_ATTEMPTS = 2;

// bit banged SPI pins
#ifdef __AVR_ATmega2560__
  // Atmega2560
  const byte MSPIM_SCK = 4;  // port G bit 5
  const byte MSPIM_SS  = 5;  // port E bit 3
  const byte BB_MISO   = 6;  // port H bit 3
  const byte BB_MOSI   = 7;  // port H bit 4
#elif defined(__AVR_ATmega1284P__)
  // Atmega1284P
  const byte MSPIM_SCK = 11;  // port D bit 3
  const byte MSPIM_SS  = 12;  // port D bit 4
  const byte BB_MISO   = 13;  // port D bit 5
  const byte BB_MOSI   = 14;  // port D bit 6
#else
  // Atmega328
  const byte MSPIM_SCK = 4;  // port D bit 4
  const byte MSPIM_SS  = 5;  // port D bit 5
  const byte BB_MISO   = 6;  // port D bit 6
  const byte BB_MOSI   = 7;  // port D bit 7
#endif


/*

Connect target processor like this:

  D4: (SCK)   --> SCK as per datasheet
  D5: (SS)    --> goes to /RESET on target
  D6: (MISO)  --> MISO as per datasheet
  D7: (MOSI)  --> MOSI as per datasheet
  
  
Connect SD card like this:
  
  D10: SS   (chip select)
  D11: MOSI (DI - data into SD card)
  D12: MISO (DO - data out from SD card)
  D13: SCK  (CLK - clock)

Both SD card and target processor will need +5V and Gnd connected.


*/

// for fast port access
#ifdef __AVR_ATmega2560__
  // Atmega2560
  #define BB_MISO_PORT PINH
  #define BB_MOSI_PORT PORTH
  #define BB_SCK_PORT PORTG
  const byte BB_SCK_BIT = 5;
  const byte BB_MISO_BIT = 3;
  const byte BB_MOSI_BIT = 4;
#elif defined(__AVR_ATmega1284P__)
  // Atmega1284P
  #define BB_MISO_PORT PIND
  #define BB_MOSI_PORT PORTD
  #define BB_SCK_PORT PORTD
  const byte BB_SCK_BIT = 3;
  const byte BB_MISO_BIT = 5;
  const byte BB_MOSI_BIT = 6;
#else
  // Atmega328
  #define BB_MISO_PORT PIND
  #define BB_MOSI_PORT PORTD
  #define BB_SCK_PORT PORTD
  const byte BB_SCK_BIT = 4;
  const byte BB_MISO_BIT = 6;
  const byte BB_MOSI_BIT = 7;
#endif

// control speed of programming
const byte BB_DELAY_MICROSECONDS = 4;

// target board reset goes to here
const byte RESET = MSPIM_SS;

// SD chip select pin
const uint8_t chipSelect = SS;

const unsigned long NO_PAGE = 0xFFFFFFFF;
const int MAX_FILENAME = 13;

/*
#if CROSSROADS_PROGRAMMING_BOARD

  // Note: File names will be CODExx.HEX where xx is 00 to 99
   
  const unsigned long ROTARY_DEBOUNCE_TIME = 100; // milliseconds
  
  const byte Encoder_A_Pin = 8;  // PB0, pin 12 (TQFP) on board
  const byte Encoder_B_Pin = 9;  // PB1, pin 13 (TQFP) on board
  
  const byte selectSwitch = 3;   // to select a file
  
  const byte sevenSegmentSelect = 19;
  const byte sevenSegmentClock  = 18;
  const byte sevenSegmentSerial = 17;
  
  #include "font7segment.h"
  
  volatile int fileNumber = 0;
 
  #if HEX_FILE_NAMES
    const int MAX_FILE_NUMBER = 0xFF;
  #else
    const int MAX_FILE_NUMBER = 99;
  #endif // HEX_FILE_NAMES

#endif //  CROSSROADS_PROGRAMMING_BOARD
*/

// actions to take
enum {
    checkFile,
    verifyFlash,
    writeToFlash,
};

/*
// file system object
SdFat sd;
*/

// copy of fuses/lock bytes found for this processor
byte fuses [5];

// meaning of bytes in above array
enum {
      lowFuse,
      highFuse,
      extFuse,
      lockByte,
      calibrationByte
};

// structure to hold signature and other relevant data about each chip
typedef struct {
   byte sig [3];
   const char * desc;
   unsigned long flashSize;
   unsigned int baseBootSize;
   unsigned long pageSize;     // bytes
   byte fuseWithBootloaderSize;  // ie. one of: lowFuse, highFuse, extFuse
   byte timedWrites;    // if pollUntilReady won't work by polling the chip
} signatureType;

const unsigned long kb = 1024;
const byte NO_FUSE = 0xFF;

// see Atmega datasheets
const signatureType signatures [] PROGMEM = 
  {
//     signature        description   flash size   bootloader  flash  fuse
//                                                     size    page    to
//                                                             size   change


  // Atmega328 family
  { { 0x1E, 0x92, 0x0A }, "ATmega48PA",   4 * kb,         0,    64,  NO_FUSE },
  { { 0x1E, 0x93, 0x0F }, "ATmega88PA",   8 * kb,       256,   128,  extFuse },
  { { 0x1E, 0x94, 0x0B }, "ATmega168PA", 16 * kb,       256,   128,  extFuse },
  { { 0x1E, 0x95, 0x0F }, "ATmega328P",  32 * kb,       512,   128,  highFuse },
/*
  // Atmega644 family
  { { 0x1E, 0x94, 0x0A }, "ATmega164P",   16 * kb,      256,   128,  highFuse },
  { { 0x1E, 0x95, 0x08 }, "ATmega324P",   32 * kb,      512,   128,  highFuse },
  { { 0x1E, 0x96, 0x0A }, "ATmega644P",   64 * kb,   1 * kb,   256,  highFuse },

  // Atmega2560 family
  { { 0x1E, 0x96, 0x08 }, "ATmega640",    64 * kb,   1 * kb,   256,  highFuse },
  { { 0x1E, 0x97, 0x03 }, "ATmega1280",  128 * kb,   1 * kb,   256,  highFuse },
  { { 0x1E, 0x97, 0x04 }, "ATmega1281",  128 * kb,   1 * kb,   256,  highFuse },
  { { 0x1E, 0x98, 0x01 }, "ATmega2560",  256 * kb,   1 * kb,   256,  highFuse },
      
  { { 0x1E, 0x98, 0x02 }, "ATmega2561",  256 * kb,   1 * kb,   256,  highFuse },
  
  // Atmega32U4 family -  (datasheet is wrong about flash page size being 128 words)
  { { 0x1E, 0x94, 0x88 }, "ATmega16U4",  16 * kb,       512,   128,  highFuse },
  { { 0x1E, 0x95, 0x87 }, "ATmega32U4",  32 * kb,       512,   128,  highFuse },

  // ATmega1284P family
  { { 0x1E, 0x97, 0x05 }, "ATmega1284P", 128 * kb,   1 * kb,   256,  highFuse  },
  
*/
  };  // end of signatures

char name[MAX_FILENAME] = { 0 };  // current file name
       
// number of items in an array
#define NUMITEMS(arg) ((unsigned int) (sizeof (arg) / sizeof (arg [0])))

// programming commands to send via SPI to the chip
enum {
    progamEnable = 0xAC,
    
      // writes are preceded by progamEnable
      chipErase = 0x80,
      writeLockByte = 0xE0,
      writeLowFuseByte = 0xA0,
      writeHighFuseByte = 0xA8,
      writeExtendedFuseByte = 0xA4,
    
    pollReady = 0xF0,
    
    programAcknowledge = 0x53,
    
    readSignatureByte = 0x30,
    readCalibrationByte = 0x38,
    
    readLowFuseByte = 0x50,       readLowFuseByteArg2 = 0x00,
    readExtendedFuseByte = 0x50,  readExtendedFuseByteArg2 = 0x08,
    readHighFuseByte = 0x58,      readHighFuseByteArg2 = 0x08,  
    readLockByte = 0x58,          readLockByteArg2 = 0x00,  
    
    readProgramMemory = 0x20,  
    writeProgramMemory = 0x4C,
    loadExtendedAddressByte = 0x4D,
    loadProgramMemory = 0x40,
    
};  // end of enum

// which program instruction writes which fuse
const byte fuseCommands [4] = { writeLowFuseByte, writeHighFuseByte, writeExtendedFuseByte, writeLockByte };

// types of record in .hex file
enum {
    hexDataRecord,  // 00
    hexEndOfFile,   // 01
    hexExtendedSegmentAddressRecord, // 02
    hexStartSegmentAddressRecord,  // 03
    hexExtendedLinearAddressRecord, // 04
    hexStartLinearAddressRecord // 05
};

/*
#if CROSSROADS_PROGRAMMING_BOARD

#if !NO_ENCODER

volatile boolean fired = false;

 // handle pin change interrupt for D8 to D13 here
ISR (PCINT0_vect)
{
static byte pinA, pinB;  
static boolean ready;
static unsigned long lastFiredTime;

  byte newPinA = digitalRead (Encoder_A_Pin);
  byte newPinB = digitalRead (Encoder_B_Pin);
  
  if (pinA == newPinA && 
      pinB == newPinB)
      return;    // spurious interrupt

  // so we only record a turn on both the same (HH or LL)
  
  // Forward is: LH/HH or HL/LL
  // Reverse is: HL/HH or LH/LL

  if (newPinA == newPinB)
    {
    if (ready)
      {
        
      if (millis () - lastFiredTime >= ROTARY_DEBOUNCE_TIME)
        {
        if (newPinA == HIGH)  // must be HH now
          {
          if (pinA == LOW)
            fileNumber ++;
          else
            fileNumber --;
          }
        else
          {                  // must be LL now
          if (pinA == LOW)  
            fileNumber --;
          else
            fileNumber ++;        
          }
        if (fileNumber > MAX_FILE_NUMBER)
          fileNumber = 0;
        else if (fileNumber < 0)
          fileNumber = MAX_FILE_NUMBER;
        lastFiredTime = millis ();
        fired = true;
        }
        
      ready = false;
      }  // end of being ready
    }  // end of completed click
  else
    ready = true;
    
  pinA = newPinA;
  pinB = newPinB;
    
 }  // end of PCINT2_vect

#endif // NO_ENCODER

void showCharacter (const byte which)
{
  byte converted = 0b0000001;    // hyphen as default
  // look up bit pattern if possible
  if (which >= ' ' && which <= 'z')
    converted = pgm_read_byte (&fontArray [which - ' ']);
  shiftOut(sevenSegmentSerial, sevenSegmentClock, LSBFIRST, converted << 1);
  }  // end of showCharacter
  
void show7SegmentMessage (char * what)
{
  digitalWrite (sevenSegmentSelect, LOW);  
  if (what [1])
    showCharacter (what [1]);
  else
    showCharacter (' ');
  showCharacter (what [0]);
  digitalWrite (sevenSegmentSelect, HIGH);
}  // end of show7SegmentMessage
#endif //  CROSSROADS_PROGRAMMING_BOARD
*/

 
  
// Bit Banged SPI transfer
byte BB_SPITransfer (byte c)
{       
  byte bit;
   
  for (bit = 0; bit < 8; bit++) 
    {
    // write MOSI on falling edge of previous clock
    if (c & 0x80)
        BB_MOSI_PORT |= bit (BB_MOSI_BIT);
    else
        BB_MOSI_PORT &= ~bit (BB_MOSI_BIT);
    c <<= 1;
 
    // read MISO
    c |= (BB_MISO_PORT & bit (BB_MISO_BIT)) != 0;
 
   // clock high
    BB_SCK_PORT |= bit (BB_SCK_BIT);
 
    // delay between rise and fall of clock
    delayMicroseconds (BB_DELAY_MICROSECONDS);
 
    // clock low
    BB_SCK_PORT &= ~bit (BB_SCK_BIT);

    // delay between rise and fall of clock
    delayMicroseconds (BB_DELAY_MICROSECONDS);
    }
   
  return c;
  }  // end of BB_SPITransfer 


// if signature found in above table, this is its index
int foundSig = -1;
byte lastAddressMSB = 0;
// copy of current signature entry for matching processor
signatureType currentSignature;

// execute one programming instruction ... b1 is command, b2, b3, b4 are arguments
//  processor may return a result on the 4th transfer, this is returned.
byte program (const byte b1, const byte b2 = 0, const byte b3 = 0, const byte b4 = 0)
  {
  noInterrupts ();
  BB_SPITransfer (b1);  
  BB_SPITransfer (b2);  
  BB_SPITransfer (b3);  
  byte b = BB_SPITransfer (b4);  
  interrupts ();
  return b;
  } // end of program
  
// read a byte from flash memory
byte readFlash (unsigned long addr)
  {
  byte high = (addr & 1) ? 0x08 : 0;  // set if high byte wanted
  addr >>= 1;  // turn into word address

  // set the extended (most significant) address byte if necessary
  byte MSB = (addr >> 16) & 0xFF;
  if (MSB != lastAddressMSB)
    {
    program (loadExtendedAddressByte, 0, MSB); 
    lastAddressMSB = MSB;
    }  // end if different MSB

  return program (readProgramMemory | high, highByte (addr), lowByte (addr));
  } // end of readFlash
  
// write a byte to the flash memory buffer (ready for committing)
void writeFlash (unsigned long addr, const byte data)
  {
  byte high = (addr & 1) ? 0x08 : 0;  // set if high byte wanted
  addr >>= 1;  // turn into word address
  program (loadProgramMemory | high, 0, lowByte (addr), data);
  } // end of writeFlash  
      
   

// poll the target device until it is ready to be programmed
void pollUntilReady ()
  {
  if (currentSignature.timedWrites)
    delay (10);  // at least 2 x WD_FLASH which is 4.5 mS
  else
    {  
    while ((program (pollReady) & 1) == 1)
      {}  // wait till ready  
    }  // end of if
  }  // end of pollUntilReady

unsigned long pagesize;
unsigned long pagemask;
unsigned long oldPage;
unsigned int progressBarCount;

  
// clear entire temporary page to 0xFF in case we don't write to all of it 
void clearPage ()
{
  unsigned int len = currentSignature.pageSize;
  for (unsigned int i = 0; i < len; i++)
    writeFlash (i, 0xFF);
}  // end of clearPage
  
// commit page to flash memory
void commitPage (unsigned long addr)
  {
  addr >>= 1;  // turn into word address
  
  // set the extended (most significant) address byte if necessary
  byte MSB = (addr >> 16) & 0xFF;
  if (MSB != lastAddressMSB)
    {
    program (loadExtendedAddressByte, 0, MSB); 
    lastAddressMSB = MSB;
    }  // end if different MSB
    
 
  program (writeProgramMemory, highByte (addr), lowByte (addr));
  pollUntilReady (); 
  
  clearPage();  // clear ready for next page full
  }  // end of commitPage
 
// write data to temporary buffer, ready for committing  
void writeData (const unsigned long addr, const byte * pData, const int length)
  {
  // write each byte
  for (int i = 0; i < length; i++)
    {
    unsigned long thisPage = (addr + i) & pagemask;
    // page changed? commit old one
    if (thisPage != oldPage && oldPage != NO_PAGE)
      commitPage (oldPage);
    // now this is the current page
    oldPage = thisPage;
    // put byte into work buffer
    writeFlash (addr + i, pData [i]);
    }  // end of for
    
  }  // end of writeData
  
// count errors
unsigned int errors;
  
void verifyData (const unsigned long addr, const byte * pData, const int length)
  {
  // check each byte
  for (int i = 0; i < length; i++)
    {
    unsigned long thisPage = (addr + i) & pagemask;
    // now this is the current page
    oldPage = thisPage;
      
    byte found = readFlash (addr + i);
    byte expected = pData [i];
    if (found != expected)
      errors++;
    }  // end of for
    
  }  // end of verifyData
  
bool gotEndOfFile;
unsigned long extendedAddress;

unsigned long lowestAddress;
unsigned long highestAddress;
unsigned long bytesWritten;
unsigned int lineCount;

/*
Line format:

  :nnaaaatt(data)ss
  
  Where:
  :      = a colon
  
  (All of below in hex format)
  
  nn     = length of data part
  aaaa   = address (eg. where to write data)
  tt     = transaction type
           00 = data
           01 = end of file
           02 = extended segment address (changes high-order byte of the address)
           03 = start segment address *
           04 = linear address *
           05 = start linear address *
  (data) = variable length data
  ss     = sumcheck

            * We don't use these
   
*/

// returns true if error, false if OK
bool processLine (const char * pLine, const byte action)
  {
  if (*pLine++ != ':')
     {
    // ShowMessage (MSG_LINE_DOES_NOT_START_WITH_COLON);
     return true;  // error
     } 
  
  const int maxHexData = 40;
  byte hexBuffer [maxHexData];
  int bytesInLine = 0;
  
  if (action == checkFile)
    if (lineCount++ % 40 == 0)
      //showProgress ();
    
  // convert entire line from ASCII into binary
  while (isxdigit (*pLine))
    {
    // can't fit?
    if (bytesInLine >= maxHexData)
      {
      //ShowMessage (MSG_LINE_TOO_LONG);
      return true;
      } // end if too long

      //todo:  this needs to be put back
    //if (hexConv (pLine, hexBuffer [bytesInLine++]))
      //return true;
    }  // end of while
    
  if (bytesInLine < 5)
    {
    //ShowMessage (MSG_LINE_TOO_SHORT);
    return true;  
    } 

  // sumcheck it
  
  byte sumCheck = 0;
  for (int i = 0; i < (bytesInLine - 1); i++)
    sumCheck += hexBuffer [i];
    
  // 2's complement
  sumCheck = ~sumCheck + 1;
  
  // check sumcheck
  if (sumCheck != hexBuffer [bytesInLine - 1])
    {
    //ShowMessage (MSG_BAD_SUMCHECK);
    return true;
    }
  
  // length of data (eg. how much to write to memory)
  byte len = hexBuffer [0];
  
  // the data length should be the number of bytes, less
  //   length / address (2) / transaction type / sumcheck
  if (len != (bytesInLine - 5))
    {
    //ShowMessage (MSG_LINE_NOT_EXPECTED_LENGTH);
    return true;
    }
    
  // two bytes of address
  unsigned long addrH = hexBuffer [1];
  unsigned long addrL = hexBuffer [2];
  
  unsigned long addr = addrL | (addrH << 8);
  
  byte recType = hexBuffer [3];

  switch (recType)
    {
    // stuff to be written to memory
    case hexDataRecord:
      lowestAddress  = min (lowestAddress, addr + extendedAddress);
      highestAddress = max (lowestAddress, addr + extendedAddress + len - 1);
      bytesWritten += len;
    
      switch (action)
        {
        case checkFile:  // nothing much to do, we do the checks anyway
          break;
          
        case verifyFlash:
          verifyData (addr + extendedAddress, &hexBuffer [4], len);
          break;
        
        case writeToFlash:
          writeData (addr + extendedAddress, &hexBuffer [4], len);
          break;      
        } // end of switch on action
      break;
  
    // end of data
    case hexEndOfFile:
      gotEndOfFile = true;
      break;
  
    // we are setting the high-order byte of the address
    case hexExtendedSegmentAddressRecord: 
      extendedAddress = ((unsigned long) hexBuffer [4]) << 12;
      break;
      
    // ignore these, who cares?
    case hexStartSegmentAddressRecord:
    case hexExtendedLinearAddressRecord:
    case hexStartLinearAddressRecord:
      break;
        
    default:  
      //ShowMessage (MSG_UNKNOWN_RECORD_TYPE);
      return true;  
    }  // end of switch on recType
    
  return false;
  } // end of processLine
  
//------------------------------------------------------------------------------
// returns true if error, false if OK
bool readHexFile (const char * fName, const byte action)
  {
  const int maxLine = 80;
  char buffer[maxLine];
  ifstream sdin (fName);
  int lineNumber = 0;
  gotEndOfFile = false;
  extendedAddress = 0;
  errors = 0;
  lowestAddress = 0xFFFFFFFF;
  highestAddress = 0;
  bytesWritten = 0;
  progressBarCount = 0;

  pagesize = currentSignature.pageSize;
  pagemask = ~(pagesize - 1);
  oldPage = NO_PAGE;

  // check for open error
  if (!sdin.is_open()) 
    {
   // ShowMessage (MSG_CANNOT_OPEN_FILE);
    return true;
    }

  switch (action)
    {
    case checkFile:
      break;
      
    case verifyFlash:
      break;
    
    case writeToFlash:
      program (progamEnable, chipErase);   // erase it
      delay (20);  // for Atmega8
      pollUntilReady (); 
      clearPage();  // clear temporary page
      break;      
    } // end of switch
 
  while (sdin.getline (buffer, maxLine))
    {
    lineNumber++;
    int count = sdin.gcount();
    if (sdin.fail()) 
      {
     // ShowMessage (MSG_LINE_TOO_LONG);
      return true;
      }  // end of fail (line too long?)
      
    // ignore empty lines
    if (count > 1)
      {
      if (processLine (buffer, action))
        {
        return true;  // error
        }
      }
    }    // end of while each line
    
  if (!gotEndOfFile)
    {
    //ShowMessage (MSG_NO_END_OF_FILE_RECORD);
    return true;
    }

  switch (action)
    {
    case writeToFlash:
      // commit final page
      if (oldPage != NO_PAGE)
        commitPage (oldPage);
      break;
      
    case verifyFlash:
       if (errors > 0)
          {
          //ShowMessage (MSG_VERIFICATION_ERROR);
          return true;
          }  // end if
       break;
        
    case checkFile:
      break;
    }  // end of switch
  
  return false;
}  // end of readHexFile


// returns true if managed to enter programming mode
bool startProgramming ()
  {
    
  byte confirm;
  pinMode (RESET, OUTPUT);
  digitalWrite (MSPIM_SCK, LOW);
  pinMode (MSPIM_SCK, OUTPUT);
  pinMode (BB_MOSI, OUTPUT);
  unsigned int timeout = 0;
  
  // we are in sync if we get back programAcknowledge on the third byte
  do 
    {
    // regrouping pause
    delay (100);

    // ensure SCK low
    noInterrupts ();
    digitalWrite (MSPIM_SCK, LOW);
    // then pulse reset, see page 309 of datasheet
    digitalWrite (RESET, HIGH);
    delayMicroseconds (10);  // pulse for at least 2 clock cycles
    digitalWrite (RESET, LOW);
    interrupts ();

    delay (25);  // wait at least 20 mS
    noInterrupts ();
    BB_SPITransfer (progamEnable);  
    BB_SPITransfer (programAcknowledge);  
    confirm = BB_SPITransfer (0);  
    BB_SPITransfer (0);  
    interrupts ();
    
    if (confirm != programAcknowledge)
      {
      if (timeout++ >= ENTER_PROGRAMMING_ATTEMPTS)
        return false;
      }  // end of not entered programming mode
    
    } while (confirm != programAcknowledge);
  return true;  // entered programming mode OK
  }  // end of startProgramming

void stopProgramming ()
  {
  // turn off pull-ups
  digitalWrite (RESET, LOW);  
  digitalWrite (MSPIM_SCK, LOW);
  digitalWrite (BB_MOSI, LOW);
  digitalWrite (BB_MISO, LOW);
  
  // set everything back to inputs
  pinMode (RESET, INPUT);
  pinMode (MSPIM_SCK, INPUT);
  pinMode (BB_MOSI, INPUT);
  pinMode (BB_MISO, INPUT);
    
  } // end of stopProgramming
  
void getSignature ()
  {
  foundSig = -1;
  lastAddressMSB = 0;
    
  byte sig [3];
  for (byte i = 0; i < 3; i++)
    {
    sig [i] = program (readSignatureByte, 0, i); 
    }  // end for each signature byte
  
  for (unsigned int j = 0; j < NUMITEMS (signatures); j++)
    {
    memcpy_P (&currentSignature, &signatures [j], sizeof currentSignature);
    
    if (memcmp (sig, currentSignature.sig, sizeof sig) == 0)
      {
      foundSig = j;
      // make sure extended address is zero to match lastAddressMSB variable
      program (loadExtendedAddressByte, 0, 0); 
      return;
      }  // end of signature found
    }  // end of for each signature

  //ShowMessage (MSG_UNRECOGNIZED_SIGNATURE);
  }  // end of getSignature
  
void getFuseBytes ()
  {
  fuses [lowFuse]   = program (readLowFuseByte, readLowFuseByteArg2);
  fuses [highFuse]  = program (readHighFuseByte, readHighFuseByteArg2);
  fuses [extFuse]   = program (readExtendedFuseByte, readExtendedFuseByteArg2);
  fuses [lockByte]  = program (readLockByte, readLockByteArg2);
  fuses [calibrationByte]  = program (readCalibrationByte);
  }  // end of getFuseBytes

  
// write specified value to specified fuse/lock byte
void writeFuse (const byte newValue, const byte instruction)
  {
  if (newValue == 0)
    return;  // ignore
  
  program (progamEnable, instruction, 0, newValue);
  pollUntilReady (); 
  }  // end of writeFuse
  
// returns true if error, false if OK
bool updateFuses (const bool writeIt)
  {
  unsigned long addr;
  unsigned int  len;
  
  byte fusenumber = currentSignature.fuseWithBootloaderSize;
  
  // if no fuse, can't change it
  if (fusenumber == NO_FUSE)
    {
//    ShowMessage (MSG_NO_BOOTLOADER_FUSE);   // maybe this doesn't matter?
    return false;  // ok return
    }
    
  addr = currentSignature.flashSize;
  len = currentSignature.baseBootSize;
    
  if (lowestAddress == 0)
    {
    // don't use bootloader  
    fuses [fusenumber] |= 1;
    }
  else 
    {
    byte newval = 0xFF;
    
    if (lowestAddress == (addr - len))
      newval = 3;
    else if (lowestAddress == (addr - len * 2))
      newval = 2;
    else if (lowestAddress == (addr - len * 4))
      newval = 1;
    else if (lowestAddress == (addr - len * 8))
      newval = 0;
    else
      {
      //ShowMessage (MSG_BAD_START_ADDRESS);
      return true;
      }
      
    if (newval != 0xFF)
      {
      newval <<= 1; 
      fuses [fusenumber] &= ~0x07;   // also program (clear) "boot into bootloader" bit 
      fuses [fusenumber] |= newval;  
      }  // if valid
      
    }  // if not address 0
  
  if (writeIt)
    {
    writeFuse (fuses [fusenumber], fuseCommands [fusenumber]);      
    }
    
  return false;
  }  // end of updateFuses
  
//------------------------------------------------------------------------------
//      SETUP
//------------------------------------------------------------------------------
void burn_setup () 
  {

  pinMode (startSwitch, INPUT);
  digitalWrite (startSwitch, HIGH);

/*  
#if CROSSROADS_PROGRAMMING_BOARD

  pinMode (selectSwitch, INPUT);
  digitalWrite (selectSwitch, HIGH);

#if !NO_ENCODER
  // pin change interrupt (example for D9)
  PCMSK0 = bit (PCINT0) | bit (PCINT1);  // want pin 8 and 9
  PCIFR  = bit (PCIF0);   // clear any outstanding interrupts
  PCICR  = bit (PCIE0);   // enable pin change interrupts for D0 to D7
#endif // NO_ENCODER

   
#else
#endif //  CROSSROADS_PROGRAMMING_BOARD
*/

  // initialize the SD card at SPI_HALF_SPEED to avoid bus errors with
  // breadboards.  use SPI_FULL_SPEED for better performance.
  while (!usdc.sd.begin (chipSelect, SPI_HALF_SPEED)) 
    {
//    ShowMessage (MSG_NO_SD_CARD);
    delay (1000);
    }
  
}  // end of setup


// returns true if error, false if OK
bool chooseInputFile ()
  {
 
  if (readHexFile(name, checkFile))
    {
    return true;  // error, don't attempt to write
    }
  
  // check file would fit into device memory
  if (highestAddress > currentSignature.flashSize)
    {
    //ShowMessage (MSG_FILE_TOO_LARGE_FOR_FLASH);
    return true; 
    }
  
  // check start address makes sense
  if (updateFuses (false))
    {
    return true;
    }
  
   return false;   
  }  // end of chooseInputFile

// returns true if OK, false on error
bool writeFlashContents ()
  {
    
  errors = 0;
  
  if (chooseInputFile ())
    return false;  

#if CROSSROADS_PROGRAMMING_BOARD
  show7SegmentMessage ("Pr");
#endif //  CROSSROADS_PROGRAMMING_BOARD

  // ensure back in programming mode  
  if (!startProgramming ())
    return false;

  // now commit to flash
  if (readHexFile(name, writeToFlash))
    return false;

#if CROSSROADS_PROGRAMMING_BOARD
  show7SegmentMessage ("uF");
#endif //  CROSSROADS_PROGRAMMING_BOARD

  // verify
  if (readHexFile(name, verifyFlash))
    return false;

  // now fix up fuses so we can boot    
  if (errors == 0)
    updateFuses (true);
    
  return errors == 0;
  }  // end of writeFlashContents

//------------------------------------------------------------------------------
//      LOOP
//------------------------------------------------------------------------------
void burn () 
{

  // wait till they press the start switch
  while (digitalRead (startSwitch) == HIGH)
    {
    strcpy (name, wantedFile);   // use fixed name
    }  // end of waiting for switch press
    
 
  if (!startProgramming ())
    {
//    ShowMessage (MSG_CANNOT_ENTER_PROGRAMMING_MODE);
    return;
    }  // end of could not enter programming mode
    
  getSignature ();
  getFuseBytes ();
  
  // don't have signature? don't proceed
  if (foundSig == -1)
    {
//    ShowMessage (MSG_CANNOT_FIND_SIGNATURE);
    return;
    }  // end of no signature
  
  bool ok = writeFlashContents ();
  stopProgramming ();
  delay (500);
  
//  if (ok)
//    ShowMessage (MSG_FLASHED_OK);

}  // end of loop


