#include "MIDIUSB.h"
#include <Adafruit_NeoPixel.h>

#define NEO_PIN         7
#define NUM_PXL        32  // 16 buttons * 2 LEDs per button
#define debounceDelay   5
#define MIDI_CHANNEL    1   // Matches Channel 1 (0 in code parameters = Channel 1)

// Button states for debouncing
int state = 0;
int prev_state = 0;
unsigned long lastDebounceTime = 0;

// Keep button states so we're only sending midi signals when there's a change
int previousButtons = 0;

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUM_PXL, NEO_PIN, NEO_GRB + NEO_KHZ800);

// Accurate Midi Fighter color tables (0-127 velocity lookup data arrays)
const byte _r[128] = {0, 65, 130, 255, 255, 255, 130, 65, 255, 255, 130, 65, 255, 255, 130, 65, 134, 81, 40, 20, 73, 0, 0, 0, 73, 0, 0, 0, 73, 0, 0, 0, 73, 0, 0, 0, 73, 0, 0, 0, 73, 0, 0, 0, 45, 0, 0, 0, 105, 45, 24, 12, 255, 255, 130, 65, 255, 255, 130, 65, 255, 150, 117, 32, 0, 0, 0, 0, 0, 16, 125, 28, 255, 186, 174, 97, 12, 0, 0, 0, 24, 89, 174, 40, 255, 134, 113, 0, 57, 85, 53, 89, 49, 105, 210, 255, 255, 182, 142, 130, 57, 0, 12, 20, 20, 101, 130, 219, 215, 255, 158, 101, 20, 219, 125, 154, 142, 61, 113, 223, 158, 53, 24, 4, 182, 61, 178, 73};
const byte _g[128] = {0, 65, 130, 255, 61, 0, 0, 0, 186, 61, 32, 16, 174, 255, 130, 65, 255, 255, 130, 65, 255, 255, 130, 65, 255, 255, 130, 65, 255, 255, 130, 65, 255, 255, 130, 65, 194, 166, 85, 45, 134, 85, 45, 24, 36, 0, 0, 0, 53, 0, 0, 0, 61, 0, 0, 0, 65, 0, 0, 0, 12, 53, 81, 53, 57, 73, 20, 0, 69, 0, 125, 28, 0, 255, 235, 255, 138, 255, 166, 40, 0, 0, 24, 16, 49, 223, 255, 255, 255, 255, 255, 138, 81, 81, 28, 0, 69, 166, 255, 89, 40, 73, 77, 20, 28, 57, 0, 65, 73, 190, 223, 178, 20, 210, 235, 150, 101, 61, 113, 255, 0, 0, 206, 65, 174, 49, 81, 20};
const byte _b[128] = {0, 65, 130, 255, 61, 0, 0, 0, 105, 0, 0, 0, 45, 0, 0, 0, 49, 0, 0, 0, 73, 0, 0, 0, 93, 24, 12, 4, 89, 85, 45, 24, 182, 150, 73, 36, 255, 255, 130, 65, 255, 255, 130, 65, 255, 255, 130, 65, 251, 255, 130, 65, 255, 255, 130, 65, 109, 81, 40, 20, 0, 0, 0, 4, 0, 24, 109, 255, 77, 202, 125, 28, 0, 45, 4, 8, 0, 93, 255, 255, 255, 255, 121, 0, 0, 4, 20, 0, 36, 109, 202, 255, 194, 231, 255, 89, 0, 0, 0, 4, 0, 12, 32, 40, 89, 24, 0, 40, 16, 36, 45, 12, 45, 105, 138, 255, 255, 61, 113, 255, 0, 0, 0, 0, 0, 0, 0, 0};

void noteOn(byte channel, byte pitch, byte velocity) {
  midiEventPacket_t noteOn = {0x09, 0x90 | channel, pitch, velocity};
  MidiUSB.sendMIDI(noteOn);
}

void noteOff(byte channel, byte pitch, byte velocity) {
  midiEventPacket_t noteOff = {0x08, 0x80 | channel, pitch, velocity};
  MidiUSB.sendMIDI(noteOff);
}

// Maps incoming MIDI pitch numbers (40-55) to the respective button index (0-15)
int getButtonIndex(int note) {
  if (note >= 40 && note <= 55) {
    return (note - 40);
  }
  return -1;
}

void handleLocalButtonEdge(int pinStates) {
  for(int i = 0; i < 16; i++) {
    if (bitRead(pinStates, i) != bitRead(previousButtons, i)) {
      if (bitRead(pinStates, i)) {
        bitWrite(previousButtons, i, 1);
        noteOn(MIDI_CHANNEL - 1, (40 + i), 127); // Note range 40-55
        MidiUSB.flush();
      } else {
        bitWrite(previousButtons, i, 0);
        noteOff(MIDI_CHANNEL - 1, (40 + i), 0);
        MidiUSB.flush();
      }
    }
  }
}

void setup() {
  /* Pin Configuration (Original 32u4 inputs layout)
   * btn: 1   2   3   4   5   6   7   8   9   10  11  12  13  14  15  16
   * pin: D0  B7  F4  F5  D1  D5  F7  F6  D6  D4  C7  C6  B4  D7  B6  B5
   */
  DDRB  &= ~(1<<7 | 1<<6 | 1<< 5 | 1<<4);
  PORTB |=  (1<<7 | 1<<6 | 1<< 5 | 1<<4);
  DDRC  &= ~(1<<7 | 1<<6);
  PORTC |=  (1<<7 | 1<<6);
  DDRD  &= ~(1<<7 | 1<<6 | 1<< 5 | 1<<4 | 1<<1 | 1<<0);
  PORTD |=  (1<<7 | 1<<6 | 1<< 5 | 1<<4 | 1<<1 | 1<<0);
  DDRF  &= ~(1<<7 | 1<<6 | 1<< 5 | 1<<4);
  PORTF |=  (1<<7 | 1<<6 | 1<< 5 | 1<<4);

  pixels.begin();
  
  // POST Startup Animation: Sets all dual-pixels to a dim background pink (30,10,10)
  for(int i = 0; i < 16; i++) {
    pixels.setPixelColor((i * 2), pixels.Color(30, 10, 10));
    pixels.setPixelColor((i * 2) + 1, pixels.Color(30, 10, 10));
  }
  pixels.show();
  
  Serial.begin(115200);
}

void loop() {
  // 1. Process Downstream MIDI Packets from DAW (The new function feature)
  midiEventPacket_t rxPacket = MidiUSB.read();
  if (rxPacket.header != 0) {
    uint8_t type     = rxPacket.byte1 & 0xF0;
    uint8_t channel  = (rxPacket.byte1 & 0x0F) + 1;
    uint8_t note     = rxPacket.byte2;
    uint8_t velocity = rxPacket.byte3;

    if (channel == MIDI_CHANNEL) {
      int btnIndex = getButtonIndex(note);
      if (btnIndex != -1) {
        if (type == 0x90 && velocity > 0) { // Note On
          uint8_t vIdx = velocity & 0x7F; 
          // Extract specific color values from the lookup table arrays based on MIDI velocity
          pixels.setPixelColor((btnIndex * 2), pixels.Color(_r[vIdx], _g[vIdx], _b[vIdx]));
          pixels.setPixelColor((btnIndex * 2) + 1, pixels.Color(_r[vIdx], _g[vIdx], _b[vIdx]));
        } 
        else if (type == 0x80 || (type == 0x90 && velocity == 0)) { // Note Off
          // Revert back to the idle default standby hue (Pink)
          pixels.setPixelColor((btnIndex * 2), pixels.Color(30, 10, 10));
          pixels.setPixelColor((btnIndex * 2) + 1, pixels.Color(30, 10, 10));
        }
        pixels.show();
      }
    }
  }

  // 2. Original Hardware Button Matrix Parsing (Blazing fast 32u4 IO reads)
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

  if (pinStates != prev_state) {
    lastDebounceTime = millis();
  }
  
  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (pinStates != state) {
      state = pinStates;
      handleLocalButtonEdge(pinStates);
    }
  }

  prev_state = pinStates;  
}
