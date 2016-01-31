#include "DigitalOutput.h"
#include "DigitalInput.h"
#include "Valve.h"
#include "Timer.h"
#include "LoopFreq.h"

//============================================================================
//
// Debugging definitions
//
//============================================================================
#define DEBUG_LOOP_FREQ
#ifdef DEBUG_LOOP_FREQ
#   define DEBUG_SERIAL
    LoopFreq g_loopFreq( 5000 ); // Run 5000 loop()'s and report timing
#endif // DEBUG_LOOP_FREQ

#define DEBUG_LEAK
#ifdef DEBUG_LEAK
#   define DEBUG_SERIAL
#endif // DEBUG_LEAK

//============================================================================
//
// Globals
//
//============================================================================
enum Status {
   NORMAL = 0,
   LEAK = 1, // leak detected, don't close valves
   LEAK_SHUTOFF = 2, // leak detected, close valves
};
Status g_status = NORMAL;

// Push button switch.  Short press to clear a leak status.  Long
// press for "test" mode which toggles the leak status.  Wired
// GND->button->PIN.  Wired to A0 (LOW = on)
DigitalInput g_button;

// Status LED.   Wired to D6.
DigitalOutput g_led;

// Articulated valve.  See init for wiring.
Valve g_valve;

// Leak sensor.  These must be wired 5V->sensor->PIN with a pull down
// resistor from PIN->GND.  Wired to D3
DigitalInput g_sensor;

//============================================================================
void statusChange( Status status );
void buttonChangedCb( DigitalInput::Status status, int8_t buttonId );
void sensorChangedCb( DigitalInput::Status status, int8_t switchId  );
void valveChangedCb( Valve::Status status, int8_t valveId );

//============================================================================
//
// Arduino main program
//
//============================================================================
void setup()
{
#ifdef DEBUG_SERIAL
   Serial.begin( 19200 );
#endif // DEBUG_SERIAL
   
   g_button.init( 14, LOW ); // A0
   g_sensor.init( 3, HIGH );
   g_led.init( 6 );

   // Get the initial valve status to set the system into the correct
   // state.
   Valve::Status status = g_valve.init(
         16, // control open pin A2
         17, // control close pin A3
         18, // report open pin A4
         19, // report close pin A5
         10000, // 10 sec, max power on time
         10000  // 10 sec, cool down time between cycling
         );
   valveChangedCb( status, 0 );

   // If either valve isn't in the full opened state, there is a
   // problem so switch to leak alert status.
   if ( status != Valve::OPENED )
   {
      statusChange( LEAK_SHUTOFF );
   }
}

//============================================================================
void loop()
{
#ifdef DEBUG_LOOP_FREQ
   g_loopFreq.poll();
#endif // DEBUG_LOOP_FREQ
   
   long t = millis();

   // Poll the LED for blinking.
   g_led.poll( t );

   // Poll each item.  If the item state changes, the callback
   // function will be run.
   g_button.poll( t, buttonChangedCb );
   g_sensor.poll( t, sensorChangedCb );
   g_valve.poll( t, valveChangedCb );
}

//============================================================================
void
statusChange( Status status )
{
   if ( status == g_status )
   {
      return;
   }

#ifdef DEBUG_LEAK
   Serial.print( "Status  -> " ); Serial.println( status );
#endif   
   
   switch( status )
   {
   case NORMAL:
      // Going back to normal operations.  Turn off the LED and open
      // the valve.
      g_led.off();
      g_valve.open();

      // TODO: send message, leak cleared.
      break;
      
   case LEAK:
      // Leak in secondary sensor - don't close the valves.
      g_led.blinkSlow();

      // TODO: send message, leak, valves opened
      break;

   case LEAK_SHUTOFF:
      // Leak in primary sensor - close the valves.
      g_valve.close();
      g_led.blinkFast();

      // TODO: send message, leak, valves closing
      break;

   }

   g_status = status;
}
   
//============================================================================
void
buttonChangedCb( DigitalInput::Status status,
                 int8_t buttonId )
{
#ifdef DEBUG_LEAK
   Serial.print( "Button " ); Serial.print( buttonId );
   Serial.print( " -> " ); Serial.println( status );
#endif
   
   switch( status )
   {
   case DigitalInput::NONE:
   case DigitalInput::CLOSED:
      break;

   case DigitalInput::OPENED:
      // Normal button release will reset the status from leak to
      // normal.
      statusChange( NORMAL );
      break;
      
   case DigitalInput::OPENED_LONG:
      // Long button press will enter "test" mode and toggle the leak
      // status.
      switch( g_status )
      {
      case NORMAL:
         statusChange( LEAK );
         break;
      case LEAK:
      case LEAK_SHUTOFF:
         statusChange( NORMAL );
         break;
      }
      break;
   }
}

//============================================================================
void
sensorChangedCb( DigitalInput::Status status,
                 int8_t sensorId )
{
   switch( status )
   {
   case DigitalInput::NONE:
      break;
      
   case DigitalInput::CLOSED:
      // Sensor tripped - we have a leak.  
      statusChange( LEAK_SHUTOFF );
      break;

   case DigitalInput::OPENED:
   case DigitalInput::OPENED_LONG:
      // Ignore sensor releases.  If there was a leak, it requires a
      // manual reset using the push button switch.

      // TODO: send message, leak no longer detected.
      break;
   }
}

//============================================================================
void
valveChangedCb( Valve::Status status,
                int8_t valveId )
{
#ifdef DEBUG_LEAK
   Serial.print( "Valve " ); Serial.print( valveId );
   Serial.print( " -> " ); Serial.println( status );
#endif
   
   // Valve state changes toggle the two LED's back and forth to
   // indicate the valve state.
   switch( status )
   {
   case Valve::NONE:
      break;
      
   case Valve::OPENED:
      // TODO: send message
      g_led.off();
      break;
   case Valve::OPENING:
      // TODO: send message
      g_led.blinkFast();
      break;
      
   case Valve::CLOSED:
      // TODO: send message
      g_led.on();
      break;
   case Valve::CLOSING:
      // TODO: send message
      g_led.blinkFast();
      break;
      
   case Valve::UNKNOWN:
      // TODO: send message
      g_led.blinkFast();
      break;
      
   case Valve::STALLED:
      // TODO: send message
      g_led.blinkSlow();
      break;
   }
}

//============================================================================
