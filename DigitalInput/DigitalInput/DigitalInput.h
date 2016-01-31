#pragma once
#include "Arduino.h"

// Debounced switch (digital input) class.
//
// Used to read the state of a digital (on/off) inputs (switches,
// button, etc) that must remain stable for a certain amount of time.
// Create the DigitalInput, call init() once to configure the pins.  Then
// call poll() periodically and act on the return value or pass in a
// callback function which will be run when the switch state changes.
//
//= Wiring the input to an Arduino pin
//
// Use the onState input in the init() routine to set how the switch
// is wired:
//
// 1) GND -> switch -> pin
//    Pass LOW to init() as the onState.  Pin will be set as INPUT_PULLUP.
//
// 2) VCC -> switch -> pin
//                   \- resistor -> GND
//    Pass HIGH to init() as the onState.  Pin will be set as INPUT.
//
// Code is based on the bounch2 library (and others)
// from here: https://github.com/thomasfredericks/Bounce2/wiki
//
// digitalRead() is used normally to test the switch.  If a pin
// doesn't support that (A6, A7 on Pro-Mini's), then set the model to
// ANALOG in the init() call.  In that case, analagRead(pin) > 128 is
// used in place of digitalRead().
//
// IMPORTANT: A6 and A7 have no internal pull resistors.  So you must
// use an external pull up (if onState is LOW) or a pull down (if on
// state is HIGH) resistor.
//
//= Wiring the switch using a shift register.
//
// If the switch is hooked up to a shift register, then the on/off
// value is read outside of the DigitalInput code.  In that case, use
// the initShift() method that doesn't require the input pin to set up
// the DigitalInput.  Pass in a pointer to the integer into which the
// shift register will be loaded.  During loop(), read the shift
// register first and call poll() with the value of the switch.
//

// Time the input must be held for a "long" vs short close.
#define DIGITALINPUT_LONG_CLOSE_TIME 2000 // millis

//#define DIGITALINPUT_DEBUG

class DigitalInput
{
public:
   // Return codes from poll().  Use < 0 for any release event.
   enum Status {
      NONE = 0,
      CLOSED = 1,
      OPENED = -1,
      OPENED_LONG = -2, // opened after a long time closed
   };
   // digitalRead() is used normally to test the pin.  If a pin
   // doesn't support that (A6, A7 on Pro-Mini's), then set the mode
   // to ANALOG.  If using ANALOG, an external pull up (onState=LOW)
   // or down (onState=HIGH) resistor MUST be used.
   enum ReadMode {
      DIGITAL = 0,
      ANALOG = 1,
   };

   // DigitalInput state change callback.  Called by poll() when the
   // status of the input changes.  Input is the status and the
   // identifier passed to poll.
   typedef void (*StateChangeCb)( Status status, int8_t identifier );

   // NOTE: Can't use constructors (even though we should) because
   // Arduino sketch generally requires these to be global variables
   // so we don't know that the ctor would be called after hardware
   // init.

   // DigitalInput wired to an Arduino pin
   void init( uint8_t pin, uint8_t onState=LOW, ReadMode mode=DIGITAL,
              uint8_t debounceMillis=5, bool initPin=true );
   
   // DigitalInput wired to a shift register.
   void initShift( uint8_t* buffer, uint8_t bitIndex, uint8_t onState=LOW,
                   uint8_t debounceMillis=5, bool initialState=false );
   
   // NOTE: long is better than unsigned long for milli values - code
   // can ignore roll overs for duration computations.  For details,
   // see: http://playground.arduino.cc/Code/TimingRollover
   Status poll( long currentMillis, StateChangeCb callback=NULL,
                int8_t identifier=0 );

   bool isOn();     // with debouncing
   bool isOnRaw();  // without debouncing
   bool pressed();
   bool released();

private:
   // If m_buffer is null, then m_pin is the Arduino pin to control.
   // If m_buffer is non-null, then it points at the output shift
   // register value to push the output into and m_pin is the index of
   // the bit in the buffer to set.
   uint8_t m_pin;
   uint8_t* m_buffer;

   // Bit field struct to save storage space.
   typedef struct {
      // 1 if reading from a pin (m_buffer == null), 0 otherwise.
      uint8_t isPin : 1;
      
      // Flag for reading the pin using digitalRead() (1) or
      // analogRead() (0).  For almost all pins, this is true.  If
      // this is false, analagRead(pin) > 128 is used in place of
      // digitalRead().  See setAnalog() for details.  If isPin==0, it
      // will set to 1.
      uint8_t isDigital : 1;

      // LOW (0) or HIGH (1) - indicates which state the input is in
      // when it's activated.
      uint8_t onState : 1;
      
      // Current DigitalInput state on the last read (unstable value)
      // (1=on, 0=off) This is on/off and not high/low.
      uint8_t unstable : 1;
      
      // Last stable input state (debounced value) (1=on, 0=off)
      // This is on/off and not high/low.
      uint8_t stable : 1;

      // Did the state change from unstable to stable on the last read
      // (0=no, 1=yes) - ie did m_debounceMillis elapse with a
      // constant value on the last read.
      uint8_t changed : 1;
      
      // Time the input was pressed (0=short, 1=long).  Could be
      // expanded in the future with other values if needed (1 more
      // bit available).
      uint8_t longPress : 1;
   } Info; // 1 byte

   // DigitalInput info. see above.
   Info m_info;
   
   // Number of milliseconds the input must be in the same state
   // before it's reported.
   uint8_t m_debounceMillis;
   
   // Time in millis for a stable (debounced) value.  This is set when
   // the input changes value.  If the input stays in the same state
   // until this time, then it's stable.  This assumes that poll() is
   // called more often than m_debounceMillis of course.
   long m_stopMillis;
};

//============================================================================
#ifdef DIGITALINPUT_DEBUG
#   define DIGITALINPUT_DBG( s, i ) Serial.print( s ); Serial.println( i ); 
#else
#   define DIGITALINPUT_DBG( s, i ) 
#endif

//============================================================================
// Return if the pin is current on or not.
//
// This ignores the debouncing (stable) value and just returns whether
// or not the pin matches the on state of the input.  If the input
// is attached to a shift register, the register integer bit field
// must be updated before this is called.
//
inline
bool
DigitalInput::
isOnRaw()
{
   // Extract a bit from a shift register buffer.
   if ( ! m_info.isPin )
   {
      // read bit m_pin from the buffer.
      return bitRead( *m_buffer, m_pin ) == m_info.onState;
   }
   // Normal digital pin.
   else if ( m_info.isDigital )
   {
      return digitalRead( m_pin ) == m_info.onState;
   }
   // Analog only pin to act like a digital pin.
   else
   {
      return ( analogRead( m_pin ) > 128 ) == m_info.onState;
   }
}

//============================================================================
// Create a digital input attached to a pin.
//
//= INPUTS
//- pin             The Arduino pin the input is connected to.
//- onState         LOW or HIGH, the state the input is in when it's on.
//- mode            Use digital read or analog read to test the pin.
//- debounceMillis  Number of milliseconds the input must be in the same state
//                  before it's reported.
//- initPin         If true, then the input is set to INPUT_PULLUP or INPUT
//                  if onState is LOW or HIGH respectively.  If false, the
//                  caller must configure the pin.
//
inline
void
DigitalInput::
init( uint8_t pin,
      uint8_t onState,
      ReadMode mode,
      uint8_t debounceMillis,
      bool initPin )
{
   m_pin = pin;
   m_buffer = 0;
   m_info.isPin = 1;
   m_info.isDigital = ( mode == DIGITAL );
   m_info.onState = onState;
   m_info.unstable = 0;
   m_info.stable = 0;
   m_info.changed = 0;
   m_info.longPress = 0;
   m_debounceMillis = debounceMillis;
   
   // HIGH = external pulldown, define as INPUT.
   // LOW = internal pullup, define as INPUT_PULLUP.
   if ( initPin )
   {
      pinMode( m_pin, onState == HIGH ? INPUT : INPUT_PULLUP );
   }

   // Get the initial input value.  Assume the first read is stable.
   m_info.stable = isOnRaw();

   DIGITALINPUT_DBG( "DigitalInput init on pin ", pin );
}

//============================================================================
// Create a input attached to a shift register.
//
//= INPUTS
//- buffer          Pointer to the integer the shift register is read into.
//                  This variable must remain in scope with the input.
//- bitIndex        Bit index inside *m_buffer of the bit to read the
//                  input value for.
//- onState         LOW or HIGH, the state the input is in when it's on.
//- mode            Use digital read or analog read to test the pin.
//- debounceMillis  Number of milliseconds the input must be in the same state
//                  before it's reported.
//- initialState    Initial state of the input (0=off,1=on) to set.
//
inline
void
DigitalInput::
initShift( uint8_t* buffer,
           uint8_t bitIndex,
           uint8_t onState,
           uint8_t debounceMillis,
           bool initialState )
{
   m_pin = bitIndex;
   m_buffer = buffer;
   m_info.isPin = 0;
   m_info.isDigital = 1;
   m_info.onState = onState;
   m_info.unstable = 0;
   m_info.stable = 0;
   m_info.changed = 0;
   m_info.longPress = 0;
   m_debounceMillis = debounceMillis;
   
   // Set the initial input value.  
   m_info.stable = initialState;

   DIGITALINPUT_DBG( "DigitalInput init on buffer bit ", bitIndex );
}

//============================================================================
// Return true if the input is currently activated (and stable).
//
inline
bool
DigitalInput::
isOn()
{
   return m_info.stable;
}

//============================================================================
// Return true if the input has gone from open->closed in the last
// poll() call.
//
inline
bool
DigitalInput::
pressed()
{
   return m_info.stable && m_info.changed;
}

//============================================================================
// Return true if the input has gone from closed->open in the last
// poll() call.
//
inline
bool
DigitalInput::
released()
{
   return ! m_info.stable && m_info.changed;
}

//============================================================================
