/*
 * Arduino Solar Controller
 *
 * by karldm, March 2016
 *
 * ascdata.cpp
 * 
 * Data management & memory accesses (EEPROM&FLASH)
 * 
 * TODO:
 * - check access in getParVal() and setParVal()
 * - modif _indextype[] to include type (byte-int-ulong) + format + units? byte[3]
 *   e.g. TEMP will define int & f4.2
 *        should define several quantities :
 *        basic: byte - int - ULONG - F4.2 - STRING10
 *        App customized: TEMP - HUMID - PRES - SWITCH - IPV4 
 * - suppress _lastLindexSearch
 * - compatibility with REST library to define
 
 2016/05/31
 - added bridge link to store data with: bridge.put bridge.get {key,value} in : getParVal / setParVal
 - values are stored in 'datastore' on Lininino part
 - two functions to sync datastore:  bridgeGetAll() bridgePutAll()

 2016/09/29
 - add the request data: BridgePutRequest(), BridgeGetRequest(), ifRequest()

2016/10/01
- store two values in EEPROM a) the last saved for re-power b) the default
 
 */

#include "ascdata.h"

//+++++++1+++++++++2+++++++++3+++++++++4+++++++++5+++++++++6+++++++++7+++++++++8
/*
 * Ascdata
 * declare Ascdata ascdata()
 *
 */
Ascdata::Ascdata() {
  _npar = 0;
  _lastIndexSearch = -1;  // last index found in data list
}

/*
 * par_F()
 * 
 * Parameter declaration
 * 
 * access = "rws" byte coded
 *
 *  r = read
 *  w = write
 *  s = saved in EEPROM
 *
 */
int Ascdata::par_F( byte * ppar, const __FlashStringHelper * label, const __FlashStringHelper * options ) {
  int err = 0;

  if ( _npar < NPARMAX )
  {
    // add the parameter    
    _P[_npar] = (byte *) ppar;
    
    _indextype[_npar] = TYPEBYTE; // encode _indextype info
    datalabels[_npar] = (char *)label;
    dataoptions[_npar] = (char *)options;

    _npar++;
    err = _npar;
  }
  else 
  {
    //Serial.print("> Failed to add ");
    //Serial.println(label);
    err = -1; // no more space available -- how to print it?
  }
  return(err);
}
 
int Ascdata::par_F( int * ppar, const __FlashStringHelper * label, const __FlashStringHelper * options ) {
  int err = 0;
  
  if ( _npar < NPARMAX )
  {
    // add the parameter
    _P[_npar] = (int *) ppar;
    
    _indextype[_npar] = TYPEINT; // encode _indextype info
    datalabels[_npar] = (char *)label;
    dataoptions[_npar] = (char *)options;

    _npar++;
    err = _npar;
  }
  else 
  {
    //Serial.print("> Failed to add ");
    //Serial.println(label);
    err = -1; // no more space available -- how to print it?
  }
  return(err);
}

int Ascdata::par_F( unsigned long * ppar, const __FlashStringHelper * label, const __FlashStringHelper * options ) {
  int err = 0;
  
  if ( _npar < NPARMAX )
  {
    // add the parameter    
    _P[_npar] = (unsigned long *) ppar;

    _indextype[_npar] = TYPEULONG; // encode _indextype info
    datalabels[_npar] = (char *)label;
    dataoptions[_npar] = (char *)options;

    _npar++;
    err = _npar;
  }
  else 
  {
    //Serial.print("> Failed to add ");
    //Serial.println(label);
    err = -1; // no more space available -- how to print it?
  }
  return(err);
}

/*
 *  GetNpar()
 *  
 *  Get the number of declared parameters
 *  Info in console about data usage
 */
int Ascdata::getNpar() {
  //
  PrintInfoDataUsage( _npar, NPARMAX );
  return(_npar);
}

/*
 * getParIndex() => searchPar() -- internal?
 * 
 * Get the index, return -1 if not found
 * & set _lastLindexSearch
 */
int Ascdata::getParIndex(const char * label) {
  int err = -1;
  int index = 0;

  // find the parameter index
  // look in the byte list
  while ( err == -1 && index < _npar ) 
  {
    if ( strcmp_P( label, datalabels[index]) ==0 ) 
    {
      err = index;
    }
   index++;
   _lastIndexSearch = err;
  }  
  return(err);
}

/*
 * checkParAccess( char access )
 * 
 * check par access of the current parameter
 * return true if ok
 */
boolean Ascdata::checkParAccess( char access ) {
  char options[BUFFERVALUE];
  int type;
  char * fmt;
  
  strcpy_P( options, dataoptions[_lastIndexSearch]); // copy into options
  fmt = strtok( options, " "); // locate the format string

  return( strchr( fmt, access) != NULL );
}
 
/*
 * getParVal() 
 * 
 * Get the value into a string with format adaptation
 *
 */
void Ascdata::getParVal( char * svalue ) {
  // use _lastLindexSearch & _lastIndexSearch;
  char options[BUFFERVALUE];
  char * fmt;
    
  strcpy_P( options, dataoptions[_lastIndexSearch]); // copy into options
  fmt = strchr( options, ' ')+1; // locate the format string
  
  switch ( _indextype[_lastIndexSearch] )
  {
    case TYPEBYTE :
      // copy the value with format transformation
      sprintf( svalue, "%d", * (byte *)_P[_lastIndexSearch] );
      break;
      
    case TYPEINT :
      // copy the value with format transformation
      if ( strcmp( fmt, "f4.2") == 0 ) {
        // see http://stackoverflow.com/questions/27651012/arduino-sprintf-float-not-formatting
        dtostrf( ((float)*(int *)_P[_lastIndexSearch] )/100, 4, 2, svalue);
        sprintf( svalue, "%s", svalue);
      }
      else {
        sprintf( svalue, "%i", * (int *)_P[_lastIndexSearch] );    
      }
      break;
      
    case TYPEULONG :
      // copy the value with format transformation
      sprintf( svalue, "%lu", * (unsigned long *)_P[_lastIndexSearch] );
      break;  
   }
}

int Ascdata::getParVal( char * svalue, const char * label ) {
  // a direct usage with label
  int index;
  // search
  index = this->getParIndex( label );
  if (index != -1) this->getParVal( svalue );

  return( index );
}

 /*
  * SetParVal()
  * 
  * Set the par value from a string with format adaptation
  * Should be called only of a valid parameter is found!
  * 
  * return 
  *   true if the value is modified (old != current)
  *   false otherwise
  */
boolean Ascdata::setParVal( const char * svalue ) {
  // use _lastLindexSearch
  char    options[BUFFERVALUE];
  char    strval[13] = ""; // a twelve digits string
  char    *fmt;
  char    *pvalue;
  int     ivalue = 0;
  boolean updated;
 
  strcpy_P( options, dataoptions[_lastIndexSearch]); // copy into options
  fmt = strchr(options, ' ')+1; // locate the format string

  switch ( _indextype[_lastIndexSearch] )
  {
    case TYPEBYTE :
      // copy the value with format transformation
      updated = (* (byte *)_P[_lastIndexSearch] != (byte) atoi(svalue));
      * (byte *)_P[_lastIndexSearch] = (byte) atoi(svalue);
      break;
      
    case TYPEINT :
      // copy the value with format transformation
      
      if ( strcmp( fmt, "f4.2") == 0 )
      {
        // we should copy the string in XXX.XX format
        //
        // sscanf doesn't work properly with %x on Arduino (x!=i)...
        //
        // add two '0' with append
        // locate the '.'
        // remove '.' and move two digits + '\0', 
        // that's it
        //
        strcpy( strval, svalue ); // strval is a working string
        strcat( strval, "00\0" );   // add two '0'
        pvalue = strchr(strval, '.'); // locate the .
        if ( pvalue != NULL) 
        {
          // move and get only two digits
          pvalue[0] = pvalue[1];
          pvalue[1] = pvalue[2];
          pvalue[2] = '\0';
        }
        updated = (* (int *)_P[_lastIndexSearch] != atoi(strval));
        * (int *)_P[_lastIndexSearch] = atoi(strval);
      }
      else {
        updated = (* (int *)_P[_lastIndexSearch] != atoi(svalue));
        * (int *)_P[_lastIndexSearch] = atoi(svalue);
      }
      break;
      
    case TYPEULONG :
      // copy the value with format transformation
      updated = (* (unsigned long *)_P[_lastIndexSearch] != atol(svalue));
      * (unsigned long *)_P[_lastIndexSearch] = atol(svalue);         
      break;  
  }
  return( updated );
}

int Ascdata::setParVal( const char * svalue, const char * label ) {
  // a direct usage with label
  int index;
  // search
  index = this->getParIndex( label );
  if (index != -1) this->setParVal( svalue );

  return( index );
}

/*
 * A simple mechanism to loop into the data
 * 
 * === usage ===========================================
 * index = mydata.loopIndex(-1) // first call with -1
 *
 * while (index != -1) {
 *  do_with_index (_lastLindexSearch is updated)
 *  index = mydata.loopIndex(index); // don't forget it!
 * }
 * =====================================================
 */
int Ascdata::loopIndex(int index) {
  int nxtindx;

  // First call with -1
  if ( index == -1 )
  {
    nxtindx = 0;
  } 
  else if ( index >= _npar-1 )
  {
    // last element done
    nxtindx = -1;
  }
  else
  {
    nxtindx = index+1;
  }
  _lastIndexSearch = nxtindx;

  return( nxtindx );
}
/*
 * Return a string with the label of _lastIndexSearch
 */
  char * Ascdata::loopLabel() {
    strcpy_P( labelbuf, datalabels[_lastIndexSearch] ); // Achtung
    return( labelbuf );
  }
  
/*
 * Return a string with the svalue of _lastIndexSearch
 */
  char * Ascdata::loopSvalue() {
    this->getParVal( labelbuf );
    return( labelbuf );
 }

/*
 * #######################################
 * Synchronization with datastore (bridge)
 * #######################################
 */
 
/*
 * bridgeGet()
 * 
 * access = '*' to retrieve all the data from datastore
 * else, only the data with the selected acces are copied
 * In a classical use, the access in 'g' (get)
 * 
 * return the number of saved updated parameters
 */
int  Ascdata::bridgeGet( char access ) {
  //
  int index;
  int updates;
  char bufval[BUFFERVALUE]; // a BUFFERVALUE-1 chars buffer
  char buflab[BUFFERLABEL]; // a BUFFERLABEL-1 chars buffer

  updates = 0;
  index = this->loopIndex(-1); // first call with -1
 
  while (index != -1) {
    // we only get the selected data or 'all' if access == '*'
    strcpy_P( buflab, datalabels[_lastIndexSearch] ); // Achtung

    if ( this->checkParAccess(access) || access == '*' ) {
      // get the data from datastore  
      Bridge.get( buflab, bufval, BUFFERVALUE-1 );
      if ( setParVal( bufval ) & checkParAccess('s') ) updates += 1; // check for 's' option
    }
    index = this->loopIndex(index); // don't forget it!
  }
  return( updates );
}

/*
 * bridgePut()
 * 
 * access = '*' to copy all the data into datastore
 * else, only the data with the selected acces are copied
 * In a classical use, the access in 'p' (put)
 */
int  Ascdata::bridgePut( char access ) {
  //
  int index;
  char bufval[BUFFERVALUE]; // a BUFFERVALUE-1 chars buffer

  index = this->loopIndex(-1); // first call with -1
 
  while (index != -1) {
    // we only put the selected data or 'all' if access == '*'
    if ( this->checkParAccess(access) || access == '*' ) {
      // put the data into datastore
      strcpy_P( labelbuf, datalabels[_lastIndexSearch] ); // Achtung
      getParVal( bufval );     
      Bridge.put( labelbuf, bufval );
    }
    index = this->loopIndex(index); // don't forget it!
  }
  return( 0 ); 
} 

/*
 * bridgePutVersion()
 * 
 * put the version info into datastore
 */
void Ascdata::bridgePutVersion( const char * sversion )
{
  Bridge.put( "version", sversion );
}

/*
 * bridgePutRequest()
 * 
 * put the request data into datastore
 */
void Ascdata::bridgePutRequest( const char * srequest )
{
  Bridge.put( "request", srequest );
}

/*
 * bridgeGetRequest()
 * 
 * get the current request and store it in _lastrequest
 * return true if != 'none' //i.e. if there is a new request
 */
boolean Ascdata::bridgeGetRequest()
{
  Bridge.get( "request", _lastrequest, REQUESTBUF_SIZE-1 ); // get the current request
  return( ( strcmp( "none", _lastrequest ) != 0 ) );
}

/*
 * isRequest()
 * 
 * true if srequest == _lastrequest
 */
boolean Ascdata::isRequest(const char * srequest)
{
  return( ( strcmp( srequest, _lastrequest ) == 0 ) );
}

/* 
 *  #################
 *  EEPROM Management
 *  #################
 *  
 *  loop in the par list and write if s or u are slected
 *  only the modified values will be writen 
 *  (put and get functions of EEPROM.h)
 *  
 *  Two values are stored for each 's' data
 *  a) the last saved data (value = 0)
 *  b) the default value (defined in the main sktech) (value = 1)
 */
int Ascdata::EEPROM_put( char* tag10, int value )
{ 
  int err = 0;
  int index;
  int eeaddress = 10*sizeof(byte); // begin at the first location -- 10 bytes for info

  // write the tag
  for ( int i = 0; i<10; i++ ) {
    EEPROM.put( i*sizeof(byte), tag10[i] );
  }
  
  index = this->loopIndex(-1);
  while( index != -1 && err == 0 )
  {
    // get the access of current parameter
    if ( this->checkParAccess('s') )
    {      
      switch ( _indextype[_lastIndexSearch] )
      {
        case TYPEBYTE :
          eeaddress = eeaddress + value*sizeof(byte);       // access to the default value if value == 1
          EEPROM.put(eeaddress, * (byte *)_P[_lastIndexSearch] );
          eeaddress = eeaddress + (2-value)*sizeof(byte);   // skip 2 if value == 0 
          break;
      
        case TYPEINT :
          eeaddress = eeaddress + value*sizeof(int);        // access to the default value if value == 1
          EEPROM.put(eeaddress, * (int *)_P[_lastIndexSearch] );
          eeaddress = eeaddress + (2-value)*sizeof(int);    // skip 2 if value == 0 
          break;
      
        case TYPEULONG :
          //EEPROM.put(eeaddress, * (unsigned long *)_P[_lastIndexSearch] );
          eeaddress = eeaddress + value*sizeof(unsigned long);        // access to the default value if value == 1
          EEPROMWritelong( eeaddress, * (unsigned long *)_P[_lastIndexSearch] );
          eeaddress = eeaddress + (2-value)*sizeof(unsigned long);    // skip 2 if value == 0 
         break; 
      }
      if (eeaddress >= EEPROM.length()) err = -1;
    }
    index = this->loopIndex( index );
  }
  return( err );
}
/*
 * Get the data from EEPROM in the same way than updateEEPROM()
 * share the same looping mechanism
 * 
 *  Two values are stored for each 's' data
 *  a) the last saved data (value = 0)
 *  b) the default value (defined in the main sktech) (value = 1)
 * 
 * WARNING: 
 *  on YUN, EEPROM is erased on sketch upload
 *  to modify this behaviours, we need to modify the bootloader
 *  see http://forum.arduino.cc/index.php?topic=204656.0 and
 *  http://www.engbedded.com/fusecalc/
 *  
 */
int Ascdata::EEPROM_get( char* tag10, int value ) 
{ 
  int err = 0;
  int index;
  int eeaddress = 10*sizeof(byte); // begin at the first location -- 10 bytes for info
  char cur;

  // read and check first 10 bytes
  for ( int i = 0; i<10; i++ ) {
    EEPROM.get( i*sizeof(byte), cur );
    if ( cur != tag10[i] ) err = -1;
  }
  
  index = this->loopIndex(-1);
  while( index != -1 && err == 0 )
  {
    // get the access of current parameter
    if ( this->checkParAccess('s') )
    { 
      switch ( _indextype[_lastIndexSearch] )
      {
        case TYPEBYTE :
          eeaddress = eeaddress + value*sizeof(byte);       // access to the default value if value == 1
          EEPROM.get(eeaddress, * (byte *)_P[_lastIndexSearch] );
          eeaddress = eeaddress + (2-value)*sizeof(byte);   // skip 2 if value == 0 
          break;
      
        case TYPEINT :
          eeaddress = eeaddress + value*sizeof(int);        // access to the default value if value == 1
          EEPROM.get(eeaddress, * (int *)_P[_lastIndexSearch] );
          eeaddress = eeaddress + (2-value)*sizeof(int);    // skip 2 if value == 0 
          break;
      
        case TYPEULONG :
          //EEPROM.get(eeaddress, * (unsigned long *)_P[_lastIndexSearch] );
          eeaddress = eeaddress + value*sizeof(unsigned long);        // access to the default value if value == 1
          * (unsigned long *)_P[_lastIndexSearch] = EEPROMReadlong( eeaddress );
          eeaddress = eeaddress + (2-value)*sizeof(unsigned long);    // skip 2 if value == 0 
          break; 
      }
      if (eeaddress >= EEPROM.length()) err = -1;
    }
    index = this->loopIndex( index );
  }
  return( err );
}

/*
 * Added utilities for EEPROM management
 * see : http://playground.arduino.cc/Code/EEPROMReadWriteLong
 *
 * Write a 4 byte (32bit) long to the EEPROM 
 */
void EEPROMWritelong(int address, long value)
      {
      //Decomposition from a long to 4 bytes by using bitshift.
      //One = Most significant -> Four = Least significant byte
      byte four = (value & 0xFF);
      byte three = ((value >> 8) & 0xFF);
      byte two = ((value >> 16) & 0xFF);
      byte one = ((value >> 24) & 0xFF);

      //Write the 4 bytes into the eeprom memory.
      EEPROM.write(address, four);
      EEPROM.write(address + 1, three);
      EEPROM.write(address + 2, two);
      EEPROM.write(address + 3, one);
      }
/*
 * Read a 4 byte (32bit) long from the EEPROM
 */

unsigned long EEPROMReadlong(int address)
      {
      //Read the 4 bytes from the eeprom memory.
      unsigned long four = EEPROM.read(address);
      unsigned long three = EEPROM.read(address + 1);
      unsigned long two = EEPROM.read(address + 2);
      unsigned long one = EEPROM.read(address + 3);

      //Return the recomposed long by using bitshift.
      return ((four << 0) & 0xFF) + ((three << 8) & 0xFFFF) + ((two << 16) & 0xFFFFFF) + ((one << 24) & 0xFFFFFFFF);
      }
 
