/*
  Watchdog Functions

  Creates a custom Watchdog Timer which can be used to trigger a hardware reset upon timeout.
  SetupWatchdog() must be called once at the start of the sketch to prepare the timer, interrrupts and output pins.
  ResetWatchdog() must be caled once per loop, or after any time-consuming steps.
    if ResetWatchdog() is not called before the timeout period elapses, the Arduino will be reset
  HardwareReset() can be called to force the Arduino to reset

  Two definitions are required to use these functions:
  WATCHDOG_TIMEOUT_SECONDS: Sets the timeout in seconds
  PIN_RESET_ARDUINO:        The digital pin which is wired to the Arduino reset pin

  Example:
    #define WATCHDOG_TIMEOUT_SECONDS 2
    #define PIN_RESET_ARDUINO 4

  The circuit:
  * Wire connecting specified digital pin and the reset pin

  Created 15 Jan 2014
  By Aaron Lobo

  Modified From:
  http://www.engblaze.com/microcontroller-tutorial-avr-and-arduino-timer-interrupts/

  Note: 
    In case of a reset loop, the programmer will not be able to communicate with the Arduino.
    Simply disconnect the reset wire to resolve this.
 
 */

#include <Arduino.h>
// avr-libc library includes
#include <avr/io.h>
#include <avr/interrupt.h>


#define PIN_RESET_ARDUINO 4
#define WATCHDOG_TIMEOUT_SECONDS 90
volatile int watchdogSeconds = 0;

void SetupWatchdog(){
  // Initialize Timer1
  cli();          // Disable global interrupts
  TCCR1A = 0;     // Set entire TCCR1A register to 0
  TCCR1B = 0;     // Same for TCCR1B

  // Set compare match register to desired timer count:
  OCR1A = 15624;
  // Turn on CTC mode:
  TCCR1B |= (1 << WGM12);
  // Set CS10 and CS12 bits for 1024 prescaler:
  TCCR1B |= (1 << CS10);
  TCCR1B |= (1 << CS12);
  // Enable timer compare interrupt:
  TIMSK1 |= (1 << OCIE1A);
  sei();          // Enable global interrupts

  //Setup the hardware reset
  digitalWrite(PIN_RESET_ARDUINO, HIGH); // Set it to HIGH immediately on boot
  pinMode(PIN_RESET_ARDUINO, OUTPUT);    // We declare it an output ONLY AFTER it's HIGH
  digitalWrite(PIN_RESET_ARDUINO, HIGH);
}

void HardwareReset(){
  Serial.println("Reset");
  digitalWrite(PIN_RESET_ARDUINO, LOW);
}

void ResetWatchdog(){
  watchdogSeconds = 0;
}

/**
* Watchdog Timer Interrupt
* This is run once every second
*/
ISR(TIMER1_COMPA_vect){
  watchdogSeconds++;
  if (watchdogSeconds >= WATCHDOG_TIMEOUT_SECONDS){
    HardwareReset();
  }
}

