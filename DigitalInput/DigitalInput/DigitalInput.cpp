#include "DigitalInput.h"

//============================================================================
// Poll the switch.
//
// This should be called in each loop().  
//
//= INPUTS
//- currentMillis   The current elapsed time in milliseconds.
//- callback        Optional callback function.  Will be called if the
//                  status changes.
//- identifier      Optional, arbitrary identifer to pass to the callback.
//                  Useful for allowing one callback to handle multiple
//                  switches by changing the identifier.
//
//= RETURNS
//- Returns NONE if nothing has changed, otherwise the return value
//  indicates if the switch was pressed or released in the last interval.
//
DigitalInput::Status
DigitalInput::
poll( long currentMillis,
      StateChangeCb callback,
      int8_t identifier )
{
   Status result = NONE;

   // Clear the changed flag and read the current switch state.
   m_info.changed = 0;

   // Read the switch from the pin or the shift register.
   bool isOn = isOnRaw();
   
   // If the current switch state is different than the last switch
   // state, restart the debouncing counter.
   if ( isOn != m_info.unstable )
   {
      // When the switch is released, record if it's a long press or a
      // short press.
      if ( ! isOn )
      {
         bool isLong = ( ( currentMillis - m_stopMillis ) >
                         DIGITALINPUT_LONG_CLOSE_TIME );
         m_info.longPress = isLong;
      }

      // Update the next time at which we could have a stable value
      // and store the switch state as the unstable value.
      m_stopMillis = currentMillis + m_debounceMillis;
      m_info.unstable = isOn;
   }

   // Current switch state is the same as the previous call, see if
   // enough time has passed in this state to trigger the switch.  See
   // m_stopMillis docs for roll-over comments.
   else if ( ( currentMillis - m_stopMillis ) >= 0 )
   {
      // Only trigger a result if the current state is different than
      // the last stable state.
      if ( isOn != m_info.stable )
      {
         // Set the return values correctly.
         if ( isOn )
         {
            result = CLOSED;
         }
         // Long press flag was set above on the first call to poll()
         // after the switch is released.
         else if ( m_info.longPress )
         {
            result = OPENED_LONG;
         }
         else
         {
            result = OPENED;
         }

         // Save the switch state in the stable field and record that
         // a change occurred.  The changed flag is used in the
         // pressed() and released() methods.
         m_info.stable = isOn;
         m_info.changed = 1;
      }
   }

   // Run the state change callback if supplied.
   if ( result != NONE && callback )
   {
      DIGITALINPUT_DBG( "DigitalInput status change ", result );
      callback( result, identifier );
   }

   return result;
}

//============================================================================



