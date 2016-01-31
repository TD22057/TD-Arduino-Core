#include "Timer.h"

//============================================================================
// Poll the timer.
//
// This should be called in each loop().  
//
//= INPUTS
//- currentMillis   The current elapsed time in milliseconds.
//- callback        Optional callback function.  Will be called if the
//                  status changes.
//
//= RETURNS
//- Returns 0 if nothing has changed, otherwise the number of remaining
//  triggers (including this one) is returned.  Will be -1 if an
//  infinite number is remaining.
//
int8_t
Timer::
poll( long currentMillis,
      TimerCb callback )
{
   // If there are remaining firings and enough time has passed, fire
   // the timer.
   if ( m_count != 0 && ( currentMillis - m_nextTime ) >= 0 )
   {
      m_nextTime = millis() + m_duration;
      if ( callback )
      {
         callback( m_identifier );
      }

      // Only reduce the count if it's not infinity (-1).
      if ( m_count > 0 )
      {
         // Return count and then decrement.  Otherwise we'd return 0
         // on the last firing which would indicate nothing happened.
         return m_count--;
      }
      // Infinite timer.
      else
      {
         return -1;
      }
   }

   return 0;
}

//============================================================================
