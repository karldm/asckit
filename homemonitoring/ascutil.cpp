/*
   ascutil.cpp
   
   Arduino Solar Controller
   by karldm, Feb 2017
   
   The MIT License (MIT)

   Copyright (c) 2017 www.renergia.fr

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

#include "ascutil.h"
#define FUN_NAME_SIZE 20
#define MES_BUF_SIZE  50

static char activefname[] = "ASC";  // id on console messages
static char mesbuf[MES_BUF_SIZE];

/*
 * #####
 * Timer
 * #####
 * 
 * declare Timer timer( delay)
 * timer.delay()
 * timer.start()
 * timer.check()
 *
 */
 Timer::Timer( unsigned long delayMillis )
 {
   _delay = delayMillis;
   _starTimeMillis = millis(); // start the timer
 }

void Timer::start()
{
  // reset the timer
  _starTimeMillis = millis();
}

bool Timer::check()
{
  // check and restart if timeout
  bool istimeout;
  istimeout = (millis() - _starTimeMillis) > _delay;
  
  if ( istimeout ) 
  {
    _starTimeMillis = millis();
  }
  return( istimeout );
}

bool Timer::check( unsigned long delayMillis )
{
  // check if timeout (only)
  // require the delay
  bool istimeout;
  istimeout = (millis() - _starTimeMillis) > delayMillis;
  return( istimeout );
}

unsigned long Timer::elapsed()
{
  // return elapsed time
  return( millis() - _starTimeMillis );
}

/*
 * ########
 * Counterh
 * ########
 * 
 * Cumulative counter in x.hour
 * Counter index( unsigned long valueh )
 * index.tic()         // start or stop
 * index.add(x)        // x=1 for hours, W for Wh
 * index.get()
 */
 Counterh::Counterh( unsigned long index )
 {
   _index = index;
   _addedMillis = 0;
   _lastSamplingMillis = 0; // last sampling time
   _running = false;        // use .run to start
 }

bool Counterh::run()
{
  // activate counting
  _running = true;
  _lastSamplingMillis = millis(); // so you don't need to call it if not running
  return(_running);
}

bool Counterh::stop()
{
  // stop counting
  _running = false;
  _lastSamplingMillis = millis();
  return(_running);
}

bool Counterh::tic()
{
  // run <-> stop counting
  _running = !_running;
  _lastSamplingMillis = millis();
  return(_running);
}

unsigned long Counterh::set( unsigned long index )
{
  // set the counter value
  _index = index;
  return(_index);
}

unsigned long Counterh::add( unsigned long curvalue )
{
  // add and check if index to be increased
  // beware! only positive values can be added!
  unsigned long intpart;
  unsigned long curMillis;

  curMillis = millis(); // current time

  if ( _running ) {
    _addedMillis += curvalue*( curMillis - _lastSamplingMillis);
  
    // check if index is to be incremented
    intpart = _addedMillis / ONEHOURMS; // 1 hour in millis
    if ( intpart > 0 )
    {
      _index += intpart;
  	  _addedMillis = _addedMillis % ONEHOURMS;; // keep the millis part
    }
  }
  
  _lastSamplingMillis = curMillis; // set the last sampling time
  
  return(_index);
}

unsigned long Counterh::get()
{
  // get the counter value
  return(_index);
}

/*
 * Declare the message origin
 */
void SetFname( const char * fname ) 
{
  snprintf( activefname, FUN_NAME_SIZE, "%s", fname );
}

void SetFname( const __FlashStringHelper * fname ) 
{
   // ??? TO CHECK
   strcpy_P( activefname, (char *)fname ); // achtung!
}

/*
 * ####################
 * Various LED controls
 * ####################
 */

 /*
  *  Blink a LED once
  */
void LedBlinking(int pin, int delayonms, Timer * timerLED )
{
  //
  if ( timerLED->check() ) {

    digitalWrite( pin, HIGH );
    delay( delayonms );
    digitalWrite( pin, LOW );
  }
}

/*
 *  Blink a LED N times
 */
void LedBlinkingN(int pin, int delayms, int n )
{
  // blink the LED N times with delay()
  digitalWrite( pin, LOW ); //  begin with LOW
  delay( delayms );

  for (int i=0; i < n; i++) {
    digitalWrite( pin, HIGH );
    delay( delayms );
    digitalWrite( pin, LOW );
    delay( 2*delayms );
  }
}

/*
 *  Glow a LED -- only with PWD~ outputs
 */
void LedGlowing(int pin, int periodms, int minl, int maxl )
{
  //
  unsigned long i = millis();
  unsigned long frac;
  int level;
  unsigned long j = i % periodms;
  if (j > periodms / 2) {
    j = periodms - j;
  }
  frac = minl + 2 * j * (maxl - minl) / periodms;
  level = (int) frac;
  analogWrite(pin, level );
}

/*
 * ###########################################
 * Messages and logs on serial port or console
 * ###########################################
 * 
 * Console management on Yun
 * Call BeginInfo() to open the Console => wait for connection
 * do it after Bridge.begin()
 * Need to define CONSOLE
 */

 /*
  * Open the console on Yun
  * if CONSOLE is not defined, just do nothing...
  */
void BeginInfo() {
//
#ifdef CONSOLE
  Console.begin();
  /*
  while (!Console) {
    ; // wait for Console port to connect -- for debug only
  }
  */
#endif

}

void PrintInfoDataUsage( int npar, int nparmax ) {
//
#ifdef CONSOLE
  Console.print(activefname);
  Console.print(",i,Data usage: parameters ");
  Console.print( npar );
  Console.print("/");
  Console.print( nparmax );
  Console.print("(");
  Console.print( npar*100/nparmax );
  Console.println("%)");
#endif
}

void PrintInfo( const char type, const char * message ) {
  //
#ifdef CONSOLE
  if (type == 'i' || type == 'w' || type == 'd') 
  {
    Console.print(activefname);
    Console.print(',');
    Console.print(type);
    Console.print(',');
    Console.println(message);
  }
#endif
}

void PrintInfo( const char type, const __FlashStringHelper * message ) {
  //
#ifdef CONSOLE
  if (type == 'i' || type == 'w' || type == 'd')
  {
    Console.print(activefname);
    Console.print(',');
    Console.print(type);
    Console.print(',');
    Console.println(message);
  }
#endif
}

void PrintInfo( const char type, const char * format, const char * val ) {
  // print a formatted value
#ifdef CONSOLE  
  if (type == 'i' || type == 'w' || type == 'd')
  {
    snprintf( mesbuf, MES_BUF_SIZE, format, val );
    Console.print( activefname);
    Console.print( ',');
    Console.print( type);
    Console.print( ',');
    Console.println( mesbuf);
  }
#endif
}

