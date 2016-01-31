#include "Arduino.h"
#include "Timer.h"

Timer g_t1( 1 );
Timer g_t2( 2 );
int g_stage = 0;

static void timerCb( int8_t identifier );

void
setup()
{
   Serial.begin( 19200 );

   g_t1.repeat( 3000 ); // 3 seconds
}

void
loop()
{
   long now = millis();

   if ( g_t1.poll( now ) )
   {
      Serial.println( "T1: poll triggered" );
      
      g_stage++;
      if ( g_stage == 1 )
      {
         Serial.println( "Firing T2 once in 1 second" );
         g_t2.once( 1000 );
      }
      else if ( g_stage == 2 )
      {
         Serial.println( "Firing T2 repeat 3 times, every 0.5 second" );
         g_t2.repeat( 500, 3 );
      }
      else if ( g_stage == 3 )
      {
         Serial.println( "Firing T2 repeat, every 0.5 second" );
         g_t2.repeat( 500 );
      }
      else if ( g_stage == 4 )
      {
         Serial.println( "Firing T2, should stop repeat" );
         g_t2.once( 500 );
      }
      else if ( g_stage == 5 )
      {
         Serial.println( "Firing T2 repeat, every 0.5 second" );
         g_t2.repeat( 500 );
      }
      else if ( g_stage == 6 )
      {
         Serial.println( "Stopping T2 repeat" );
         g_t2.off();
      }
   }

   g_t2.poll( now, timerCb );
}

static void
timerCb( int8_t identifier )
{
   Serial.print( "T2: callback id = " );
   Serial.println( identifier );
}
