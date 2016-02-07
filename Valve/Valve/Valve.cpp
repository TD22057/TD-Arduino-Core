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
#include "Valve.h"

//============================================================================
// Initialize the valve using pins.
//
//= INPUTS
//- pinOpen           The digital pin attached to the motor controller
//                    to open the valve.  When this is HIGH and pinClose
//                    is LOW, the valve should open.
//- pinClose          The digital pin attached to the motor controller
//                    to close the valve.  When this is HIGH and pinOpen
//                    is LOW, the valve should close.
//- pinIsOpened       The digital pin connected to the valve which
//                    indicates the valve is fully opened when this goes LOW.
//- pinIsClosed       The digital pin connected to the valve which
//                    indicates the valve is fully closed when this goes LOW.
//- powerOnTimeOut    Time out in milliseconds after which to cut power to
//                    the valve.  This should be longer than the nominal
//                    valve open close time.
//- dutyCycleTimeOut  Valve power cycle time out in milliseconds.  Commands
//                    will be never be issued closer than this time together
//                    in order to allow the motor controller to cool.
//
//= RETURN VALUE
//
//- Returns the current valve status.  Possible values are UNKNOWN,
//  OPENED, CLOSED).  If the valve is partially open, UNKNOWN is
//  returned.
//
Valve::Status
Valve::
init( uint8_t pinOpen,
      uint8_t pinClose,
      uint8_t pinIsOpened,
      uint8_t pinIsClosed,
      long powerOnTimeOut,
      long dutyCycleTimeOut,
      ReadMode openedMode,
      ReadMode closedMode )
{
   // Initialize the open/close control pins.
   m_open.init( pinOpen );
   m_close.init( pinClose );
   
   // DigitalInputes are connected to ground when that status is active.
   m_isOpened.init( pinIsOpened, LOW,
                    openedMode == DIGITAL ? DigitalInput::DIGITAL :
                                            DigitalInput::ANALOG );
   m_isClosed.init( pinIsClosed, LOW,
                    closedMode == DIGITAL ? DigitalInput::DIGITAL :
                                            DigitalInput::ANALOG );

   m_powerOnTimeOut = powerOnTimeOut;
   m_dutyCycleTimeOut = dutyCycleTimeOut;

   return initialState();
}

//============================================================================
// Initialize the valve using shift registers.
//
// This init routine can be used for more control such as wiring with
// shift registers or a mix of pins and shift registers.  
// 
//= INPUTS
//- open              The digital output object to trigger to open the valve.
//                    When open is on and close is off, the valve should open.
//- clsoe             The digital output object to trigger to close the valve.
//                    When close is on and open is off, the valve should close.
//- isOpened          The digital input connected to the valve which signals
//                    when the valve is fully opened.
//- isClosed          The digital input connected to the valve which signals
//                    when the valve is fully closed.
//- powerOnTimeOut    Time out in milliseconds after which to cut power to
//                    the valve.  This should be longer than the nominal
//                    valve open close time.
//- dutyCycleTimeOut  Valve power cycle time out in milliseconds.  Commands
//                    will be never be issued closer than this time together
//                    in order to allow the motor controller to cool.
//
//= RETURN VALUE
//
//- Returns the current valve status.  Possible values are UNKNOWN,
//  OPENED, CLOSED).  If the valve is partially open, UNKNOWN is
//  returned.
//
Valve::Status
Valve::
init( const DigitalOutput& open,
      const DigitalOutput& close,
      const DigitalInput& isOpened,
      const DigitalInput& isClosed,
      long powerOnTimeOut,
      long dutyCycleTimeOut )
{
   // Initialize the open/close controls.
   m_open = open;
   m_close = close;

   // Initialize the sensors.
   m_isOpened = isOpened;
   m_isClosed = isClosed;
   
   m_powerOnTimeOut = powerOnTimeOut;
   m_dutyCycleTimeOut = dutyCycleTimeOut;

   return initialState();
}

//============================================================================
// Configure the initial valve state.
//
//= RETURN VALUE
//
//- Returns the current valve status.  Possible values are UNKNOWN,
//  OPENED, CLOSED).  If the valve is partially open, UNKNOWN is
//  returned.
//
Valve::Status
Valve::
initialState()
{
   // Initial state is unknown.  If neither switch is latched, then
   // the valve is in an intermediate state.  Up to the caller to
   // decide what to do about that.
   m_status = UNKNOWN;

   // Make sure power is off to the valve.
   powerOff();

   // Read the open/close pins directly to get the status.  We don't
   // want to wait for the debounce interval here since this is the
   // initial value and the valve isn't moving.
   if ( m_isOpened.isOnRaw() )
   {
      m_status = OPENED;
   }
   else if ( m_isClosed.isOnRaw() )
   {
      m_status = CLOSED;
   }

   // Reset the last cycle time far enough in the past so that the
   // valve can be powered right away on startup.
   m_lastPowerCycle = millis() - m_dutyCycleTimeOut;

   return m_status;
}


//============================================================================
// Poll the valve.
//
// This should be called in each loop().  
//
//= INPUTS
//- currentMillis   The current elapsed time in milliseconds.
//- callback        Optional callback function.  Will be called if the
//                  status changes.
//- identifier      Optional, arbitrary identifer to pass to the callback.
//                  Useful for allowing one callback to handle multiple
//                  valves by changing the identifier.
//
//= RETURNS
//- Returns NONE if nothing has changed, otherwise the return value
//  indicates if the valve status has changed in the last interval.
//  See the status enum docs for possible values.
//
Valve::Status
Valve::
poll( long currentMillis,
      StateChangeCb callback,
      int8_t identifier )
{
   // Poll the status switches.  Do this first so they get basically
   // the same time.
   DigitalInput::Status openedState = m_isOpened.poll( currentMillis );
   DigitalInput::Status closedState = m_isClosed.poll( currentMillis );

   Status prevState = m_status;

   // Handle status change on the open switch.
   switch ( openedState )
   {
   // Open status received from the valve.
   case DigitalInput::CLOSED:
      // Set the open status and power off the valve.
      m_status = OPENED;
      powerOff();
      break;
      
   // Open status no longer active - valve must be closing.  
   case DigitalInput::OPENED:
   case DigitalInput::OPENED_LONG:
      m_status = CLOSING;
      break;

   default:
      break;
   }

   // Handle status change on the close switch.
   switch ( closedState )
   {
   // Close status received from the valve.
   case DigitalInput::CLOSED:
      // Set the close status and power off the valve.
      m_status = CLOSED;
      powerOff();
      break;
      
   // Close status no longer active - valve must be opening.  
   case DigitalInput::OPENED:
   case DigitalInput::OPENED_LONG:
      m_status = OPENING;
      break;

   default:
      break;
   }

   // Check ongoing status vs timeouts to see if we should cut power
   // or issue any pending commands.
   switch( m_status )
   {
   case OPENING:
   case CLOSING:
      // If we are opening/closing the valve, turn off the power after
      // the time out has ellapsed.  This should only happen if the
      // valve stops for some reason (or the time out is too short).
      if ( currentMillis - ( m_lastPowerCycle + m_powerOnTimeOut ) >= 0 )
      {
         m_status = STALLED;
         powerOff();
      }
      break;
      
   case OPENED:
   case CLOSED:
      // We have a pending command.  See if enough time has ellapsed
      // to execute the pending command.
      if ( m_pendingState != NONE &&
           ( currentMillis - ( m_lastPowerCycle + m_dutyCycleTimeOut ) >= 0 ) )
      {
         // Command the valve go to the opening or closing state.  If
         // the valve is already in this state, this is a null op.
         powerOn( m_pendingState );
         m_pendingState = NONE;
      }
      break;

   default:
      break;
   }

   // Return the new status if it's changed.
   if ( m_status != prevState )
   {
      // Run the state change callback if supplied.
      if ( callback )
      {
         callback( m_status, identifier );
      }
      return m_status;
   }

   return NONE;
}

//============================================================================
// Power off the valve.
//
void
Valve::
powerOff()
{
   // Set both control pins to HIGH to power off the h-bridge.
   m_open.on();
   m_close.on();

   m_lastPowerCycle = millis();
}

//============================================================================
// Power on the valve
//
// Also changes m_status to the input mode.  If the valve is already
// in the input state, the function does nothing and returns.
//
//= INPUTS
//- mode    Must be either OPENING or CLOSING to set the valve into that
//          state.  Other modes are ignored.
//
void
Valve::
powerOn( Status mode )
{
   switch( mode )
   {
   case OPENING:
      // Ignore commands to go to the same state.
      if ( m_status == OPENED )
      {
         return;
      }
      m_open.on();
      m_close.off();
      break;
      
   case CLOSING:
      // Ignore commands to go to the same state.
      if ( m_status == CLOSED )
      {
         return;
      }
      m_open.off();
      m_close.on();
      break;
      
   default:  // should never get here.
      return;
   }

   m_status = mode;
   m_lastPowerCycle = millis();
}

//============================================================================
