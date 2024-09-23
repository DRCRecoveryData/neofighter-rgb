#include <FastLED.h>
#include "MIDIUSB.h"

#define LED_PIN 7
#define NUM_LEDS 32
#define BUTTON_COUNT 16
#define DEBOUNCE_DELAY 5
#define BRIGHTNESS 100 // Adjust brightness as needed

CRGB leds[NUM_LEDS];

// Color Palette for 128 values
const byte _R[128] = {0, 65, 130, 255, 255, 255, 130, 65, 255, 255, 130, 65, 255, 255, 130, 65, 134, 81, 40, 20, 73, 0, 0, 0, 73, 0, 0, 0, 73, 0, 0, 0, 73, 0, 0, 0, 73, 0, 0, 0, 73, 0, 0, 0, 45, 0, 0, 0, 105, 45, 24, 12, 255, 255, 130, 65, 255, 255, 130, 65, 255, 150, 117, 32, 0, 0, 0, 0, 0, 16, 125, 28, 255, 186, 174, 97, 12, 0, 0, 0, 24, 89, 174, 40, 255, 134, 113, 0, 57, 85, 53, 89, 49, 105, 210, 255, 255, 182, 142, 130, 57, 0, 12, 20, 20, 101, 130, 219, 215, 255, 158, 101, 20, 219, 125, 154, 142, 61, 113, 223, 158, 53, 24, 4, 182, 61, 178, 73};
const byte _G[128] = {0, 65, 130, 255, 61, 0, 0, 0, 186, 61, 32, 16, 174, 255, 130, 65, 255, 255, 130, 65, 255, 255, 130, 65, 255, 255, 130, 65, 255, 255, 130, 65, 255, 255, 130, 65, 194, 166, 85, 45, 134, 85, 45, 24, 36, 0, 0, 0, 53, 0, 0, 0, 61, 0, 0, 0, 65, 0, 0, 0, 12, 53, 81, 53, 57, 73, 20, 0, 69, 0, 125, 28, 0, 255, 235, 255, 138, 255, 166, 40, 0, 0, 24, 16, 49, 223, 255, 255, 255, 255, 255, 138, 81, 81, 28, 0, 69, 166, 255, 89, 40, 73, 77, 20, 28, 57, 0, 65, 73, 190, 223, 178, 20, 210, 235, 150, 101, 61, 113, 255, 0, 0, 206, 65, 174, 49, 81, 20};
const byte _B[128] = {0, 65, 130, 255, 61, 0, 0, 0, 105, 0, 0, 0, 45, 0, 0, 0, 49, 0, 0, 0, 73, 0, 0, 0, 93, 24, 12, 4, 89, 85, 45, 24, 182, 150, 73, 36, 255, 255, 130, 65, 255, 255, 130, 65, 255, 255, 130, 65, 251, 255, 130, 65, 255, 255, 130, 65, 109, 81, 40, 20, 0, 0, 0, 4, 0, 24, 109, 255, 77, 202, 125, 28, 0, 45, 4, 8, 0, 93, 255, 255, 255, 255, 121, 0, 0, 4, 20, 0, 36, 109, 202, 255, 194, 231, 255, 89, 0, 0, 0, 4, 0, 12, 32, 40, 89, 24, 0, 40, 16, 36, 45, 12, 45, 105, 138, 255, 255, 61, 113, 255, 0, 0, 0, 0, 0, 0, 0, 0};

const int buttonPins[BUTTON_COUNT] = {0, 7, 4, 5, 1, 5, 7, 6, 6, 4, 7, 6, 4, 7, 6, 5}; // Corresponding pins for buttons

int state = 0;
int prev_state = 0;
unsigned long lastDebounceTime = 0;

void noteOn(byte channel, byte pitch, byte velocity) {
    midiEventPacket_t noteOn = {0x09, 0x90 | channel, pitch, velocity};
    MidiUSB.sendMIDI(noteOn);
}

void noteOff(byte channel, byte pitch, byte velocity) {
    midiEventPacket_t noteOff = {0x08, 0x80 | channel, pitch, velocity};
    MidiUSB.sendMIDI(noteOff);
}

void setLeds(int pinStates) {
    for (int i = 0; i < BUTTON_COUNT; i++) {
        if (bitRead(pinStates, i)) { // Button down
            leds[i * 2] = CRGB(_R[i], _G[i], _B[i]); // Set color from palette
            leds[i * 2 + 1] = CRGB(_R[i], _G[i], _B[i]);
            leds[i * 2].fadeToBlackBy(255 - BRIGHTNESS); // Adjust brightness
            leds[i * 2 + 1].fadeToBlackBy(255 - BRIGHTNESS); // Adjust brightness
            noteOn(0, 40 + i, 127); // Note range 40-56
        } else { // Button up
            leds[i * 2] = CRGB::Black; // No color
            leds[i * 2 + 1] = CRGB::Black;
            noteOff(0, 40 + i, 0);
        }
    }
    FastLED.show();
}

void setup() {
    FastLED.addLeds<NEOPIXEL, LED_PIN>(leds, NUM_LEDS);
    FastLED.setBrightness(BRIGHTNESS); // Set global brightness
    
    // Setup buttons
    for (int i = 0; i < BUTTON_COUNT; i++) {
        pinMode(buttonPins[i], INPUT_PULLUP); // Configure each button pin
    }

    Serial.begin(115200);
}

void loop() {
    int pinStates = 0;

    for (int i = 0; i < BUTTON_COUNT; i++) {
        if (digitalRead(buttonPins[i]) == LOW) { // Button pressed
            pinStates |= (1 << i);
        }
    }

    if (pinStates != prev_state) {
        lastDebounceTime = millis(); // Reset debounce timer
    }

    if ((millis() - lastDebounceTime) > DEBOUNCE_DELAY) {
        if (pinStates != state) {
            state = pinStates;
            setLeds(pinStates);
        }
    }

    prev_state = pinStates;
}