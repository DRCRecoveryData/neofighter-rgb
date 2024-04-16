#include "MIDIUSB.h"
#include <Adafruit_NeoPixel.h>

#define NEO_PIN 7
#define NUM_PXL 32
#define debounceDelay 5

// Color palette from the first code
const byte _R[128] = {0, 61, 125, 190, 255, 255, 61, 125, 190, 255, 255, 61, 125, 190, 255, 255, 61, 125, 190, 255, 255, 61, 125, 190, 255, 255, 45, 93, 142, 223, 190, 28, 61, 93, 190, 125, 12, 28, 45, 158, 61, 0, 0, 0, 125, 0, 0, 0, 0, 125, 0, 0, 0, 0, 125, 0, 0, 0, 0, 125, 0, 0, 0, 0, 125, 0, 0, 0, 0, 125, 0, 0, 0, 0, 125, 0, 0, 0, 0, 125, 0, 0, 0, 0, 125, 0, 12, 28, 45, 158, 61, 28, 61, 93, 190, 125, 45, 93, 142, 223, 190, 61, 125, 190, 255, 255, 61, 125, 190, 255, 255, 61, 125, 190, 255, 255, 61, 125, 190, 255, 255, 36, 73, 109, 146, 182, 219, 255};
const byte _G[128] = {0, 0, 0, 0, 125, 0, 12, 28, 45, 158, 61, 28, 61, 93, 190, 125, 45, 93, 142, 223, 190, 61, 125, 190, 255, 255, 61, 125, 190, 255, 255, 61, 125, 190, 255, 255, 61, 125, 190, 255, 255, 61, 125, 190, 255, 255, 61, 125, 190, 255, 255, 61, 125, 190, 255, 255, 61, 125, 190, 255, 255, 61, 125, 190, 255, 255, 45, 93, 142, 223, 190, 28, 61, 93, 190, 125, 12, 28, 45, 158, 61, 0, 0, 0, 125, 0, 0, 0, 0, 125, 0, 0, 0, 0, 125, 0, 0, 0, 0, 125, 0, 0, 0, 0, 125, 0, 0, 0, 0, 125, 0, 0, 0, 0, 125, 0, 0, 0, 0, 125, 0, 36, 73, 109, 146, 182, 219, 255};
const byte _B[128] = {0, 0, 0, 0, 125, 0, 0, 0, 0, 125, 0, 0, 0, 0, 125, 0, 0, 0, 0, 125, 0, 0, 0, 0, 125, 0, 0, 0, 0, 125, 0, 0, 0, 0, 125, 0, 0, 0, 0, 125, 0, 0, 0, 0, 125, 0, 12, 28, 45, 158, 61, 28, 61, 93, 190, 125, 45, 93, 142, 223, 190, 61, 125, 190, 255, 255, 61, 125, 190, 255, 255, 61, 125, 190, 255, 255, 61, 125, 190, 255, 255, 61, 125, 190, 255, 255, 61, 125, 190, 255, 255, 61, 125, 190, 255, 255, 61, 125, 190, 255, 255, 61, 125, 190, 255, 255, 45, 93, 142, 223, 190, 28, 61, 93, 190, 125, 12, 28, 45, 158, 61, 36, 73, 109, 146, 182, 219, 255};

// Keep button states so we're only sending MIDI signals when there's a change
int previousButtons = 0;

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUM_PXL, NEO_PIN, NEO_GRB + NEO_KHZ800);

void noteOn(byte channel, byte pitch, byte velocity) {
  midiEventPacket_t noteOn = {0x09, 0x90 | channel, pitch, velocity};
  MidiUSB.sendMIDI(noteOn);
}

void noteOff(byte channel, byte pitch, byte velocity) {
  midiEventPacket_t noteOff = {0x08, 0x80 | channel, pitch, velocity};
  MidiUSB.sendMIDI(noteOff);
}

void setPixels(int pinStates){
  for(int i=0; i<16; i++){
    if(bitRead(pinStates, i)){ // Button down
      byte velocity = 127; // Maximum velocity when button is pressed
      noteOn(0, (36+i), velocity); // Send MIDI note on
      pixels.setPixelColor((i*2), pixels.Color(_R[velocity], _G[velocity], _B[velocity])); // Set LED color based on velocity
      pixels.setPixelColor((i*2)+1, pixels.Color(_R[velocity], _G[velocity], _B[velocity]));
    } else { // Button up
      byte velocity = 0; // Velocity 0 when button is released
      noteOff(0, (36+i), velocity); // Send MIDI note off
      pixels.setPixelColor((i*2), pixels.Color(_R[velocity], _G[velocity], _B[velocity])); // Set LED color based on velocity
      pixels.setPixelColor((i*2)+1, pixels.Color(_R[velocity], _G[velocity], _B[velocity]));
    }
  }
  pixels.show();
}

void setup() {

  // Set pins for input
  DDRB  &= ~(1<<7 | 1<<6 | 1<< 5 | 1<<4);
  PORTB |=  (1<<7 | 1<<6 | 1<< 5 | 1<<4);
  DDRC  &= ~(1<<7 | 1<<6);
  PORTC |=  (1<<7 | 1<<6);
  DDRD  &= ~(1<<7 | 1<<6 | 1<< 5 | 1<<4 | 1<<1 | 1<<0);
  PORTD |=  (1<<7 | 1<<6 | 1<< 5 | 1<<4 | 1<<1 | 1<<0);
  DDRF  &= ~(1<<7 | 1<<6 | 1<< 5 | 1<<4);
  PORTF |=  (1<<7 | 1<<6 | 1<< 5 | 1<<4);

  pixels.begin();
  Serial.begin(115200);
}

void loop(){

  int pinStates =   (PIND&(1<<0) ? 0 : (1<<0)) |
                    (PINB&(1<<7) ? 0 : (1<<1)) |
                    (PINF&(1<<4) ? 0 : (1<<2)) |
                    (PINF&(1<<5) ? 0 : (1<<3)) |
                    (PIND&(1<<1) ? 0 : (1<<4)) |
                    (PIND&(1<<5) ? 0 : (1<<5)) |
                    (PINF&(1<<7) ? 0 : (1<<6)) |
                    (PINF&(1<<6) ? 0 : (1<<7)) |
                    (PIND&(1<<6) ? 0 : (1<<8)) |
                    (PIND&(1<<4) ? 0 : (1<<9)) |
                    (PINC&(1<<7) ? 0 : (1<<10)) |
                    (PINC&(1<<6) ? 0 : (1<<11)) |
                    (PINB&(1<<4) ? 0 : (1<<12)) |
                    (PIND&(1<<7) ? 0 : (1<<13)) |
                    (PINB&(1<<6) ? 0 : (1<<14)) |
                    (PINB&(1<<5) ? 0 : (1<<15));

  // Check for button state changes
  if (pinStates != previousButtons) {
    setPixels(pinStates);
    previousButtons = pinStates;
  }
}
