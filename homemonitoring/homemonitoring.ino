/*
   Home Monitoring - ASCKit Shield v1.3

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
#define VERSION "hm.1.0d"         // Software version 
#define TAG10   "HMASCKit.."      // Tag for EEPROM data structure check -- unused

// Relays states : ON - OFF
#define OFF     0                  // OFF mode or state
#define ON      1                  // ON state

//
// CONTROLLER
// states : STOP - RUN - INIT - FAIL
// 
#define STOP       0                // Controller STOP state or mode
#define RUN        1                // Controller RUN state or mode
#define INIT       2                // Controller INIT state -- power onP
#define FAIL       3                // Controller FAIL state -- TAMB measurements error

// Hardware configuration
#define PINDHT     2                // DHT sensor bus
#define PINLED1    3                // White LED pin number
#define PINOW1     4                // OW1 Bus pin number (single sensor TUSR3)
#define PINOW2     5                // OW2 Bus pin number (single sensor TUSR4)
#define PINOW3     6                // OW3 Bus pin number (single sensor TUSR1)
#define PINOW4     7                // OW4 Bus pin number (single sensor TUSR2)
#define PINSWUSR1 12                // SWUSR1 pin number (U on the shield)
#define PINSWUSR2  8                // SWUSR2 pin number (H on the shield)
#define PINSWUSR3  9                // SWUSR3 pin number (SH on the SolarBoard)

// sensors
// masks for the ERRSENSOR parameter (use for error diagnostic)
#define MASKTAMB   0b00000001
#define MASKHAMB   0b00000010
#define MASKTUSR3  0b00000100
#define MASKTUSR4  0b00001000
#define MASKTUSR1  0b00010000
#define MASKTUSR2  0b00100000

#define TEMPERATURE_RESOLUTION 12  // nb bits for DS18B20 sensors

//=== DHT SENSOR MODEL ===
#define DHTTYPE DHT22            // DHT 22  == AM2302 == AM2321

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
TEMP   TUSR1;              // temperature user#1 (.01 degC)
TEMP   TUSR2;              // temperature user#2 (.01 degC)
TEMP   TUSR3;              // temperature user#3 (.01 degC)
TEMP   TUSR4;              // temperature user#4 (.01 degC)

/*
 * Parameters
 */
TEMP  DTDHT = 0;           // Overheating of the DHT sensor -- depends on the ackage and position

/*
 * Calculated
 */
int    NLOOPS = 0;         // number of loops per sec
byte   SWUSR1 = OFF;       // user switch#1
byte   SWUSR2 = OFF;       // user switch#2
byte   SWUSR3 = OFF;       // user switch#3
byte   STATECTRL = INIT;   // controller INIT state at power on
byte   ERRSENSOR = 0;      // error status on sensor -- begin with no error
byte   ERRCTRL = 0;        // internal error id# -- go to STOP mode if != 0

/************************************************************/
/********** END PARAMETERS DEFAULT VALUES *******************/
/************************************************************/

// Various timers - see usage in util.cpp
// declare delays in millis
Timer  timerOW( 750 / (1 << (12 - TEMPERATURE_RESOLUTION)) ); // min delay for DS18B20 convertion
Timer  timerDHT( 2000 );          // DHT readout delay, may be reduced...
Timer  timerLED( 3456 );          // flash the LED1
Timer  timerBridge( 500 );        // synchronize bridge data (i.e. commands responsivity...)

// DHT sensor bus
DHT dht(PINDHT, DHTTYPE);

// 1-Wire buses & sensors
OneWire ow1(PINOW1);    // for TUSR3
OneWire ow2(PINOW2);    // for TUSR4
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
  pinMode( PINDHT, OUTPUT ); // done after
  pinMode( PINSWUSR1, OUTPUT );
  pinMode( PINSWUSR2, OUTPUT );
  pinMode( PINSWUSR3, OUTPUT );

  // setoutputs -- check STATECTRL = INIT
  // done azap -- set switches in OFF position
  STATECTRL = INIT; // force the INIT state
  SetOutputs();     // set the outputs

  // bridge
  Bridge.begin();   // start the script /usr/bin/run-bridge on the MPU side
  
  // infos to the console by PrintInfo()
  // only for debug purpose! It's waiting for the console for ever
  BeginInfo();      // start the Console if CONSOLE if defined in util.h -- debug only
  PrintInfo('i', "Home Monitoring, v%s", VERSION);

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

  // Calculated
  ascdata.par_F( &NLOOPS,   F("nloops"),   F("p i"));

  // sensors & switches
  ascdata.par_F( &TAMB,     F("tamb"),     F("p f4.2"));
  ascdata.par_F( &HAMB,     F("hamb"),     F("p f4.2"));
  ascdata.par_F( &TUSR1,    F("tusr1"),    F("p f4.2"));
  ascdata.par_F( &TUSR2,    F("tusr2"),    F("p f4.2"));
  ascdata.par_F( &TUSR3,    F("tusr3"),    F("p f4.2"));
  ascdata.par_F( &TUSR4,    F("tusr4"),    F("p f4.2"));
  ascdata.par_F( &SWUSR1,   F("swusr1"),   F("gs i"));     // user switch -- saved
  ascdata.par_F( &SWUSR2,   F("swusr2"),   F("gs i"));     // user switch -- saved
  ascdata.par_F( &SWUSR3,   F("swusr3"),   F("gs i"));     // user switch -- saved

  // parameters
  ascdata.par_F( &DTDHT,    F("dtdht"),    F("gs f4.2"));

  // states
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
  static Timer timerLoops( 1000 ); // calcultate the nb of loops executed par sec
  int index;

  ReadSensors();
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

  }
}

/*
 * === ReadSensors() ===
 */
void ReadSensors() {
  // read the sensors
  // what if an error occur?
  // no check for time serie (i.e. glitches)
  float dhtval;

  /////////////////////
  // Read DHT sensor //
  // TAMB & HAMB     //
  /////////////////////
  if ( timerDHT.check() ) {
    
    dhtval = dht.readTemperature(); // default is Celsius
    if (isnan(dhtval)) {
      // FAIL
      if ( IsSensorValid( MASKTAMB ) ) PrintInfo( 'w', F("Failed to read from DHT sensor TAMB!...")); // only once
      ErrSensorRaise( MASKTAMB );
    }
    else {
      if ( !IsSensorValid( MASKTAMB ) ) PrintInfo( 'i', F("DHT sensor TAMB ok...")); // only once
      TAMB = int(100 * dhtval) - DTDHT; // correction for overheating
      ErrSensorClear( MASKTAMB );
    }
    
    dhtval = dht.readHumidity();
    if (isnan(dhtval)) {
      // soft FAIL
      if ( IsSensorValid( MASKHAMB ) ) PrintInfo( 'w', F("Failed to read from DHT sensor HAMB!...")); // only once
      ErrSensorRaise( MASKHAMB );
    }
    else {
      if ( !IsSensorValid( MASKHAMB ) ) PrintInfo( 'i', F("DHT sensor HAMB ok...")); // only once
      HAMB = int(100 * dhtval);
      //simple correction for humidity
      HAMB = HAMB + 100*int(DTDHT/(2*(100-int(HAMB/100))/3+6));
      ErrSensorClear( MASKHAMB );
    }
  }

  if ( timerOW.check() ) {
    //
    // read 1-wire sensors
    // order compatible with controller!
    //
    TUSR3 = ReadOWbus( sensor1, MASKTUSR3 );
    TUSR4 = ReadOWbus( sensor2, MASKTUSR4 );
    TUSR1 = ReadOWbus( sensor3, MASKTUSR1 );
    TUSR2 = ReadOWbus( sensor4, MASKTUSR2 );
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
  // Set the user switches
  // switch user #1 -- no error catch, simply copy the SWMH value
  if (SWUSR1 == HIGH) {
    digitalWrite( PINSWUSR1, HIGH );
  }
  else {
    digitalWrite( PINSWUSR1, LOW );
  }

  // switch user#2 -- no error catch, simply copy the SWUSR value
  if (SWUSR2 == HIGH) {
    digitalWrite( PINSWUSR2, HIGH );
  }
  else {
    digitalWrite( PINSWUSR2, LOW );
  }

  // switch user#3 -- no error catch, simply copy the SWUSR value
  if (SWUSR3 == HIGH) {
    digitalWrite( PINSWUSR3, HIGH );
  }
  else {
    digitalWrite( PINSWUSR3, LOW );
  }
  
  //
  // set the LED1
  switch ( STATECTRL ) {
    
    case INIT:
      // show LED1
      LedBlinkingN( PINLED1, 100, 4 ); // 4 short pulses....
      digitalWrite( PINLED1, HIGH );   // then lighted during INIT phase
      break;

    case STOP:
      LedBlinkingN( PINLED1, 100, 5 ); // 5 pulses
      break;

    case RUN:
      // run phase: switches controlled by the states
      //LedBlinking( PINLED1, 100, &timerLED ); // RUN => short flash
      LedGlowing( PINLED1, 6740, 0, 80 );
      break;

    case FAIL:
      LedBlinkingN( PINLED1, 100, 1 ); // one pulse
      break;
  }
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

