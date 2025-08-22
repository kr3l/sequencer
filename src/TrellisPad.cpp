#include "Arduino.h"
#include "TrellisPad.h"

TrellisPad* TrellisPad::instance = nullptr;

TrellisPad::TrellisPad() {
    trellis = new Adafruit_NeoTrellis();
    if (instance) {
        Serial.println("Multiple instances of TrellisPad are not supported!");
    }
    instance = this; // store the singleton instance
}

//define a callback for key presses
TrellisCallback TrellisPad::buttonCallback(keyEvent evt) {
  uint8_t idx = evt.bit.NUM;
  // Check is the pad pressed?
  if (evt.bit.EDGE == SEESAW_KEYPAD_EDGE_RISING) {
    // button pressed
    pressStart[idx] = millis();
    isPressed[idx] = true;
  //  trellis.pixels.setPixelColor(evt.bit.NUM, Wheel(map(evt.bit.NUM, 0, trellis.pixels.numPixels(), 0, 255))); //on rising
  } else if (evt.bit.EDGE == SEESAW_KEYPAD_EDGE_FALLING) {
  // or is the pad released?
    isPressed[idx] = false;
  //  trellis.pixels.setPixelColor(evt.bit.NUM, 0); //off falling

    if (millis() - pressStart[idx] < LONG_PRESS_TIME / 2) {
      // short press
      onShortPress(idx % 4, idx / 4);
    }
  }

  // Turn on/off the neopixels!
  // trellis.pixels.show();

  return 0;
}

void TrellisPad::setup(void) {
  if (!trellis->begin()) {
    Serial.println("Could not start trellis, check wiring?");
    while(1);
  } else {
    Serial.println("NeoPixel Trellis started");
  }
  
  //activate all keys and set callbacks
  for(int i=0; i<NEO_TRELLIS_NUM_KEYS; i++){
    trellis->activateKey(i, SEESAW_KEYPAD_EDGE_RISING);
    trellis->activateKey(i, SEESAW_KEYPAD_EDGE_FALLING);
    trellis->registerCallback(i, staticButtonCallback);
    isPressed[i] = false;
    isBlinking[i] = false;
    ledState[i] = false;
    color[i] = 0x000000;
  }

  //do a little animation to show we're on
  for (uint16_t i=0; i<trellis->pixels.numPixels(); i++) {
    trellis->pixels.setPixelColor(i, Wheel(map(i, 0, trellis->pixels.numPixels(), 0, 255)));
    trellis->pixels.show();
    delay(50);
  }
  for (uint16_t i=0; i<trellis->pixels.numPixels(); i++) {
    trellis->pixels.setPixelColor(i, 0x000000);
    trellis->pixels.show();
    delay(50);
  }
}

void TrellisPad::loop(void) {
    trellis->read();
    unsigned long now = millis();

      // Check for long presses
    for (int i = 0; i < 16; i++) {
        if (isPressed[i] && (now - pressStart[i] >= LONG_PRESS_TIME)) {
            int x = i % 4;
            int y = i / 4;
            onLongPress(x, y);
            isPressed[i] = false;  // prevent repeated triggering
        }
    }

    // Handle blinking for programming keys
    for (int i = 0; i < 16; i++) {
        if (isBlinking[i] && (now - lastBlink[i] >= BLINK_INTERVAL)) {
        ledState[i] = !ledState[i];
        lastBlink[i] = now;
        if (ledState[i]) {
            trellis->pixels.setPixelColor(i, color[i]); // red on
        } else {
            trellis->pixels.setPixelColor(i, 0x000000); // off
        }
        }
    }
}

void TrellisPad::setBlinking(int idx, bool _blinking) {
  isBlinking[idx] = _blinking;
}

void TrellisPad::setShortPressCallback(Callback cb) {
    shortCallback = cb;
}

void TrellisPad::setLongPressCallback(Callback cb) {
    longCallback = cb;
}

void TrellisPad::onShortPress(int x, int y) {
  int idx = y * 4 + x;
  Serial.print("Short press detected on button ");
  Serial.println(idx);

  if (shortCallback) {
    shortCallback(x, y);
  }
}

void TrellisPad::onLongPress(int x, int y) {
  int idx = y * 4 + x;
  Serial.print("Long press detected on button ");
  Serial.println(idx);

  if (longCallback) {
    longCallback(x, y);
  }
}

// Input a value 0 to 255 to get a color value.
// The colors are a transition r - g - b - back to r.
uint32_t TrellisPad::Wheel(byte WheelPos) {
  if(WheelPos < 85) {
   return trellis->pixels.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  } else if(WheelPos < 170) {
   WheelPos -= 85;
   return trellis->pixels.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  } else {
   WheelPos -= 170;
   return trellis->pixels.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  return 0;
}