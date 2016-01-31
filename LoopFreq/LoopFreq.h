#pragma once
#include "Arduino.h"

// Loop calling frequency debugging tool.
//
// Used to measure the amount of time it takes to call loop().  Will
// record the elapsed time for n calls to poll() and then report the
// timing results to Serial output.
//
//    LoopFreq g_dbgLoop( 5000 ); // report after 5000 calls.
//
//    void loop()
//    {
//       g_dbgLoop.poll();
//       ...
//    }
//
class LoopFreq
{
public:
   LoopFreq( int numCalls, bool repeat=false );
   void poll();

private:
   // Number of calls to make between reports.
   int m_numCalls;

   // True to repeat forever.  False to report once.
   bool m_repeat;

   // Current number of calls made to poll().
   int m_count;

   // Starting time of the reporting cycline in millis.
   long m_start;
};

