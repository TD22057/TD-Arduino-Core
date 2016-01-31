#include "Arduino.h"
#include "Sonar.h"
#include "MedianFilter.h"
#include "DigitalIO.h"

#define ECHO_PIN 3
#define TRIGGER_PIN 5
#define PING_RATE_HZ 10
#define MEDIAN_SAMPLES 5 // for smoothing - set to 0 for none

Sonar< ECHO_PIN, TRIGGER_PIN, MEDIAN_SAMPLES > g_sonar;

void callback( uint16_t dist_cm );

void
setup()
{
   Serial.begin( 19200 );

   g_sonar.init( PING_RATE_HZ );

   Serial.println( "Starting..." );
}

void
loop()
{
   g_sonar.poll( callback );
}      

void
callback( uint16_t dist_cm )
{
   Serial.print( "Distance: " );
   Serial.print( dist_cm );
   Serial.println( " cm" );
}
