/*
   Arduino Solar Controller - ASC-Kit v1.3

   by karldm, Dec 2016


   The MIT License (MIT)

   Copyright (c) 2016 www.renergia.fr

   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software and associated documentation files (the "Software"), to deal
   in the Software without restriction, including without limitation the rights
   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
   copies of the Software, and to permit persons to whom the Software is
   furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included in all
   copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   SOFTWARE.
*/

//-------1---------2---------3---------4---------5---------6---------7---------8
#define VERSION "asc.1.0a"        // Software version 
#define TAG10   "ASCKit...."      // Tag for EEPROM data structure check -- unused

//
// SOLAR HEATER STATES
// modes  : OFF - HEAT
// states : OFF - ON
//
// MAIN HEATER
// modes  : OFF - HEAT - ECO - FP
// states : ON - OFF

#define OFF     0                  // OFF mode or state
#define ON      1                  // ON state
#define HEAT    1                  // main heater & solar heater HEAT mode
#define ECO     2                  // Main heater ECO mode
#define FP      3                  // Main heater freeze protection

//
// CONTROLLER
// states : STOP - RUN - INIT - FAIL1 - FAIL2
// 
#define STOP      0                // Controller STOP state or mode
#define RUN       1                // Controller RUN state or mode
#define INIT      2                // Controller INIT state -- power onP
#define FAIL1     3                // Controller FAIL1 -- solar heater OFF due to TCOL measurement error
#define FAIL2     4                // Controller FAIL2 state -- sh and mh OFF due to TAMB measurements error

// Hardware configuration
#define PINDHT    2                // DHT sensor bus
#define PINLED1   3                // White LED status pin number
#define PINOW1    4                // OW1 Bus pin number (single sensor TCOL)
#define PINOW2    5                // OW2 Bus pin number (single sensor TEXT)
#define PINOW3    6                // OW1 Bus pin number (single sensor TUSR1)
#define PINOW4    7                // OW2 Bus pin number (single sensor TUSR2)
#define PINSWMH   8                // SWMH pin number
#define PINSWSH   9                // SWSH pin number
#define PINSWUSR 12                // SWUSR pin number

// sensors
// masks for the ERRSENSOR parameter (use for error diagnostic)
#define MASKTAMB   0b00000001
#define MASKHAMB   0b00000010
#define MASKTCOL   0b00000100
#define MASKTEXT   0b00001000
#define MASKTUSR1  0b00010000
#define MASKTUSR2  0b00100000

#define TEMPERATURE_RESOLUTION 12  // nb bits for DS18B20 sensors

//=== DHT SENSOR MODEL ===
#define DHTTYPE DHT22              // DHT 22  == AM2302 == AM2321
#define NSMPMA  5                  // nb of samples for computation of the moving average

// includes
#include <avr/pgmspace.h>

#include <Console.h>
#include <Bridge.h>

#include <DHT.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#include "ascutil.h"
#include "ascdata.h"

//-------1---------2---------3---------4---------5---------6---------7---------8
// Ascdata objet - global access -- needed?
Ascdata ascdata;

/************************************************************/
/***    PARAMETERS TYPE DECLARATION AND DEFAULT VALUES    ***/
/************************************************************/

// available types are :
//   byte
//   int            => TEMP
//   unsigned long  => ULONG

/*
 * Sensors
 */
TEMP   TAMB;               // ambiant temperature (.01 degC)
int    HAMB;               // humidity (0.01%)
TEMP   TCOL;               // collector temperature (.01 degC)
TEMP   TEXT;               // external temperature (.01 degC)
TEMP   TUSR1;              // temperature user#1 (.01 degC)
TEMP   TUSR2;              // temperature user#2 (.01 degC)

/*
 * Calculated
 */
int    NLOOPS = 0;         // number of loops per sec
int    PSOLTH = 0;         // heating solar power (W)
byte   SWSH = OFF;         // Solar heater switch (FAN)
byte   SWMH = OFF;         // Main heater switch
byte   SWUSR = OFF;        // user switch
byte   STATESH = OFF;      // begin with the OFF state
byte   STATEMH = OFF;      // begin with OFF state
byte   STATECTRL = INIT;   // controller INIT state at power on
byte   ERRSENSOR = 0;      // error status on sensor -- begin with no error
byte   ERRCTRL = 0;        // internal error id# -- go to STOP mode if != 0
ULONG  INDSH = 0;          // Cumulative index of solar energy (Wh)
ULONG  TCSH = 0;           // Cumulated time of SH usage (h)
ULONG  TCMH = 0;           // Cumulated time of MH usage (h)

/*
 * Controller parameters
 */
TEMP   TSET = 2400;        // set point (.01 degC)
TEMP   DTECO = 500;        // delta T eco (.01 degC)
byte   MODESH = ON;        // mode solar heater
byte   MODEMH = ON;        // mode main heater
ULONG  TMHON = 120;        // min time main heater in ON state(s)
ULONG  TMHOFF = 120;       // min time main heater in OFF state(s)
ULONG  TSHON = 120;        // min time Fan in ON state (s)
ULONG  TSHOFF = 120;       // min time Fan in OFF state (s)
TEMP   DTSHON = 600;       // delta T Fan ON (.01 deg.C)
TEMP   DTSHOFF = 300;      // delta T Fan OFF (.01 deg.C)
TEMP   AUG1 = 200;         // Solar heater set point delta (.01 deg.C)
TEMP   AUG2 =   0;         // Main heater set point delta (.01 deg.C)
TEMP   TFP = 800;          // Freeze protection set point (.01 deg.C)
TEMP   HYST = 100;         // Hysteresis parameter (.01 deg.C)
TEMP   DTDHT = 0;          // Overheating of the ambiance sensor when mounted inside the DIN package -- 0 if external mounting
                           // 1000 for closed mounting
int    VFAN = 200;         // Fan flow (cub.m/h)
int    ASOLT = 4;          // Thermal collectors total area (sq.m) -- for INDSH calculation
byte   SYSTEM = 1;         // System type (1=MH only, 2=MH+SH)
byte   CONF1 = 1;          // Air circulation configuration (1=External, 2=Recirculated) -- for INDSH calculation

/************************************************************/
/********** END PARAMETERS DEFAULT VALUES *******************/
/************************************************************/

// Define the x.hour counters
Counterh counterhTCMH( TCMH );   // time usage main heater (h)
Counterh counterhTCSH( TCSH );   // time usage solar heater (h)
Counterh counterhINDSH( INDSH ); // estimated usefull solar energy (Wh)

// Various timers - see usage in util.cpp
// declare delays in millis
Timer  timerOW( 750 / (1 << (12 - TEMPERATURE_RESOLUTION)) ); // min delay for DS18B20 convertion
Timer  timerDHT( 3000 );          // DHT readout delay, may be reduced...
Timer  timerMH( 0 );              // main heater time-in-state, begin with OFF state
Timer  timerSH( 0 );              // solar heater time-in-state, begin with OFF state
Timer  timerEEPROM( ONEHOURMS );  // update period of EEPROM data
Timer  timerLED( 3456 );          // flash the LED1
Timer  timerBridge( 2000 );       // synchronize bridge data (i.e. commands responsivity...)

// DHT sensor bus
DHT dht(PINDHT, DHTTYPE);

// 1-Wire buses & sensors
OneWire ow1(PINOW1);    // for TCOL
OneWire ow2(PINOW2);    // for TAMB
OneWire ow3(PINOW3);    // for TUSR1
OneWire ow4(PINOW4);    // for TUSR2

DallasTemperature sensor1(&ow1);
DallasTemperature sensor2(&ow2);
DallasTemperature sensor3(&ow3);
DallasTemperature sensor4(&ow4);

/*
 * === setup() ===
 */
void setup() {
  // Set PinMode
  pinMode( PINLED1, OUTPUT );
  pinMode( PINDHT, OUTPUT );
  pinMode( PINOW1, OUTPUT );
  pinMode( PINOW2, OUTPUT );
  pinMode( PINSWMH, OUTPUT );
  pinMode( PINSWSH, OUTPUT );
  pinMode( PINSWUSR, OUTPUT );

  // setoutputs -- check STATECTRL = INIT
  // done azap -- set switches in OFF position
  STATECTRL = INIT; // force the INIT state
  StateEngine();    // calculate the heaters states
  SetOutputs();     // set the outputs

  // bridge
  Bridge.begin();   // start the script /usr/bin/run-bridge on the MPU side
  
  // infos to the console by PrintInfo()
  // only for debug purpose! It's waiting for the console for ever
  BeginInfo();      // start the Console if CONSOLE if defined in util.h -- dev
  //delay(10000);   // DEBUG -- add a delay to be able to see the messages in the CONSOLE
  PrintInfo('i', "Arduino Solar Controller - Kit, %s", VERSION);

  /************************************************************
     WARNING:
     THE DECLARATION ORDER DEFINES THE DATA STORAGE IN EEPROM
     ANY CHANGE WILL PREVENT TO RETRIEVE THE STORED DATAS
   ************************************************************/
  //
  // Parameters declaration
  // ( pointer, label, options)
  // options = "<access> <format>" (! only one space)
  //   where <access> = {pgs}
  //                  p: put the value to datastore
  //                  g: get the value from datastore
  //                  s: save the value into the EEPROM
  //         <format> = f4.2 (xxxx.xx) i -- ipv4 (x.x.x.x)
  //

  // usr : user parameters
  ascdata.par_F( &TSET,     F("tset"),     F("gs f4.2"));
  ascdata.par_F( &DTECO,    F("dteco"),    F("gs f4.2"));
  ascdata.par_F( &MODESH,   F("modesh"),   F("gs i"));
  ascdata.par_F( &MODEMH,   F("modemh"),   F("gs i"));

  // dev : device parameters
  ascdata.par_F( &TMHON,    F("tmhon"),    F("gs i"));
  ascdata.par_F( &TMHOFF,   F("tmhoff"),   F("gs i"));
  ascdata.par_F( &TSHON,    F("tshon"),    F("gs i"));
  ascdata.par_F( &TSHOFF,   F("tshoff"),   F("gs i"));
  ascdata.par_F( &DTSHON,   F("dtshon"),   F("gs f4.2"));
  ascdata.par_F( &DTSHOFF,  F("dtshoff"),  F("gs f4.2"));
  ascdata.par_F( &AUG1,     F("aug1"),     F("gs f4.2"));
  ascdata.par_F( &AUG2,     F("aug2"),     F("gs f4.2"));
  ascdata.par_F( &TFP,      F("tfp"),      F("gs f4.2"));
  ascdata.par_F( &HYST,     F("hyst"),     F("gs f4.2"));
  ascdata.par_F( &DTDHT,    F("dtdht"),    F("gs f4.2"));
  ascdata.par_F( &VFAN,     F("vfan"),     F("gs i"));
  ascdata.par_F( &ASOLT,    F("asolt"),    F("gs i"));
  ascdata.par_F( &SYSTEM,   F("system"),   F("gs i"));
  ascdata.par_F( &CONF1,    F("conf1"),    F("gs i"));

  // Calculated
  ascdata.par_F( &NLOOPS,   F("nloops"),   F("p i"));
  ascdata.par_F( &PSOLTH,   F("psolth"),   F("p i"));
  ascdata.par_F( &INDSH,    F("indsh"),    F("ps i"));     // update period tbd
  ascdata.par_F( &TCMH,     F("tcmh"),     F("ps i"));     // update period tbd
  ascdata.par_F( &TCSH,     F("tcsh"),     F("ps i"));     // update period tbd

  // sensors & switches
  ascdata.par_F( &TAMB,     F("tamb"),     F("p f4.2"));
  ascdata.par_F( &HAMB,     F("hamb"),     F("p f4.2"));
  ascdata.par_F( &TCOL,     F("tcol"),     F("p f4.2"));
  ascdata.par_F( &TEXT,     F("text"),     F("p f4.2"));
  ascdata.par_F( &TUSR1,    F("tusr1"),    F("p f4.2"));
  ascdata.par_F( &TUSR2,    F("tusr2"),    F("p f4.2"));
  ascdata.par_F( &SWSH,     F("swsh"),     F("p i"));
  ascdata.par_F( &SWMH,     F("swmh"),     F("p i"));
  ascdata.par_F( &SWUSR,    F("swusr"),    F("gs i"));     // user switch -- saved?

  // states
  ascdata.par_F( &STATEMH,  F("statemh"),  F("p i"));
  ascdata.par_F( &STATESH,  F("statesh"),  F("p i"));
  ascdata.par_F( &STATECTRL, F("statectrl"), F("p i"));

  // errors
  ascdata.par_F( &ERRSENSOR, F("errsensor"), F("p i"));
  ascdata.par_F( &ERRCTRL,   F("errctrl"),   F("p i"));

  /************************************************************/
  /***************  END OF DECLARATION   **********************/
  /************************************************************/
  ascdata.getNpar(); // info on data usage if console activated -- dev

  //
  // Get configuration from EEPROM (if available and consistent with the present version (TAG10)
  //
  PrintInfo( 'i', F("Get EEPROM saves data..."));
  
  if ( ascdata.EEPROM_get( TAG10, 0 ) != 0 ) { 
    // data structure not compatible
    // Note: EEPROM is reset after a schetch upload => we write the default values
    PrintInfo( 'w', F("EEPROM TAG error."));
    // then update the EEPROM with the defaults values defined in the sketch
    // counters values are reset
    PrintInfo( 'i', F("Update EEPROM data with the sketch default values..."));
    ascdata.EEPROM_put( TAG10, 0 );
    // write the default values into EEPROM as well
    ascdata.EEPROM_put( TAG10, 1 );
  } 
  else {
    // Data retrieved from the EEPROM are ok
    // Update counters values
    counterhTCMH.set( TCMH );
    counterhTCSH.set( TCSH );
    counterhINDSH.set( INDSH );  
  }

  //
  // 1-Wire Initialization
  // 
  // Start up the library and define the resolution
  // Define the Async mode
  // Start a conversion
  //
  OWsensorBegin( sensor1 );
  OWsensorBegin( sensor2 );
  OWsensorBegin( sensor3 );
  OWsensorBegin( sensor4 );
  
  // Keys generation in datastore
  //
  // put all the data to create the keys
  ascdata.bridgePut('*'); // create all the keys in datastore
  
  // add the 'version' data
  ascdata.bridgePutVersion( VERSION );
  
  // add the 'request' data
  ascdata.bridgePutRequest("none");
    
  // start the controller
  LedBlinkingN( PINLED1, 200, 4 ); // indicate that the INIT phase is finished
  STATECTRL = RUN; // go into RUN state after power on

  // setup() finished
  PrintInfo( 'i', F("Starting loop()..."));
}

/*
 * === loop() ===
 */
void loop() {
  //
  static int nloops = 0;
  static Timer timerLoops( 1000 );
  int index;

  ReadSensors();
  CalcParameters();
  StateEngine();
  SetOutputs();

  // loop performance calculation
  nloops++;
  if ( timerLoops.check() ) {
    NLOOPS = nloops; // nb loops per sec
    nloops = 0;
  }

  if ( timerBridge.check() ) {
    // Bridge synchronization
    // do it only at low rate (~500ms), but may be optimized
    //
    // Retrieve data from datastore (bridge)
    // We get only the 'g' access values
    if ( ascdata.bridgeGet('g') != 0 ) {
      ascdata.EEPROM_put( TAG10, 0 ); // update EEPROM data if some modifications in saved data
      PrintInfo( 'i', F("EEPROM update due to datastore change"));
    }

    // Update the data into datastore (bridge)
    // we only put the 'p' access values
    // you should not have both 'p' and 'g' access for the same data...
    ascdata.bridgePut('p');

    // request handle
    // see RequestHandle()
    if (ascdata.bridgeGetRequest()) {
      ascdata.bridgePutRequest("none"); // reset request into datastore
      RequestHandle();                  // execute the request
    }
  }

  if ( timerEEPROM.check() ) {
    ascdata.EEPROM_put( TAG10, 0 ); // periodic update of EEPROM saved data (counters)
    PrintInfo( 'i', F("timerEEPROM update."));
  }
}

/*
 * === ReadSensors() ===
 */
void ReadSensors() {
  //
  // Read the sensors
  // Implemntation of a moving average on the temperature for reliability purpose
  // not on the humidity...
  //
  static TEMP smp_tamb[NSMPMA];
  static int n_dht = 0, i_dht = 0, nfail_dht = 0;
  float dhtval;
  int i;

  /////////////////////
  // Read DHT sensor //
  // TAMB & HAMB     //
  /////////////////////
  if ( timerDHT.check() ) {

    //
    // read temperature & humidity
    // driven by the temperature readout ( which is the same as the humidity)
    //

    //
    // Temperature acquisition
    //
    dhtval = dht.readTemperature(); // default is Celsius
    
    // test on temperature
    if (isnan(dhtval)) {
      //
      // Fail to read the sensor -- test impact of sampling time from timerDHT()
      // Wait NSMPMA errors before raising an error on TAMB
      //
      if ( IsSensorValid( MASKTAMB ) ) PrintInfo( 'w', F("Failed to read temperature from DHT sensor...")); // only once
      
      // update the fail number
      nfail_dht++;
      
      // raise an error only if the number of error> NSMPMA
      if ( nfail_dht > NSMPMA ) ErrSensorRaise( MASKTAMB );
    }
    
    else {
      //
      // Temperature sample ok
      // compute the mean value
      //
      if ( !IsSensorValid( MASKTAMB ) ) PrintInfo( 'i', F("DHT sensor DHT ok...")); // only once
      
      // number of samples
      n_dht = min(NSMPMA, n_dht+1); // nb samples
      
      // temperature
      smp_tamb[i_dht] = int(100 * dhtval) - DTDHT; // correction for overheating
      TAMB = smp_tamb[i_dht];                      // default is the current value

      // compute the moving average
      if (n_dht == NSMPMA) {
        TAMB = 0;
        for (i=0; i<n_dht; i++) {
          TAMB = TAMB + int(smp_tamb[i]/n_dht);   // compute average
        }     
      }

      // current index update
      i_dht = (i_dht+1)%NSMPMA; // we get a new value...
      
      // fail counter
      nfail_dht = 0;
      ErrSensorClear( MASKTAMB );
    }

    //
    // Humidity acquisition
    //
    dhtval = dht.readHumidity();               // should be ok if the temperature is ok
    
    if (isnan(dhtval)) {
      //
      // Fail to read the sensor for humidity
      //
      if ( IsSensorValid( MASKHAMB ) ) PrintInfo( 'w', F("Failed to read humidity from DHT sensor...")); // only once
      
      // raise an error 
      ErrSensorRaise( MASKHAMB );
    }
    else {
      HAMB = int(100 * dhtval);
      HAMB += 100*int(DTDHT/(2*(100-int(HAMB/100))/3+6)); // 'simple' correction for overheating

      // no error detect  ed
      ErrSensorClear( MASKHAMB );
    }
  }

  ///////////////////////////
  // DS18B20 Sensors:      //
  // Read OW1 Bus => TCOL  //
  // Read OW2 Bus => TEXT  //
  // Read OW3 Bus => TUSR1 //
  // Read OW4 Bus => TUSR2 //
  ///////////////////////////
  if ( timerOW.check() ) {
    //
    // to do -- should add error messages
    //
    TCOL  = ReadOWbus( sensor1, MASKTCOL );
    TEXT  = ReadOWbus( sensor2, MASKTEXT );
    TUSR1 = ReadOWbus( sensor3, MASKTUSR1 );
    TUSR2 = ReadOWbus( sensor4, MASKTUSR2 );
  }
}

/*
 * === CalcParameters() ===
 */
void CalcParameters() {
  // Parameters calculation
  PSOLTH = 0; // default value
  if ( STATESH == ON ) {
    // PSOLTH (W) -- check positive value
    if ( CONF1 == 1 && TCOL > TEXT ) PSOLTH = int( 0.335 * VFAN * (TCOL - TEXT) / 100 ); // external air
    if ( CONF1 == 2 && TCOL > TAMB ) PSOLTH = int( 0.335 * VFAN * (TCOL - TAMB) / 100 ); // recirculated
  }

  // counters calls for cumulative quantities
  TCMH = int(counterhTCMH.add(1)); // time integration (h)
  TCSH = int(counterhTCSH.add(1)); // time integration (h)
  INDSH = int(counterhINDSH.add((unsigned long) PSOLTH)); // power integration (Wh)
}

/*
 * === StateEngine() ===
 */
void StateEngine() {
  //
  // Define the controller state STATECTRL ( INTI - STOP - RUN - FAIL1 - FAIL2 ) -- INIT defined during setup()
  // Define the heaters states: STATEMH & STATESH
  // See SetOutputs()
  //
  // TO BE CHECKED -- correction FAIL2???
  //
  if ( STATECTRL == INIT ) {
    STATEMH = OFF;
    STATESH = OFF;
  }
  else if ( STATECTRL == STOP ) {
    STATEMH = OFF;
    STATESH = OFF;    
  }
  else { // i.e. RUN, FAIL1 or FAIL2
    if ( IsSensorValid(MASKTAMB)&& IsSensorValid(MASKTCOL) ) {
      if (STATECTRL != RUN) PrintInfo( 'i', F("Controller state changed to RUN"));
      STATECTRL = RUN;
      StateEngineMH();
      StateEngineSH();      
    }
    else if ( IsSensorValid(MASKTAMB)&& !IsSensorValid(MASKTCOL) ) {
      // patch for single zone usage
      if ( SYSTEM == 2 ) {
        if (STATECTRL != FAIL1) PrintInfo( 'i', F("Controller state changed to FAIL1"));
        STATECTRL = FAIL1;
      }
      else {
        if (STATECTRL != RUN) PrintInfo( 'i', F("Controller state changed to RUN"));
        STATECTRL = RUN; // ignore TCOL error
      }
      StateEngineMH();
      STATESH = OFF;      
    }
    else {
      if (STATECTRL != FAIL2) PrintInfo( 'i', F("Controller state changed to FAIL2"));
      STATECTRL = FAIL2; // ok?
      STATEMH = OFF;
      STATESH = OFF;      
    }
  }
  
  if ( ERRCTRL != 0 ) { // catch internal error => STOP
    if (STATECTRL != STOP) PrintInfo( 'i', F("Controller state changed to STOP"));
    STATECTRL = STOP;
    STATEMH = OFF;
    STATESH = OFF;
    // the only way to quit this mode is to reset the board or to send /data/put/request/run
  }
}

/*
 * === StateEngineMH() ===
 */
void StateEngineMH() {
  // Main heater
  boolean c1, c2, c3, c4;     // conditions
  TEMP tsetmh;                // internal setpoint -- depends on MODEMH

  // define tsetmh -- depends on MODEMH
  switch ( MODEMH ) {
    case HEAT:
      tsetmh = TSET;
      break;

    case ECO:
      tsetmh = TSET - DTECO;
      break;

    case FP:
      tsetmh = TFP;
      break;

    default:
      tsetmh = TSET;
  }
  
  switch ( STATEMH ) {
    case OFF:
      // compute the conditions to go ON
      c1 = (MODEMH != OFF);
      c2 = timerMH.check( 1000*TMHOFF );                 // is minimum time in the OFF state?
      c3 = (TAMB < tsetmh) && (STATESH == OFF);          // condition if STATESH = OFF
      c4 = (TAMB < tsetmh - AUG2) && (STATESH == ON);    // condition if STATESH = ON

      if ( c1 && c2 && (c3 || c4) ) {
        PrintInfo('i', F("Main heater switched to ON"));
        STATEMH = ON;
        timerMH.start();                               // restart timer
        counterhTCMH.run();                            // run time counter
      }
      break;

    case ON:
      // compute the conditions to go OFF
      c1 = (MODEMH == OFF);
      c2 = timerMH.check( 1000*TMHON );                      // is minimum time in the ON state?
      c3 = (TAMB > tsetmh + HYST) && (STATESH == OFF);       // condition if STATESH = OFF
      c4 = (TAMB > tsetmh + HYST - AUG2) && (STATESH == ON); // condition if STATESH = ON

      if ( c1 || (c2 && (c3 || c4)) ) {
        PrintInfo('i', F("Main heater switched to OFF"));
        STATEMH = OFF;
        timerMH.start();                              // restart timer
        counterhTCMH.stop();                          // stop time counter
      }
      break;

    default:
      // error in software
      ERRCTRL = 91; // at least... call the vendor! -- ERRCTRL = ???
      PrintInfo('w', F("Internal error ERRCTRL=91"));
  }  
}
/*
 * ===stateEngineSH() ===
 */
void StateEngineSH() {
  // Solar heater
  boolean c1, c2, c3, c4;     // conditions

  switch ( STATESH ) {
    case OFF:
      // compute the conditions to go ON
      c1 = (MODESH == ON);
      c2 = timerSH.check( 1000*TSHOFF );               // is minimum time in the OFF state?
      c3 = (TCOL > TAMB + DTSHON);                     // solar collector temp condition
      c4 = (TAMB < TSET + AUG1);                       // ambiant condition

      if ( c1 && c2 && c3 && c4 ) {
        PrintInfo('i', F("Solar heater switched to HEAT"));
        STATESH = ON;
        timerSH.start();                               // restart timer
        counterhINDSH.run();                           // run energy counter
        counterhTCSH.run();                            // run time counter
      }
      break;

    case ON:
      // compute the conditions to go OFF
      c1 = (MODESH == OFF);
      c2 = timerSH.check( 1000*TSHON );                // is minimum time in the ON state?
      c3 = (TCOL < TAMB + DTSHOFF);                    // solar collector temp condition
      c4 = (TAMB > TSET + HYST + AUG1);                // ambiant condition

      if ( c1 || (c2 && (c3 || c4)) ) {
        PrintInfo('i', F("Solar heater switched to OFF"));
        STATESH = OFF;
        timerSH.start();                               // restart timer
        counterhINDSH.stop();                          // stop energy counter
        counterhTCSH.stop();                           // stop time counter
      }
      break;

    default:
      // error in software
      ERRCTRL = 92; // at least... call the vendor!
      PrintInfo('w', F("Internal error ERRCTRL=92"));
  }  
}

/*
 * SetOutputs()
 */
void SetOutputs() {
  // 
  // Set the outputs regarding to the controller state
  // the controller state is defined in StateEngine()
  //
  // Set the heaters switches -- defined by STATEMH and STATESH

  // main heater
  if (STATEMH == ON) {
    SWMH = ON;
    digitalWrite( PINSWMH, HIGH );
  } 
  else {
    SWMH = OFF;
    digitalWrite( PINSWMH, LOW );
  }

  // solar heater
  if (STATESH == ON) {
    SWSH = ON;
    digitalWrite( PINSWSH, HIGH );
  }
  else {
    SWSH = OFF;
    digitalWrite( PINSWSH, LOW );
  }

  // switch user -- no error catch, simply copy the SWUSR value
  if (SWUSR == HIGH) {
    digitalWrite( PINSWUSR, HIGH );
  }
  else {
    digitalWrite( PINSWUSR, LOW );
  }
  //
  // set the LED1
  switch ( STATECTRL ) {
    
    case INIT:
      // show LED1
      LedBlinkingN( PINLED1, 100, 4 ); // 4 short pulses
      digitalWrite( PINLED1, HIGH );   // lighted during INIT phase
      break;

    case STOP:
      LedBlinkingN( PINLED1, 100, 5 ); // 5 pulses
      break;

    case RUN:
      // run phase: switches controlled by the states
      //LedBlinking( PINLED1, 100, &timerLED ); // RUN => short flash
      LedGlowing( PINLED1, 6740, 20, 100 );
      break;

    case FAIL1:
      LedBlinkingN( PINLED1, 100, 1 ); // one pulse
      break;
      
    case FAIL2:
      LedBlinkingN( PINLED1, 100, 2 ); // two pulses
      break;
  }
}

/*
 *  === RequestHandle() ===
 */
void RequestHandle() {
  //
  // Requests from datastore: data/put/request/request_type
  //
  // stop:          set STATECTRL to STOP
  //+run:           set STATECTRL to RUN
  // default:       retrieve the EEPROM values
  //+setdefault:    copy the current values into the EEPROM (default values)
  // rst_indsh:     reset the solar ventilation energy counter
  // rst_tcsh:      reset the solar ventilation time counter
  // rst_tcmh:      reset the main heater time counter
  //
  
  if ( ascdata.isRequest("stop") ) {
    PrintInfo( 'i', F("request = stop"));
    STATECTRL = STOP;      
  }
    
  else if ( ascdata.isRequest("run") ) {
    PrintInfo( 'i', F("request = run"));
    STATECTRL = RUN;      
  }

  else if ( ascdata.isRequest("default") ) {
    PrintInfo( 'i', F("request = default"));
      
    // get default data from EEPROM
    // no need to check the TAG
    ascdata.EEPROM_get( TAG10, 1 );
    // update saved values
     ascdata.EEPROM_put( TAG10, 0 );   

    // Update current counters values -- reset!
    counterhTCMH.set( TCMH );
    counterhTCSH.set( TCSH );
    counterhINDSH.set( INDSH );  
    // update datastore -- otherwise, current datastore data will be retrieved!
    ascdata.bridgePut('s'); // only the saved data are modified
  }
    
  else if ( ascdata.isRequest("setdefault") ) {
    PrintInfo( 'i', F("request = setdefault"));
    PrintInfo( 'i', F("EEPROM default data update with current values."));
     
    // save current data into EEPROM default data
    ascdata.EEPROM_put( TAG10, 1 );      
  }

  else if ( ascdata.isRequest("rst_indsh") ) {
    PrintInfo( 'i', F("request = rst_indsh"));

    INDSH = 0;
    counterhINDSH.set( INDSH );  // set the counter value

    // save the values 
    PrintInfo( 'i', F("EEPROM update with current values."));     
    // update EEPROM data with current values
    ascdata.EEPROM_put( TAG10, 0 );
         
    // update datastore -- to be sure!
    ascdata.bridgePut('s'); // only the saved data are modified
  }

  else if ( ascdata.isRequest("rst_tcmh") ) {
    PrintInfo( 'i', F("request = rst_tcmh"));

    TCMH = 0;
    counterhTCMH.set( TCMH );  // set the counter value

    // save the values 
    PrintInfo( 'i', F("EEPROM update with current values."));     
    // update EEPROM data with current values
    ascdata.EEPROM_put( TAG10, 0 );      
         
    // update datastore -- to be sure!
    ascdata.bridgePut('s'); // only the saved data are modified
  }

  else if ( ascdata.isRequest("rst_tcsh") ) {
    PrintInfo( 'i', F("request = rst_tcsh"));

    TCSH = 0;
    counterhTCSH.set( TCSH );  // set the counter value

    // save the values
    PrintInfo( 'i', F("EEPROM update with current values."));     
    // update EEPROM data with current values
    ascdata.EEPROM_put( TAG10, 0 );      
         
    // update datastore -- to be sure!
    ascdata.bridgePut('s'); // only the saved data are modified
  }

  else {
    PrintInfo('w', F("Undefined request"));
  }

/*
 * === end request handle ===
 */
}

// #############################
// utilities for ERRSENSOR usage
// #############################

/*
 * IsSensorValid()
 */
bool IsSensorValid( byte mask ) {
  // Return true if the sensor is ok
  return( (ERRSENSOR & mask) == 0 );
}

/*
 * ErrSensorRaise()
 */
void ErrSensorRaise( byte mask ) {
  // Raise an error on sensor
  ERRSENSOR = ERRSENSOR | mask;
}

/*
 * ErrSensorClear()
 */
void ErrSensorClear( byte mask ) {
  // Clear error on sensor
  ERRSENSOR = ERRSENSOR & ~mask;
}

// ################
// 1-Wire utilities
// ################

/*
 * OWsensorBegin()
 */
void OWsensorBegin(DallasTemperature sensor) {
  // OW begin
  // variable to get the device address of a 1-wire sensor
  DeviceAddress deviceAddress;
  //
  sensor.begin();
  if (sensor.getAddress(deviceAddress, 0)) {
    sensor.setResolution(deviceAddress, TEMPERATURE_RESOLUTION);
    sensor.setWaitForConversion(false);
    sensor.requestTemperatures(); // start conversion
  }
}

/*
 * ReadOWbus()
 */
TEMP ReadOWbus( DallasTemperature sensor, byte masksensor ) {
  // Get OW temperature data -- single device connected
  TEMP temp;
  //
  temp = int(100 * sensor.getTempCByIndex(0));  // the unique sensor has index 0
  sensor.requestTemperatures();                 // restart a conversion
  // test for bad reading
  if ( (temp > -5000) && (temp < 12500) ) {
    ErrSensorClear( masksensor );
  }
  else {
    ErrSensorRaise( masksensor );
  }
  return( temp );
}

