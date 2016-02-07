#include "Arduino.h"
#include "DigitalInput.h"
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
DigitalInput g_sw1;
DigitalInput g_sw2;

DigitalPin<8> g_load;
DigitalPin<7> g_select;
uint8_t g_shiftInBuffer = 0;

void
setup()
{
   Serial.begin( 19200 );

   g_load.mode( OUTPUT );
   g_select.mode( OUTPUT );
   pinMode( 10, OUTPUT ); // arduino is spi master
   pinMode( 12, INPUT ); // MISO
   pinMode( 13, OUTPUT ); // SCK

   g_select.high();
   g_load.low();

   SPI.begin();
   
   // pull up, on=LOW
   g_sw1.initShift( &g_shiftInBuffer, 1, LOW ); // bit 1, input B

   // pull down, on=HIGH, 
   g_sw2.initShift( &g_shiftInBuffer, 7, HIGH ); // bit 7, input H

   Serial.println( "Watching switches..." );
}

void
loop()
{
   static uint8_t last = 0;
   updateShift();
   if ( g_shiftInBuffer != last )
   {
      static int num = 0;
      last = g_shiftInBuffer;
      
      Serial.print( ++num );
      Serial.print( " Read: " );
      for ( uint8_t i = 0; i < 8; i++ )
      {
         Serial.print( bitRead( last, i ) );
         Serial.print( " " );
      }
      Serial.print( "\n" );
   }

   long now = millis();

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
   g_load.high();
   g_select.low();
   g_shiftInBuffer = SPI.transfer( 0x0 );
   g_select.high();
   g_load.low();
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
