#include "LoopFreq.h"

//============================================================================
// Constructor
//
//= INPUTS
//- numCalls   Number of calls to poll() before reporting results.
//- repeat     If false, make one report and stop.  If true, keep reporting
//             every numCalls.
//
LoopFreq::
LoopFreq( int numCalls,
          bool repeat )
   : m_numCalls( numCalls ),
     m_repeat( repeat ),
     m_count( 0 ),
     m_start( 0 )
{
}

//============================================================================
// Poll the counter
//
// When called numCalls times, it will report the elapsed time and the
// average time per call to the serial print out.
//
void
LoopFreq::
poll()
{
   if ( m_count == -1 )
   {
      return;
   }
   else if ( m_count == 0 )
   {
      m_start = millis();
   }

   if ( ++m_count == m_numCalls )
   {
      long elapsed = millis() - m_start;
      float avg = (float)elapsed / m_numCalls;
      m_count = m_repeat ? 0 : -1;
      
      Serial.print( "LoopFreq " );
      Serial.print( m_numCalls );
      Serial.println( " calls" );
      Serial.print( "   Time: " );
      Serial.print( elapsed );
      Serial.println( " ms" );
      Serial.print( "   Avg : " );
      Serial.print( avg );
      Serial.println( " ms/call" );
   }
}
      
//============================================================================
