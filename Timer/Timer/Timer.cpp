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
