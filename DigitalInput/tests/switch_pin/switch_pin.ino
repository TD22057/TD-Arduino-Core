#include "Arduino.h"
#include "DigitalInput.h"

static void switchCb( DigitalInput::Status status, int8_t id );

// DigitalInput 1.  Digital pin, on=LOW, internal pull up resistor.
//    D9 -> switch -> GND
//
// DigitalInput 2:  Analog only pin, on=HIGH, external pull down resistor
//    A6 -> switch -> VCC
//       |- resistor -> GND
DigitalInput g_sw1;
DigitalInput g_sw2;

void
setup()
{
   Serial.begin( 19200 );

   // pull up resistor, on=LOW, digital pin
   g_sw1.init( 9 );

   // Pin A6, external pull down, on=HIGH, analog pin.
   g_sw2.init( 20, HIGH, DigitalInput::ANALOG );

   Serial.println( "Watching switches..." );
}

void
loop()
{
   long now = millis();

   DigitalInput::Status s1 = g_sw1.poll( now );
   if ( s1 != DigitalInput::NONE )
   {
      switch ( s1 )
      {
      case DigitalInput::CLOSED:
         Serial.println( "DigitalInput 1 pressed" );
         break;
      case DigitalInput::OPENED:
         Serial.println( "DigitalInput 1 released" );
         break;
      case DigitalInput::OPENED_LONG:
         Serial.println( "DigitalInput 1 released long" );
         break;
      case DigitalInput::NONE:
         break;
      }
   }

   g_sw2.poll( now, switchCb, 12 );
}      

static void
switchCb( DigitalInput::Status status,
          int8_t id )
{
   switch ( status )
   {
   case DigitalInput::CLOSED:
      Serial.print( "DigitalInput 2 pressed id = " );
      Serial.println( id );
      break;
   case DigitalInput::OPENED:
      Serial.print( "DigitalInput 2 released id = " );
      Serial.println( id );
      break;
   case DigitalInput::OPENED_LONG:
      Serial.print( "DigitalInput 2 released long id =" );
      Serial.println( id );
      break;
   case DigitalInput::NONE:
      break;
   }
}
