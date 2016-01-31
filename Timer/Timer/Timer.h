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
#include "Arduino.h"

// Elapsed time trigger class.
//
// Used to trigger events (possibly repeating) after a certain amount
// of time has elapsed.  Call once() or repeat() to start the timer.
// Then call poll() each time in the loop() function to see if the
// timer should fire.  poll() returns true if enough time has passed
// and can call an arbitrary function when that occurs.
//
class Timer
{
public:
   Timer( int8_t identifier=0 );

   void once( long timeMillis );
   void repeat( long timeMillis, int8_t count=-1 );
   void off();

   int8_t remaining();

   // Timer execute callback.  Called by poll() when the timer fires.
   // Input is the identifier from the constructor.
   typedef void (*TimerCb)( int8_t identifier );
   
   // NOTE: long is better than unsigned long - code can ignore roll
   // overs for duration computations.  For details, see:
   // http://playground.arduino.cc/Code/TimingRollover
   int8_t poll( long currentMillis, TimerCb callback=NULL );

private:
   // Arbitrary identifier passed to the callback function.  
   int8_t m_identifier;

   // Number of times the timer should fire.  When it hits zero, the
   // timer turns off.  If m_count is -1, then the timer repeats
   // infinitely.
   int8_t m_count;

   // Duration between timer firings in millis.
   long m_duration;

   // Time in millis of the next timer firing.
   long m_nextTime;
};

//============================================================================
// Constructor
//
//= INPUTS
//
//- identifier   Optional identifier that will be passed to the state change
//               callback in poll.  Used so that one callback can handle
//               many timers.
//
inline
Timer::
Timer( int8_t identifier )
   : m_identifier( identifier )
{
}

//============================================================================
// Multiple shot timer.
//
// poll() will return count times after the input time has elapsed.
//
//= INPUTS
//
//- timeMillis   Duration from now at which to signal the timer.
//- count        Number of times to signal.  <0 for infinite.
//
inline
void
Timer::
repeat( long timeMillis,
        int8_t count )
{
   // Constrain the count to -1 for infinite timers.
   m_count = count < 0 ? -1 : count;
   m_duration = timeMillis;
   m_nextTime = millis() + m_duration;
}   


//============================================================================
// Single shot timer.
//
// poll() will return once after the input time has elapsed.
//
//= INPUTS
//
//- timeMillis   Duration from now at which to signal the timer.
//
inline
void
Timer::
once( long timeMillis )
{
   repeat( timeMillis, 1 );
}   

//============================================================================
// Turn the timer off.
//
inline
void
Timer::
off()
{
   m_count = 0;
}

//============================================================================
// Number of times the timer will fire before stopping.
//
// Returns 0 if the timer is off.  Returns -1 if the timer is repeats
// infinitely.
//
inline
int8_t
Timer::
remaining()
{
   return m_count;
}
                    
//============================================================================
