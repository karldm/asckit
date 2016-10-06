/*
 * util.h
 *
 * Utilitary functions
 * in/out serial port
 *
 */

#ifndef ascutil_h
#define ascutil_h

#define ONEHOURMS   3600000  // one hour in millis
#define ONEDAYMS   86400000  // one days in millis
#define CONSOLE              // activate the Console -- skip to save memory

#include <arduino.h>
#include <string.h>
#include <Bridge.h>
#include <Console.h>

// Timer
class Timer
{
	public:
	Timer( unsigned long delayMillis );
	void start();
	bool check();
	bool check( unsigned long delayMillis );
	unsigned long elapsed();

	private:
	unsigned long _delay;
	unsigned long _starTimeMillis;
 };

// Counterh
class Counterh
{
	public:
	Counterh( unsigned long index );             // set the index value / begin in 'stop' state
  bool run();
  bool stop();
  bool tic();
	unsigned long set( unsigned long index );
	unsigned long add( unsigned long curvalue );
	unsigned long get();

	private:
	unsigned long _index;
	unsigned long _addedMillis;
	unsigned long _lastSamplingMillis;
	bool _running;
 };
 
//
void SetFname( char * fname);
void SetFname( const __FlashStringHelper * fname ); // ! TO CHECK???
//
void LedBlinking(int pin, int delayonms, Timer * timerLED );
void LedBlinkingN(int pin, int delayms, int n );
void LedGlowing(int pin, int periodms, int minl, int maxl );
//
void BeginInfo();                                     // start the console if CONSOLE if defined
void PrintInfoDataUsage( int npar, int nparmax );    // message on console for data usage
void PrintInfo( const char type, const char * message);
void PrintInfo( const char type, const __FlashStringHelper * message);
void PrintInfo( const char type, const char * format, const char * val );

#endif
