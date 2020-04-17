/*
  Sounds
  
  Provides functions for tone playback
*/
#include <Arduino.h>
#include "MegaMaster.h"
#include "Pitches.h"
#include "Sounds.h"
/**
* Plays the note for the duration specified.
* This will block program execution until the note has finished playing
* note: Note to play (See Pitches.h)
* duration: Duration of the note in milliseconds
*/
void playTone(int note, int duration) {
  for (long i = 0; i < duration * 1000L; i += note * 2) {
    digitalWrite(PIN_SPEAKER, HIGH);
    delayMicroseconds(note);
    digitalWrite(PIN_SPEAKER, LOW);
    delayMicroseconds(note);
  }
}

void playSuccessSound(){
  playTone(NOTE_A5, 200);
  playTone(NOTE_D5, 350);
}

void playFailSound(){
  playTone(NOTE_F5, 200);
  playTone(NOTE_E5, 300);
  playTone(NOTE_B5, 250);
}

void playAlarmSound(){
  playTone(NOTE_B5, 150);
  playTone(NOTE_F5, 150);
}

void playShortBeepSound(){
  playTone(NOTE_A5, 200);
}

void playLongBeepSound(){
  playTone(NOTE_C6, 1000);
}
