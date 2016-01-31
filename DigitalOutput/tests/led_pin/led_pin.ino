#include "Arduino.h"
#include "DigitalOutput.h"
#include "Timer.h"

// Wiring:
//
// pin  8 (D8) -> resistor -> led +
//                            led - -> GND
// pin 14 (A0) -> resistor -> led -
//                            led + -> VCC

DigitalOutput g_led1;
DigitalOutput g_led2;

int g_phase = 0;
long g_next = 0;

const long DT = 5000; // 5 sec

void
setup()
{
   Serial.begin( 19200 );

   g_led1.init( 8 );
   g_led2.init( 14, LOW );

   Serial.println( "1 ON    2 OFF" );
   g_led1.on();
   g_led2.off();

   g_next = millis() + DT;
}

void
loop()
{
   if ( ( (long)millis() - g_next ) >= 0 )
   {
      switch( g_phase )
      {
      case 0:
         Serial.println( "1 OFF   2 ON " );
         g_led1.off();
         g_led2.on();
         g_phase = 1;
         break;
      case 1:
         Serial.println( "1 ON    2 OFF" );
         g_led1.toggle();
         g_led2.toggle();
         g_phase = 2;
         break;
      case 2:
         Serial.println( "1 SLOW 2 FAST" );
         g_led1.blinkSlow();
         g_led2.blinkFast();
         g_phase = 3;
         break;
      case 3:
         Serial.println( "1 OFF  2 OFF" );
         g_led1.off();
         g_led2.off();
         g_phase = 4;
         break;
      case 4:
         Serial.println( "1 FAST 2 SLOW" );
         g_led1.blinkFast( 5 );
         g_led2.blinkSlow( 3 );
         g_phase = 5;
         break;
      case 5:
         Serial.println( "1 ON   2 ON" );
         g_led1.on();
         g_led2.on();
         g_phase = 6;
         break;
      case 6:
         break;
      }
      
      g_next = millis() + DT;
   }

   long now = millis();
   g_led1.poll( now );
   g_led2.poll( now );
}      

