#include <Arduino.h>
#include "Adafruit_NeoTrellis.h"

#define LONG_PRESS_TIME 1000
#define BLINK_INTERVAL 200     // ms per blink
#define SLOT_PLAY_DURATION 1000     // 1 second per slot play

// Store press start times for each button
unsigned long pressStart[16];
bool isPressed[16];

// Programming (blinking) state
bool isProgramming[16];
unsigned long lastBlink[16];
bool ledState[16];
float programmedValues[16];

const int dacPin = 25; // GPIO25 supports DAC on ESP32
const int potPin = 36; // GPIO36 = ADC1_CH0 on ESP32 D1 mini

float rf = 5.6; // kOhm
float r1 = 10.0; // kOhm
// float gain = 1.0 + rf / r1;
float dacOutMax = 3.3;
// float ampOutMax = dacOutMax * gain; // about 5V
float ampOutMax = 5.0;
float gain = ampOutMax / dacOutMax;

Adafruit_NeoTrellis trellis;

struct Note {
  const char* name;
  float frequency;
};

Note noteTable[] = {
  {"C3", 130.81}, {"C#3", 138.59}, {"D3", 146.83}, {"D#3", 155.56},
  {"E3", 164.81}, {"F3", 174.61}, {"F#3", 185.00}, {"G3", 196.00},
  {"G#3", 207.65}, {"A3", 220.00}, {"A#3", 233.08}, {"B3", 246.94},

  {"C4", 261.63}, {"C#4", 277.18}, {"D4", 293.66}, {"D#4", 311.13},
  {"E4", 329.63}, {"F4", 349.23}, {"F#4", 369.99}, {"G4", 392.00},
  {"G#4", 415.30}, {"A4", 440.00}, {"A#4", 466.16}, {"B4", 493.88},

  {"C5", 523.25}, {"C#5", 554.37}, {"D5", 587.33}, {"D#5", 622.25},
  {"E5", 659.25}, {"F5", 698.46}, {"F#5", 739.99}, {"G5", 783.99},
  {"G#5", 830.61}, {"A5", 880.00}, {"A#5", 932.33}, {"B5", 987.77},

  {"C6", 1046.50}, {"C#6", 1108.73}, {"D6", 1174.66}, {"D#6", 1244.51},
  {"E6", 1318.51}, {"F6", 1396.91}, {"F#6", 1479.98}, {"G6", 1567.98},
  {"G#6", 1661.22}, {"A6", 1760.00}, {"A#6", 1864.66}, {"B6", 1975.53},

  {"C7", 2093.00}, {"C#7", 2217.46}, {"D7", 2349.32}, {"D#7", 2489.02},
  {"E7", 2637.02}, {"F7", 2793.83}, {"F#7", 2959.96}, {"G7", 3135.96},
  {"G#7", 3322.44}, {"A7", 3520.00}, {"A#7", 3729.31}, {"B7", 3951.07}
};

const int numNotes = sizeof(noteTable) / sizeof(Note);

const char* melodyOdeToJoy[] = {
  "E4", "E4", "F4", "G4", "G4", "F4", "E4", "D4",
  "C4", "C4", "D4", "E4", "E4", "D4", "D4", "",

  "E4", "E4", "F4", "G4", "G4", "F4", "E4", "D4",
  "C4", "C4", "D4", "E4", "D4", "C4", "C4", ""
};
const int melodyOdeToJoyLength = sizeof(melodyOdeToJoy) / sizeof(melodyOdeToJoy[0]);

const char* melodyATeam[] = {
  "E4", "G4", "C5", "G4", "E4", "G4", "C5", "G4",
  "E4", "G4", "C5", "D5", "E4", "D5", "C5", "",

  "G4", "A4", "B4", "C5", "B4", "A4", "G4", ""
};
const int melodyATeamLength = sizeof(melodyATeam) / sizeof(melodyATeam[0]);

const char* melodyTwinkle[] = {
  "C4", "C4", "G4", "G4", "A4", "A4", "G4", "F4",
  "F4", "E4", "E4", "D4", "D4", "C4"
};
const int melodyTwinkleLength = sizeof(melodyTwinkle) / sizeof(melodyTwinkle[0]);

const char* melodyFurElise[] = {
  "E5", "D#5", "E5", "D#5", "E5", "B4", "D5", "C5",
  "A4", "", "C4", "E4", "A4", "B4", "",

  "E4", "G#4", "B4", "C5", "", "E4", "E5", "D#5", "E5", "D#5", "E5",
  "B4", "D5", "C5", "A4", ""
};
const int melodyFurEliseLength = sizeof(melodyFurElise) / sizeof(melodyFurElise[0]);

float analogPercent = 0.0f;
bool isPotMode = true;
int playSlotNumber = 0;
bool isPlayMode = false;
unsigned long playSlotStart = 0;
bool startedSlotPlay = false;

// Input a value 0 to 255 to get a color value.
// The colors are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  if(WheelPos < 85) {
   return trellis.pixels.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  } else if(WheelPos < 170) {
   WheelPos -= 85;
   return trellis.pixels.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  } else {
   WheelPos -= 170;
   return trellis.pixels.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  return 0;
}

float frequencyToVoltage(float freq) {
  float f0 = 130.81; // C3 is the lowest
  return log(freq / f0) / log(2); // log base 2
}
// Set raw DAC voltage (0–3.3V)
void setDacOutVoltage(float voltage) {
  voltage = constrain(voltage, 0.0, dacOutMax);
  int dacValue = (int)(voltage / dacOutMax * 255.0);
  dacWrite(dacPin, dacValue);
}

// Set amplified output voltage (0–5V), auto-converted to DAC
void setAmpOutVoltage(float voltage) {
  voltage = constrain(voltage, 0.0, ampOutMax);
  float requiredDacVoltage = voltage / gain;
  setDacOutVoltage(requiredDacVoltage);
}

void onLongPress(int x, int y) {
  int idx = y * 4 + x;
  if (isProgramming[idx]) {
    Serial.print("Stop Programming slot ");
  } else {
    Serial.print("Start Programming slot ");
  }
  Serial.println(idx);
  // Enter or leave programming mode (start blinking)
  isProgramming[idx] = !isProgramming[idx];
  lastBlink[idx] = millis();
  ledState[idx] = false;  // start off
}

float autotune(float dacOutVoltage) {
  float requiredDacVoltage;
  for (int i = 0; i < numNotes; i ++) {
    float voltage = frequencyToVoltage(noteTable[i].frequency);
    voltage = constrain(voltage, 0.0, ampOutMax);
    requiredDacVoltage = voltage / gain;
    if (requiredDacVoltage > dacOutVoltage) {
      // first voltage larger than requested voltage, this is the note
      return requiredDacVoltage;
    }
  }
  return requiredDacVoltage;
}

void stopPlaySlot(int idx) {
  isPlayMode = false;
  trellis.pixels.setPixelColor(playSlotNumber, 0x000000); // off
  Serial.print("Stop play slot ");
  Serial.println(playSlotNumber);
  setDacOutVoltage(0.0);
}

void schedulePlaySlot(int idx) {
  if (isPlayMode) {
    stopPlaySlot(playSlotNumber);
  }
  isPlayMode = true;
  playSlotNumber = idx;
  playSlotStart = millis();
  startedSlotPlay = false; // dac value not written yet
  Serial.print("Play slot ");
  Serial.println(idx);
}

void onShortPress(int x, int y) {
  int idx = y * 4 + x;
  Serial.print("Short press detected on button ");
  Serial.println(idx);

  if (isProgramming[idx]) {
    // short press to confirm writing pot value to slot idx
    int raw = analogRead(potPin); // read raw ADC value
    // Map raw value (0–4095) to percentage
    float dacOut =  (raw / 4095.0f) * dacOutMax;

    // autotune to nearest note
    dacOut = autotune(dacOut);

    programmedValues[idx] = dacOut;
    isProgramming[idx] = false;
    Serial.print("Programmed slot ");
    Serial.println(idx);
    if (!isProgramming[idx]) {
      Serial.print("to value ");
      Serial.println(dacOut);
       trellis.pixels.setPixelColor(idx, 0x000000); // off
    }
  } else {
    // short press to request play note of slox idx
    schedulePlaySlot(idx);
  }
}

//define a callback for key presses
TrellisCallback buttonCallback(keyEvent evt){
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

void setup() {
  Serial.begin(115200);
  Serial.println("Send note name (e.g., C4, D#3), or 'off' to stop.");
  analogReadResolution(12); // 12-bit: 0–4095
  analogSetAttenuation(ADC_11db); // for input range up to ~3.3V

  if (!trellis.begin()) {
    Serial.println("Could not start trellis, check wiring?");
    while(1);
  } else {
    Serial.println("NeoPixel Trellis started");
  }

    //activate all keys and set callbacks
  for(int i=0; i<NEO_TRELLIS_NUM_KEYS; i++){
    trellis.activateKey(i, SEESAW_KEYPAD_EDGE_RISING);
    trellis.activateKey(i, SEESAW_KEYPAD_EDGE_FALLING);
    trellis.registerCallback(i, buttonCallback);
    isPressed[i] = false;
    isProgramming[i] = false;
    ledState[i] = false;
    programmedValues[i] = 0.0f;
  }

  //do a little animation to show we're on
  for (uint16_t i=0; i<trellis.pixels.numPixels(); i++) {
    trellis.pixels.setPixelColor(i, Wheel(map(i, 0, trellis.pixels.numPixels(), 0, 255)));
    trellis.pixels.show();
    delay(50);
  }
  for (uint16_t i=0; i<trellis.pixels.numPixels(); i++) {
    trellis.pixels.setPixelColor(i, 0x000000);
    trellis.pixels.show();
    delay(50);
  }
}





float setNoteVoltage(String noteName) {
  for (int i = 0; i < numNotes; i++) {
    if (noteName.equalsIgnoreCase(noteTable[i].name)) {
      float voltage = frequencyToVoltage(noteTable[i].frequency);
      setAmpOutVoltage(voltage);
      Serial.print(noteName);
      Serial.print(", ");
      Serial.print(noteTable[i].frequency);
      Serial.print("Hz, ");
      Serial.print(voltage);
      Serial.println("V");
      return voltage;
    }
  }
  Serial.println("Note not found: " + noteName);
  return 0.0;
}

void playNote(const char* note, int durationMs) {
  setNoteVoltage(note);
  delay(durationMs);
  setAmpOutVoltage(0); // Silence between notes
  delay(50);           // Short gap
}

void playMelody(const char* melody[], int length, int noteDuration = 300) {
  for (int i = 0; i < length; i++) {
    if (strlen(melody[i]) > 0) {
      playNote(melody[i], noteDuration);
    } else {
      setAmpOutVoltage(0);
      delay(noteDuration);
    }
  }

  delay(2000); // Pause before repeating
}



void loop() {
  trellis.read();
  unsigned long now = millis();

  isPotMode = false;
  // Check for long presses
  for (int i = 0; i < 16; i++) {
    if (isPressed[i] && (millis() - pressStart[i] >= LONG_PRESS_TIME)) {
      int x = i % 4;
      int y = i / 4;
      onLongPress(x, y);
      isPressed[i] = false;  // prevent repeated triggering
    }
    if (isProgramming[i]) {
      isPotMode = true;
    }
  }

  uint32_t outColor = 0xFF0000;

  if (isPotMode) {
    int raw = analogRead(potPin); // read raw ADC value
    // Map raw value (0–4095) to percentage
    float dacOut =  (raw / 4095.0f) * dacOutMax;
    dacOut = autotune(dacOut);
    setDacOutVoltage(dacOut);
    Serial.printf("Dac Out: %.1f%V\n", dacOut);

    byte out = (byte) round((dacOut / dacOutMax) * 255.0);
    outColor = Wheel(out);
  }

  // Handle blinking for programming keys
  for (int i = 0; i < 16; i++) {
    if (isProgramming[i] && (now - lastBlink[i] >= BLINK_INTERVAL)) {
      ledState[i] = !ledState[i];
      lastBlink[i] = now;
      if (ledState[i]) {
        trellis.pixels.setPixelColor(i, outColor); // red on
      } else {
        trellis.pixels.setPixelColor(i, 0x000000); // off
      }
    }
  }

  if (isPlayMode) {
    if (millis() - playSlotStart > SLOT_PLAY_DURATION) {
      isPlayMode = false;
      trellis.pixels.setPixelColor(playSlotNumber, 0x000000); // off
      Serial.print("Stop play slot ");
      Serial.println(playSlotNumber);
      setDacOutVoltage(0.0);
    }
    if (!startedSlotPlay) {
      float outValue = programmedValues[playSlotNumber];
      setDacOutVoltage(outValue);
      Serial.printf("Dac Out: %.1f%V\n", outValue);
      startedSlotPlay = true;

      byte out = (byte) round((outValue / dacOutMax) * 255.0);
      trellis.pixels.setPixelColor(playSlotNumber, Wheel(out));
    }
  }
  trellis.pixels.show();
  delay(20);
  if (!Serial.available()) {
    delay(50);
    return;
  }
  String input = Serial.readStringUntil('\n');
  input.trim();
  if (input.equalsIgnoreCase("off")) {
    setAmpOutVoltage(0);
    Serial.println("Output off.");
    isPotMode = false;
  } else if (input.equalsIgnoreCase("star")) {
    playMelody(melodyTwinkle, melodyTwinkleLength);
    isPotMode = false;
  } else if (input.equalsIgnoreCase("ode")) {
    playMelody(melodyOdeToJoy, melodyOdeToJoyLength, 400);
    isPotMode = false;
  } else if (input.equalsIgnoreCase("ateam")) {
    playMelody(melodyATeam, melodyATeamLength);
    isPotMode = false;
  } else if (input.equalsIgnoreCase("elise")) {
    playMelody(melodyFurElise, melodyFurEliseLength);
    isPotMode = false;
  } else if (input.equalsIgnoreCase("pot")) {
    isPotMode = true;
  } else {
    setNoteVoltage(input);
    isPotMode = false;
  }
}

