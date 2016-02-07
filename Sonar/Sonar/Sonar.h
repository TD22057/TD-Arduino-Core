// Copyright 2016 by Ted Drain
//
// This program is free software: you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation, either version 3 of the License, or (at your option)
// any later version.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of  MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
// more details.
//
// You should have received a copy of the GNU General Public License along with
// this program.  If not, see <http://www.gnu.org/licenses/>.
#pragma once
#include <Arduino.h>
#include <DigitalIO.h>
#include <MedianFilter.h>

// Interrupt based HR-S04 ultrasonic sonar class
//
// This class is designed for the HR-S04 ultrasonic sensor.
// IMPORTANT: It only supports one sensor - it uses interrupts
// internally for receiving the echo signal and can't support more
// than one sensor.
//
// Using interrupts and template based pin numbers allows for very
// fast and reliable operations.  But - the echo pin must support
// interrupts so on an Arduino Uno or Pro Mini, that is D2 or D3.
//
// The 3rd templte parameter is for the number of samples to use in an
// optional median filter to eliminate outlier results.  Set it zero
// for no filtering.  The median filter returns the median of the last
// N values received by the class.
//
//= Example
//
//   static const int ECHO_PIN = 3;
//   static const int TRIGGER_PIN = 5;
//   static const int SONAR_RATE = 10; // 10 Hz pinging
//
//   // 5 sample median filter.
//   Sonar< ECHO_PIN, TRIGGER_PIN, 5 > g_sonar;
//
//   void setup()
//   {
//      Serial.begin( 19200 );
//      g_sonar.init( SONAR_RATE );
//   }
//
//   void callback( uint16_t dist_cm )
//   {
//      Serial.print( "Distance: " );
//      Serial.print( dist_cm );
//      Serial.println( " cm" );
//   }
//
//   void loop()
//   {
//      g_sonar.poll( callback );
//   }
//
typedef void (*SonarChangeCb)( uint16_t distance_cm );

// ECHO_PIN must be interrupt capable (D2 or D3 on pro mini).  
template< uint8_t ECHO_PIN, uint8_t TRIGGER_PIN, uint8_t NUM_SAMPLES=0 >
class Sonar
{
public:
   // NOTE: Can't use constructors (even though we should) because
   // Arduino sketch generally requires these to be global variables
   // so we don't know that the ctor would be called after hardware
   // init.
   void init( uint8_t rate_hz );

   // Returns distance (or 0) if a pint was received.  Will call
   // the callback when it changes.
   uint16_t poll( SonarChangeCb callback=NULL );

   void on( uint16_t rate_hz=0 );
   void off();
   void setRate( uint16_t rate_hz );
   void clear();

private:
   enum { SONAR_MAX_TIME_US = 500 * 58 }; // 5 meters * 58 us/cm 
   
   // Echo and trigger pins for the sonar module.
   DigitalPin< ECHO_PIN > m_echo;
   DigitalPin< TRIGGER_PIN > m_trigger;

   // on/off flag.  If off, then no pings are sent.
   bool m_on;

   // Time to wait in between pings in microseconds.  
   uint32_t m_rate_us;

   // True if a ping was sent and we're waiting for a return.
   bool m_sent;

   // Time in microseconds the last ping was sent.
   uint32_t m_lastSent_us;

   // Distance in cm of the last ping.
   uint16_t m_lastDist_cm;

   // Filter for removing outliers.  Returns the median value of the
   // last NUM_SAMPLES pings.
   MedianFilter< uint16_t, NUM_SAMPLES > m_filter;

   void sendPing();
   static void echoRise();
   static void echoFall();
   static volatile uint32_t s_pingBeg_us;
   static volatile uint32_t s_pingEnd_us;
};


//============================================================================
// Initialize the module.
//
//= INPUTS
//
//- rate_hz    Ping rate in Hz (times/sec).  Set to zero to ping as fast
//             as possible.
//
template< uint8_t ECHO_PIN, uint8_t TRIGGER_PIN, uint8_t NUM_SAMPLES >
inline
void
Sonar< ECHO_PIN, TRIGGER_PIN, NUM_SAMPLES >::
init( uint8_t rate_hz )
{
   m_echo.mode( INPUT );
   m_trigger.mode( OUTPUT );
   m_trigger.low();
   
   s_pingBeg_us = 0;
   s_pingEnd_us = 0;
   m_sent = false;
   m_on = true;
   m_lastSent_us = 0;
   m_lastDist_cm = 0;
   setRate( rate_hz );
}

//============================================================================
// Turn the module on.
//
//= INPUTS
//
//- rate_hz    Ping rate in Hz (times/sec).  Set to zero to ping as fast
//             as possible.
//
template< uint8_t ECHO_PIN, uint8_t TRIGGER_PIN, uint8_t NUM_SAMPLES >
inline
void
Sonar< ECHO_PIN, TRIGGER_PIN, NUM_SAMPLES >::
on( uint16_t rate_hz )
{
   m_on = true;
   if ( rate_hz )
   {
      setRate( rate_hz );
   }
}

//============================================================================
// Turn the module off.
//
// No pings are sent until on() is called.
//
template< uint8_t ECHO_PIN, uint8_t TRIGGER_PIN, uint8_t NUM_SAMPLES >
inline
void
Sonar< ECHO_PIN, TRIGGER_PIN, NUM_SAMPLES >::
off()
{
   m_on = false;
   clear();
}

//============================================================================
// Set the ping rate to use.
//
//= INPUTS
//
//- rate_hz    Ping rate in Hz (times/sec).  Set to zero to ping as fast
//             as possible.
template< uint8_t ECHO_PIN, uint8_t TRIGGER_PIN, uint8_t NUM_SAMPLES >
inline
void
Sonar< ECHO_PIN, TRIGGER_PIN, NUM_SAMPLES >::
setRate( uint16_t rate_hz )
{
   if ( rate_hz == 0 )
   {
      m_rate_us = 1;
   }
   else
   {
      // Convert rate in Hz (1/sec) to time in microseconds.
      m_rate_us = 1000000 / rate_hz;
   }
}

//============================================================================
// Clear previous values from the median filter.
//
template< uint8_t ECHO_PIN, uint8_t TRIGGER_PIN, uint8_t NUM_SAMPLES >
inline
void
Sonar< ECHO_PIN, TRIGGER_PIN, NUM_SAMPLES >::
clear()
{
   m_filter.clear();
}

//============================================================================
// Poll the module.
//
// If enough time has elapsed, a new ping will be sent.  When a
// distance is found, it will be returned.
//
//= INPUTS
//
//- callback   Optional callback to call when a ping is returned.  Only
//             called when the distance changes.
//
template< uint8_t ECHO_PIN, uint8_t TRIGGER_PIN, uint8_t NUM_SAMPLES >
inline
uint16_t
Sonar< ECHO_PIN, TRIGGER_PIN, NUM_SAMPLES >::
poll( SonarChangeCb callback )
{
   // Sonar is off - do nothing.
   if ( ! m_on )
   {
      return 0;
   }
   // If no ping has been sent, see if we should send one.
   else if ( ! m_sent )
   {
      // NOTE: if we get a time out - it can take a long time (200
      // msec) to "reset" the sensor and have the echo line go low
      // again.  Not sure why that is, but it we try to send a ping
      // which echo is high, it will lock up the arduino somehow.
      if ( m_echo.read() == LOW &&
           (int32_t)( micros() - m_lastSent_us ) > m_rate_us )
      {
         sendPing();
      }

      return 0;
   }
   // Time out - reset our flags so we can send another ping.
   else if ( (int32_t)(micros() - m_lastSent_us ) > SONAR_MAX_TIME_US )
   {
      m_sent = false;
      s_pingBeg_us = s_pingEnd_us = 0;
      return 0;
   }   
   // Ping was sent, but we haven't seen the return pulse yet.
   else if ( s_pingEnd_us == 0 )
   {
      return 0;
   }

   // We have a ping response.  Clear the sent flag so we know we can
   // send another one the next time through.
   m_sent = false;

   // If the interrupts fire too fast (if something covers the
   // sensor), things can get weird and we'll get a negative time or a
   // zero value for the beg time.
   uint32_t dt_us = s_pingEnd_us - s_pingBeg_us;
   if ( s_pingBeg_us == 0 || dt_us > SONAR_MAX_TIME_US )
   {
      return 0;
   }

   // Convert from usec to cm (value from datasheet)
   uint32_t dt_cm = dt_us / 58;

   // If requested, run a median filter on the result to eliminate
   // outliers.
   if ( NUM_SAMPLES )
   {
      m_filter.add( dt_cm );
      dt_cm = m_filter.median();
   }

   // Run the callback if the distance changed.
   if ( callback && dt_cm != m_lastDist_cm )
   {
      callback( dt_cm );
   }
   
   m_lastDist_cm = dt_cm;
   return dt_cm;
}

//============================================================================
// Send a ping.
//
template< uint8_t ECHO_PIN, uint8_t TRIGGER_PIN, uint8_t NUM_SAMPLES >
void
Sonar< ECHO_PIN, TRIGGER_PIN, NUM_SAMPLES >::
sendPing()
{
   s_pingBeg_us = 0;
   s_pingEnd_us = 0;
   m_sent = true;

   // Monitor the echo ping for a rising signal.
   attachInterrupt( digitalPinToInterrupt( ECHO_PIN ), echoRise, RISING );

   // Pulse the trigger pin for 10us to send the ping.
   m_trigger.high();
   delayMicroseconds( 10 );
   m_trigger.low();

   m_lastSent_us = micros();
}

//============================================================================
// Ping return rising interrupt.
//
// This is called when the echo pin rises which is the start of the
// timing routine to get the distance.
//
template< uint8_t ECHO_PIN, uint8_t TRIGGER_PIN, uint8_t NUM_SAMPLES >
void
Sonar< ECHO_PIN, TRIGGER_PIN, NUM_SAMPLES >::
echoRise()
{
   s_pingBeg_us = micros();
   attachInterrupt( digitalPinToInterrupt( ECHO_PIN ), echoFall, FALLING );
}

//============================================================================
// Ping return falling interrupt.
//
// This is called when the echo pin falls which is the end of the
// timing routine to get the distance.
//
template< uint8_t ECHO_PIN, uint8_t TRIGGER_PIN, uint8_t NUM_SAMPLES >
void
Sonar< ECHO_PIN, TRIGGER_PIN, NUM_SAMPLES >::
echoFall()
{
   s_pingEnd_us = micros();
   detachInterrupt( digitalPinToInterrupt( ECHO_PIN ) );
}

//============================================================================

// Static class variable declarations.
template< uint8_t ECHO_PIN, uint8_t TRIGGER_PIN, uint8_t NUM_SAMPLES >
volatile uint32_t
Sonar< ECHO_PIN, TRIGGER_PIN, NUM_SAMPLES >::s_pingBeg_us;

template< uint8_t ECHO_PIN, uint8_t TRIGGER_PIN, uint8_t NUM_SAMPLES >
volatile uint32_t
Sonar< ECHO_PIN, TRIGGER_PIN, NUM_SAMPLES >::s_pingEnd_us;
//============================================================================
