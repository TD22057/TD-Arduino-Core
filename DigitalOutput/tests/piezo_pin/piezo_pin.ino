#include "Arduino.h"
#include "DigitalOutput.h"
#include "Timer.h"

// Wiring:
//
// pin 10 (D10) -> 220 ohm resistor -> piezo +
//    piezo - -> GND
DigitalOutput g_buz;

void
setup()
{
   Serial.begin( 19200 );

   g_buz.init( 10 );
}

void
loop()
{
   Serial.println( "ON" );
   g_buz.on();
   delay( 1000 );

   Serial.println( "OFF" );
   g_buz.off();
   delay( 1000 );

   Serial.println( "ON" );
   g_buz.on();
   delay( 1000 );
   
   Serial.println( "OFF" );
   g_buz.off();
   delay( 1000 );
}      

