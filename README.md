# TD-Arduino-Core

This is a collection of libraries for use with an Arduino.  They're
fully commented and include test cases.  They follow the design
pattern I prefer to use for asynchronous usage.  There are no delays
in any of the classes.

The classes are designed to be initialized with the hardware
connections in the setup() function and support connections via
Arduino pins or shift registers (individual bits in a byte buffer).
In the loop function, call poll() on each class and the class will
notify you of changes either via return value or callback function.

One of the goals is to isolate the connections (pin assignments, shift
registers, etc) from the usage.  So once a class is initialized, all
of the interactions are with state information ("turn on", "blink 5
times", etc) instead of hardware manipulations.  For a fairly
complicated example of this behavior, see the Valve class test case.

The classes include:

- DigitalInput: On/Off inputs (switches) with either HIGH or LOW
active, optional debouncing, and support for digital pins, analog only
pins (A6/A7) and shift registers.

- DigitalOutput: On/Off ouputs (LED's, relays) including blinking.

- MedianFilter: N sample running median filter.

- Sonar: Ultrasonic sensor.

- Timer: Repeating (num or infinite) periodic triggers.

- Valve: 5 wire articulated valve control


