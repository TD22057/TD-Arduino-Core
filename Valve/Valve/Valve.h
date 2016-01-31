#pragma once
#include "Arduino.h"
#include "DigitalInput.h"
#include "DigitalOutput.h"

// Articulated valve with sensor wire controller class.
//
// This class is used to control an articulated valve using a motor
// controller (2 pins).  The valve must support two sensor wires (one
// for open, one for closed) which get pulled LOW when the valve is
// fully open or closed (2 pins).
//
// Primary use case if for 5 wire valves from TF Fluid Control Systems
// (http://www.aliexpress.com/store/414681).  5V valves from this
// company open and close in about 5 seconds so use ~10 seconds for the
// powerOnTimeOut.  
//
// The two controller pins (pinOpen, pinClose) passed to init are
// inputs to a motor controller like an H-Bridge.  Possible values for
// the pins are below.  You may need to experiment with the valve to
// determine which pin is which.
//
//  pinOpen / pinClose
//    HIGH  /   HIGH    Valve off
//     LOW  /    LOW    Valve off
//    HIGH  /    LOW    Valve opening
//     LOW  /   HIGH    Valve closing
//
// Two time out values are required by init().  These are safety
// factors which limit the amount of time that power is supplied to
// the valve (cutoff power after n seconds) and how often the valve
// power can be cycled.  The second is more important as the cheaper
// H-Bridge controllers have no thermal cutoff or heat sinks and may
// need time to cool off in between valve actuations.
//
// If a command (open/close) is issued while the valve is already
// moving (status is OPENING or CLOSING), then that command is stored
// until the previous command has finished and the cycle power time
// out has ellapsed.  At that point it will be executed.  Only one
// pending command is stored, multiple calls to open/close will only
// keep the last command.
//
class Valve
{
public:
   // Valve status values.
   enum Status {
      NONE = 0,     // Indicates no change in valve status in poll().
      OPENED = 1,   // Valve has been fully opened.
      OPENING = 2,  // Valve is in the process of opening.
      CLOSED = 3,   // Valve has been fully closed.
      CLOSING = 4,  // Valve is in the process of closing.
      UNKNOWN = 5,  // Valve not fully open or closed during start up.
      STALLED = 6   // Power on time out triggered to stop the valve while
                    // being opened or closed.
   };
   // digitalRead() is used normally to test the opened/closed pins.
   // If a pin doesn't support that (A6, A7 on Pro-Mini's), then set
   // the mode to ANALOG.
   enum ReadMode {
      DIGITAL = 0,
      ANALOG = 1,
   };

   // Regular pin wirings.
   Status init( uint8_t pinOpen, uint8_t pinClose, uint8_t pinIsOpened,
                uint8_t pinIsClosed, long powerOnTimeOut,
                long dutyCycleTimeOut, ReadMode openedMode=DIGITAL,
                ReadMode closedMode=DIGITAL );
   // Custom or shift register wirings.
   Status init( const DigitalOutput& open, const DigitalOutput& close,
                const DigitalInput& isOpened, const DigitalInput& isClosed,
                long powerOnTimeOut, long dutyCycleTimeOut );
   
   // State change callback.  Called by poll() when the status
   // changes.  Input is the object and the new status.
   typedef void (*StateChangeCb)( Status status, int8_t identifier );

   // NOTE: long is better than unsigned long - code can ignore roll
   // overs for duration computations.  For details, see:
   // http://playground.arduino.cc/Code/TimingRollover
   Status poll( long currentMillis, StateChangeCb callback=NULL,
                int8_t identifier=0 );

   Status status();
   void open( bool force=false );
   void close( bool force=false );
   void toggle();

private:
   // Controls to trigger the H-bridge motor controller.  When
   // open/close is HIGH/LOW, the valve should open.  When open/close
   // is LOW/HIGH, the valve should close.
   DigitalOutput m_open;
   DigitalOutput m_close;

   // Opened/closed signals from the valve.  
   DigitalInput m_isOpened;
   DigitalInput m_isClosed;
   
   // Current device status.
   Status m_status;

   // Command to execute next when time out's allow.  Possible
   // values: NONE, OPENING, or CLOSING.
   Status m_pendingState;

   // Duration to leave power applied to the open/close pins.
   // Normally the open/close signal should trigger and shut this off.
   long m_powerOnTimeOut;

   // Duration to wait between cycling the power.  This allows the
   // H-bridge chip to cool off.
   long m_dutyCycleTimeOut;

   // Last time power was turned on (mode==OPENING or CLOSING) or
   // turned off (mode==OPENED or CLOSED).  This plus one of the
   // durations above gives the next time power can be applied.
   long m_lastPowerCycle;

   void powerOn( Status mode );
   void powerOff();
   Status initialState();
   Status processState( long currentMillis, DigitalInput::Status openedState,
                        DigitalInput::Status closedState,
                        StateChangeCb callback, int identifier );
};

//============================================================================
// Return the current valve status.
//
// See the enum docs for details.  Should never return NONE.  If
// UNKNOWN is returned, then the valve was either not opened or closed
// on startup or power was cut off by the time out in the middle of
// cycling.
//
inline
Valve::Status
Valve::
status()
{
   return m_status;
}

//============================================================================
// Open the valve.
//
// This changes the pending command to open to the valve.  The next
// time poll() is called and the time outs are satisfied (i.e. the
// valve has finished moving and enough time has passed), it will open
// the valve.
//
//= INPUTS
//- force    If true, the valve is immediately commanded to open ignoring
//           any timeouts.  This may be useful during setup() if the valve
//           is in an unknown state to trigger an immediate opening.
//
inline
void
Valve::
open( bool force )
{
   // Ignore current state and time outs if and force the valve on.
   // Clear any pending states as well so they don't interfere later.
   if ( force )
   {
      powerOn( OPENING );
      m_pendingState = NONE;
      return;
   }
   // If the valve is already open, do nothing.  Otherwise schedule a
   // open() call the next time that the time outs allow.
   else if ( m_status != OPENED && m_status != OPENING )
   {
      m_pendingState = OPENING;
   }
}

//============================================================================
// Close the valve.
//
// This changes the pending command to close to the valve.  The next
// time poll() is called and the time outs are satisfied (i.e. the
// valve has finished moving and enough time has passed), it will close
// the valve.
//
//= INPUTS
//- force    If true, the valve is immediately commanded to close ignoring
//           any timeouts.  This may be useful during setup() if the valve
//           is in an unknown state to trigger an immediate closing.
//
inline
void
Valve::
close( bool force )
{
   // Ignore current state and time outs if and force the valve on.
   // Clear any pending states as well so they don't interfere later.
   if ( force )
   {
      powerOn( CLOSING );
      m_pendingState = NONE;
      return;
   }

   // If the valve is already closed, do nothing.  Otherwise schedule a
   // open() call the next time that the time outs allow.
   else if ( m_status != CLOSED && m_status != CLOSING )
   {
      m_pendingState = CLOSING;
   }
}

//============================================================================
// Toggle the valve state.
//
// If the status is CLOSED, it will be opened.  Any other status will
// call close().
//
inline
void
Valve::
toggle()
{
   if ( m_status == CLOSED )
   {
      open();
   }
   else
   {
      close();
   }
}

//============================================================================


