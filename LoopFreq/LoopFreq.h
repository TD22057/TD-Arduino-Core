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

