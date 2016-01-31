#pragma once
#include "Arduino.h"
#include "Timer.h"

// Simple digital output control class for LED's, relays, etc.
//
// Create the object and call init() with the pin number in setup().
// Then call poll() in the loop function with the current time from
// millis().  The load can be turned on or off, toggled, or set to
// blink for a certain number of times.
//
// Can also be used to control loads on a shift register in which case
// the class will toggle a specific bit in a buffer and the caller is
// responsible for shifting the buffer out on each loop.
//
// If onState is HIGH, then the arduino pin is set HIGH to turn the
// load on and LOW to turn the load off.  Wiring should be (resistor
// should be added as well for LED's):
//    pin -> LOAD+ -> LOAD- -> GND
//
// If onState is LOW, then the arduino pin is set LOW to turn the load
// on and HIGH to turn the load off.  Wiring should be (resistor
// should be added as well LED's):
//    pin -> LOAD- -> LOAD+ -> VCC
//
class DigitalOutput
{
public:
   // It would be nice to use constructors instead of init() methods
   // but Arduino generally requires these to be global variables and
   // we can't control if the constructor gets called before or after
   // the system is initialized.  So init routines are used instead of
   // constructors.
   
   // Wired to pin
   void init( uint8_t pin, uint8_t onState=HIGH );
   // Wired to shift register.
   void initShift( uint8_t* buffer, uint8_t bitIndex, uint8_t onState=HIGH );

   // NOTE: long is better than unsigned long - code can ignore roll
   // overs for duration computations.  For details, see:
   // http://playground.arduino.cc/Code/TimingRollover
   void poll( long currentMillis );
   
   bool isOn();
   
   void on( long durationMillis=0 );
   void off();
   void toggle();

   void blinkSlow( int num=-1 );
   void blinkFast( int num=-1 );
   void blink( int num, long blinkMillis );

private:
   // If m_info.isPin is 1, then m_pin is the pin to use and m_buffer
   // is ignored.  If m_info.isPin is 0, then m_buffer points at the
   // output shift register value to push the output into and m_pin is
   // the index of the bit in the buffer to set.
   uint8_t m_pin;
   uint8_t* m_buffer;

   // Bit field struct to save storage space.
   typedef struct {
      // 1 if output is to a pin.  0 if it's to a shift register.
      uint8_t isPin : 1;

      // LOW (0) or HIGH (1) state to set the pin to turn the load on.
      uint8_t onState : 1;

      // 1 if the output is on or blinking, 0 if it's off.
      uint8_t isActive : 1;

      // 1 if the output is currently turned on, 0 if it's off.
      // During blinking, this will toggle with the device vs isActive
      // which will stay 1 during blinking.
      uint8_t isOn : 1;
   } Info;

   // Output info. see above.
   Info m_info;

   // Timer for handling blinking.
   Timer m_timer;

   // These work in pin values (0=low, 1=high), not on/off
   void setState( bool state );
};

//============================================================================
// Initialize an output connected to a pin.
//
//= INPUTS
//- pin       The pin the load is connected to.
//- onState   LOW or HIGH, the state to set the pin to turn the load on.
//
inline
void
DigitalOutput::
init( uint8_t pin,
      uint8_t onState )
{
   m_pin = pin;
   m_buffer = 0;
   m_info.isPin = 1;
   m_info.onState = onState;
   m_info.isActive = 0;
   m_info.isOn = 0;
   
   pinMode( m_pin, OUTPUT );
   off();
}

//============================================================================
// Initialize an output connected to a shift register.
//
//= INPUTS
//- buffer    Pointer to the shift register buffer to use.  
//- pin       The Arduino pin the load is connected to.
//- onState   LOW or HIGH, the state to set the pin to turn the load on.
//
inline
void
DigitalOutput::
initShift( uint8_t* buffer,
           uint8_t bitIndex,
           uint8_t onState )
{
   m_pin = bitIndex;
   m_buffer = buffer;
   m_info.isPin = 0;
   m_info.onState = onState;
   m_info.isActive = 0;
   m_info.isOn = 0;

   off();
}

//============================================================================
// Return true if the load is on or is blinking.
//
inline
bool
DigitalOutput::
isOn()
{
   return m_info.isActive;
}

//============================================================================
// Blink slowly (once per second) 
//
//= INPUTS
//- num    The number of times to blink or -1 to blink forever.
//
inline
void
DigitalOutput::
blinkSlow( int num )
{
   blink( num, 1000 );
}

//============================================================================
// Blink quickly (once per 100ms) 
//
//= INPUTS
//- num    The number of times to blink or -1 to blink forever.
//
inline
void
DigitalOutput::
blinkFast( int num )
{
   blink( num, 100 );
}

//============================================================================
// Blink the load.
//
//= INPUTS
//- num          The number of times to blink or -1 to blink forever.
//- blinkMillis  Number of milliseconds to use for each on and off period.
//
inline
void
DigitalOutput::
blink( int num,
       long blinkMillis )
{
   // Convert number of blinks to number of timer firings.  -1 ==
   // infinite.
   if ( num > 0 )
   {
      // Timer fires once for on and once off so send twice the input
      // pulses.  Subtract once since we're toggling the state below
      // so the initial on pulse is already accounted for.
      num = 2 * num - 1;
   }

   m_timer.repeat( blinkMillis, num );

   m_info.isActive = 1;
   setState( m_info.onState );
}

//============================================================================
// Turn the load on.
//
// This will also cancel any remaining blinks.
//
//= INPUTS
//- durationMillis   Number of milliseconds to turn on for. If this is
//                   zero (default), then it will stay on until off() is
//                   called.
inline
void
DigitalOutput::
on( long durationMillis )
{
   // blink once for the input duration.
   if ( durationMillis )
   {
      blink( 1, durationMillis );
   }
   // Turn the pin on and turn off any blinking that might be happening.
   else
   {
      setState( m_info.onState );
      m_timer.off();
   }

   m_info.isActive = 1;
}

//============================================================================
// Turn the load off.
//
// This will also cancel any remaining blinks.
//
inline
void
DigitalOutput::
off()
{
   setState( ! m_info.onState );
   m_info.isActive = 0;
   m_timer.off();
}

//============================================================================
// Toggle the load state.
//
// This will also cancel any remaining blinks.
//
inline
void
DigitalOutput::
toggle()
{
   if ( m_info.isActive )
   {
      off();
   }
   else
   {
      on();
   }
}

//============================================================================
// Poll the object.
//
// This should be called in each loop().  
//
//= INPUTS
//- currentMillis   The current elapsed time in milliseconds.
//
inline
void
DigitalOutput::
poll( long currentMillis )
{
   // If the timer triggers, toggle the state.
   int8_t left = m_timer.poll( currentMillis );
   if ( left )
   {
      // Last timer firing - turn everything off.
      if ( left == 1 )
      {
         off();
      }
      // Otherwise toggle the current state.
      else
      {
         setState( ! m_info.isOn );
      }
   }
}

//============================================================================
// Set the pin state.
//
// Input is the pin state (0=low, 1=high) to set - not the on/off state.
//
inline
void
DigitalOutput::
setState( bool state )
{
   if ( m_info.isPin )
   {
      digitalWrite( m_pin, state );
   }
   else
   {
      bitWrite( *m_buffer, m_pin, state );
   }

   m_info.isOn = ( state == m_info.onState );
}

//============================================================================
