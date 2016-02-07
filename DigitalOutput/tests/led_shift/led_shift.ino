#include "Arduino.h"
#include "DigitalOutput.h"
#include "Timer.h"
#include "SPI.h"
#include "DigitalIO.h"

static void updateShift();

// Wiring:
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

DigitalOutput g_led1;
DigitalOutput g_led2;

DigitalPin< 4 > g_shiftOutEnable;
DigitalPin< 7 > g_shiftOutSelect;

uint8_t g_shiftOutBuffer = 0;

int g_phase = 0;
long g_next = 0;

const long DT = 5000; // 5 sec

void
setup()
{
   Serial.begin( 19200 );
   SPI.begin();

   g_shiftOutEnable.mode( OUTPUT );
   g_shiftOutSelect.mode( OUTPUT );
   pinMode( 10, OUTPUT ); // arduino set to spi master

   g_led1.initShift( &g_shiftOutBuffer, 1 ); // output B
   g_led2.initShift( &g_shiftOutBuffer, 7, LOW ); // output H

   g_shiftOutEnable.low(); // enable shift register outputs

   Serial.println( "1 ON    2 OFF" );
   g_led1.on();
   g_led2.off();
   updateShift();

   g_next = millis() + DT;
}

void
loop()
{
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
         break;
      }
      
      g_next = millis() + DT;
   }

   long now = millis();
   g_led1.poll( now );
   g_led2.poll( now );

   updateShift();
}      

static void
updateShift()
{
   g_shiftOutSelect.low();
   SPI.transfer( g_shiftOutBuffer );
   g_shiftOutSelect.high();
}
