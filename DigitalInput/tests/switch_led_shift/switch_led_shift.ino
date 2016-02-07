#include "Arduino.h"
#include "DigitalInput.h"
#include "DigitalOutput.h"
#include "Timer.h"
#include "SPI.h"
#include "DigitalIO.h"

static void updateShift();
static void switchCb( DigitalInput::Status status, int8_t id );

// NOTE: D13 (SCK), D12 (MISO), D11 (MOSI)
//
//   74HC589       Arduino Pro Mini
// GND   (8)    -> GND
// QH    (9)    -> MISO (D12)
// OE    (10)   -> chip select (D7)
// SCLK  (11)   -> shift clock SCK (D13)
// LCLK  (12)   -> latch clock (D8)
// SLOAD (13)   -> serial shift/parallel load (D8)
// SA    (14)   -> GND
// VCC   (16)   -> VCC
//
// DigitalInput 1.  on=LOW, external pull up resistor.
//    1 -> switch -> GND (input B)
//       |- resistor -> VCC
//
// DigitalInput 2:  on=HIGH, external pull down resistor
//    7 -> switch -> VCC (input H)
//       |- resistor -> GND
//
// 74HC595 Pins     Arduino Pins
//   GND   (8)  ->  GND
//   SRCLR (10) ->  VCC (clear reigster if low)
//   SRCLK (11) ->  D13 - SCK clockPin (shift register clock)
//   RCLK  (12) ->  D7 - chip select (CS) pin (storage register clock)
//   OE    (13) ->  D4 with pull up resistor -  output enabled:
//                     high=outputs high-z, low=outputs enabled
//   SER   (14) ->  D11 MOSI (serial output)
//   VCC   (16) ->  VCC
//
// pin 1 (B) -> resistor -> led +
//                          led - -> GND
// pin 7 (H) -> resistor -> led -
//                          led + -> VCC
//
DigitalInput g_sw1;
DigitalInput g_sw2;

DigitalOutput g_led1;
DigitalOutput g_led2;

DigitalPin< 4 > g_shiftOutEnable;
DigitalPin< 8 > g_shiftInLoad;
DigitalPin< 7 > g_select;

uint8_t g_shiftInBuffer = 0;
uint8_t g_shiftOutBuffer = 0;

// led outputs
int g_phase = 0;
long g_next = 0;
const long DT = 5000; // 5 sec

void
setup()
{
   Serial.begin( 19200 );

   g_shiftOutEnable.mode( OUTPUT );
   g_shiftInLoad.mode( OUTPUT );
   g_select.mode( OUTPUT );
   pinMode( 10, OUTPUT ); // arduino is spi master
   pinMode( 11, OUTPUT ); // MOSI
   pinMode( 12, INPUT ); // MISO
   pinMode( 13, OUTPUT ); // SCK

   g_select.high();
   g_shiftInLoad.low();

   SPI.begin();
   
   // pull up, on=LOW
   g_sw1.initShift( &g_shiftInBuffer, 1, LOW ); // bit 1, input B

   // pull down, on=HIGH, 
   g_sw2.initShift( &g_shiftInBuffer, 7, HIGH ); // bit 7, input H

   g_led1.initShift( &g_shiftOutBuffer, 1 ); // output B
   g_led2.initShift( &g_shiftOutBuffer, 7, LOW ); // output H
   
   Serial.println( "Watching switches..." );

   Serial.println( "1 ON    2 OFF" );
   g_led1.on();
   g_led2.off();
   updateShift();

   g_next = millis() + DT;
}

void
loop()
{
   updateShift();

   if ( ( (long)millis() - g_next ) >= 0 )
   {
      switch( g_phase )
      {
      case 0:
         Serial.println( "1 OFF   2 ON " );
         g_led1.off();
         g_led2.on();
         g_phase = 1;
         break;
      case 1:
         Serial.println( "1 ON    2 OFF" );
         g_led1.toggle();
         g_led2.toggle();
         g_phase = 2;
         break;
      case 2:
         Serial.println( "1 SLOW 2 FAST" );
         g_led1.blinkSlow();
         g_led2.blinkFast();
         g_phase = 3;
         break;
      case 3:
         Serial.println( "1 OFF  2 OFF" );
         g_led1.off();
         g_led2.off();
         g_phase = 4;
         break;
      case 4:
         Serial.println( "1 FAST 2 SLOW" );
         g_led1.blinkFast( 5 );
         g_led2.blinkSlow( 3 );
         g_phase = 5;
         break;
      case 5:
         Serial.println( "1 ON   2 ON" );
         g_led1.on();
         g_led2.on();
         g_phase = 6;
         break;
      case 6:
         Serial.println( "Finished" );
         g_phase = 7;
         break;
      }
      
      g_next = millis() + DT;
   }

   long now = millis();
   g_led1.poll( now );
   g_led2.poll( now );
   
   DigitalInput::Status s1 = g_sw1.poll( now );
   if ( s1 != DigitalInput::NONE )
   {
      switch ( s1 )
      {
      case DigitalInput::CLOSED:
         Serial.println( "DigitalInput 1 pressed" );
         break;
      case DigitalInput::OPENED:
         Serial.println( "DigitalInput 1 released" );
         break;
      case DigitalInput::OPENED_LONG:
         Serial.println( "DigitalInput 1 released long" );
         break;
      case DigitalInput::NONE:
         break;
      }
   }

   g_sw2.poll( now, switchCb, 12 );
}      

static void
updateShift()
{
   g_shiftInLoad.high();
   g_select.low();
   g_shiftInBuffer = SPI.transfer( g_shiftOutBuffer );
   g_select.high();
   g_shiftInLoad.low();
}
   

static void
switchCb( DigitalInput::Status status,
          int8_t id )
{
   switch ( status )
   {
   case DigitalInput::CLOSED:
      Serial.print( "DigitalInput 2 pressed id = " );
      Serial.println( id );
      break;
   case DigitalInput::OPENED:
      Serial.print( "DigitalInput 2 released id = " );
      Serial.println( id );
      break;
   case DigitalInput::OPENED_LONG:
      Serial.print( "DigitalInput 2 released long id =" );
      Serial.println( id );
      break;
   case DigitalInput::NONE:
      break;
   }
}
