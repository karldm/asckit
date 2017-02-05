/*
 * data.h
 *
 * Data management and memory accesses (EEPROM&FLASH)
 *
 */

#ifndef ascdata_h
#define ascdata_h

#include <arduino.h>
#include <avr/pgmspace.h>
#include <EEPROM.h>
#include <Console.h>
#include <Bridge.h>

#include "ascutil.h"

// should be tuned to the system size to limit memory usage
// Instead of using malloc() - look at the info on serial screen
// we will use malloc()
//
#define NPARMAX       40

typedef int TEMP;              // temperatures are coded in 0.01 deg.C - format f4.2
typedef unsigned long ULONG;   // shorter declaration

#define TYPEUNDEF      0       // not used yet
#define TYPEBYTE       1
#define TYPEINT        2
#define TYPEULONG      3

#define BUF_LAB_SIZE    20		 // use string functions with FLASH memory datas
#define REQUESTBUF_SIZE 20     // buffer size for request handle _lastrequest

#define BUFFERLABEL     15     // buffer size for label char[]
#define BUFFERVALUE     20     // buffer size for value char[]

static char labelbuf[BUF_LAB_SIZE]; // ???

static PGM_P   datalabels[NPARMAX];   // labels list
static PGM_P   dataoptions[NPARMAX];  // define access and format for communication

// Ascdata
class Ascdata
{
  public:
  Ascdata();
  // pointers family to datas in memory
  int par_F(byte * ppar, const __FlashStringHelper * label, const __FlashStringHelper * options);
  int par_F(int * ppar, const __FlashStringHelper * label, const __FlashStringHelper * opions);
  int par_F(unsigned long * ppar, const __FlashStringHelper * label, const __FlashStringHelper * options);
  int getNpar();
  
  int  getParIndex(const char * label);                     // search a parameter and set _lastIndexSearch
  boolean checkParAccess( char access );                    // check if access allowed  
  void getParVal(char * svalue);                            // retrieve value (use _lastIndexSearch)
  int  getParVal(char * svalue, const char * label);        // retrieve value with label
  
  boolean setParVal(const char * svalue );                  // set par value (use _lastIndexSearch)
  int  setParVal(const char * svalue, const char * label ); // set par value with label
  
  int loopIndex(int index);                                 // looping mechanism into the par list (use _lastIndexSearch)
  char * loopLabel();                                       // current parameter label in loop
  char * loopSvalue();                                      // current parameter svalue in loop

  int  bridgeGet(char access);                              // retrieve the selected data from datastore (bridge)
  int  bridgePut(char access);                              // put the selected data into datastore
  void bridgePutVersion(const char * sversion);             // put the version info into datastore
  void bridgePutRequest(const char * srequest);             // put the request data into datastore
  boolean bridgeGetRequest();                               // get the current request and store it in _lastrequest
  boolean isRequest(const char * srequest);                 // true if equal to the _lastrequest
  
  int  EEPROM_put(char* tag10, int value);                  // write data into EEPROM
  int  EEPROM_get(char* tag10, int value);                  // read saved par values from EEPROM -- check tag10
 
  private:
  int _npar;                                                // total nb of parameters

  void * _P[NPARMAX];                                       // pointer list

  byte _indextype[NPARMAX];                                 // index in the type lists (byte - int - long - float)
  int  _lastIndexSearch;                                    // index found in data list (-1 if not found)

  char _lastrequest[REQUESTBUF_SIZE];                       // last request from datastore
};

void EEPROMWritelong(int address, long value);
unsigned long EEPROMReadlong(int address);

// Ascdata objet - global access for Webserver
extern Ascdata ascdata;

#endif
