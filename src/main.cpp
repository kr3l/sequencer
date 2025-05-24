#include <Arduino.h>

const int dacPin = 25; // GPIO25 supports DAC on ESP32

float rf = 5.6; // kOhm
float r1 = 10.0; // kOhm
// float gain = 1.0 + rf / r1;
float dacOutMax = 3.3;
// float ampOutMax = dacOutMax * gain; // about 5V
float ampOutMax = 5.0;
float gain = ampOutMax / dacOutMax;

struct Note {
  const char* name;
  float frequency;
};

Note noteTable[] = {
  {"C0", 16.35}, {"C#0", 17.32}, {"D0", 18.35}, {"D#0", 19.45},
  {"E0", 20.60}, {"F0", 21.83}, {"F#0", 23.12}, {"G0", 24.50},
  {"G#0", 25.96}, {"A0", 27.50}, {"A#0", 29.14}, {"B0", 30.87},
  {"C1", 32.70}, {"C#1", 34.65}, {"D1", 36.71}, {"D#1", 38.89},
  {"E1", 41.20}, {"F1", 43.65}, {"F#1", 46.25}, {"G1", 49.00},
  {"G#1", 51.91}, {"A1", 55.00}, {"A#1", 58.27}, {"B1", 61.74},
  {"C2", 65.41}, {"C#2", 69.30}, {"D2", 73.42}, {"D#2", 77.78},
  {"E2", 82.41}, {"F2", 87.31}, {"F#2", 92.50}, {"G2", 98.00},
  {"G#2", 103.83}, {"A2", 110.00}, {"A#2", 116.54}, {"B2", 123.47},
  {"C3", 130.81}, {"C#3", 138.59}, {"D3", 146.83}, {"D#3", 155.56},
  {"E3", 164.81}, {"F3", 174.61}, {"F#3", 185.00}, {"G3", 196.00},
  {"G#3", 207.65}, {"A3", 220.00}, {"A#3", 233.08}, {"B3", 246.94},
  {"C4", 261.63}, {"C#4", 277.18}, {"D4", 293.66}, {"D#4", 311.13},
  {"E4", 329.63}, {"F4", 349.23}, {"F#4", 369.99}, {"G4", 392.00},
  {"G#4", 415.30}, {"A4", 440.00}, {"A#4", 466.16}, {"B4", 493.88},
  {"C5", 523.25}
};

const int numNotes = sizeof(noteTable) / sizeof(Note);

void setup() {

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

float frequencyToVoltage(float freq) {
  float f0 = 16.35; // Reference: C0 = 0V
  return log(freq / f0) / log(2); // log base 2
}

void setNoteVoltage(String noteName) {
  for (int i = 0; i < numNotes; i++) {
    if (noteName.equalsIgnoreCase(noteTable[i].name)) {
      float voltage = frequencyToVoltage(noteTable[i].frequency);
      setAmpOutVoltage(voltage);
      return;
    }
  }
  Serial.println("Note not found: " + noteName);
}

void playNote(const char* note, int durationMs) {
  setNoteVoltage(note);
  delay(durationMs);
  setAmpOutVoltage(0); // Silence between notes
  delay(50);           // Short gap
}

void loop2() {
  // Twinkle Twinkle Little Star
  playNote("C4", 500);
  playNote("C4", 500);
  playNote("G4", 500);
  playNote("G4", 500);
  playNote("A4", 500);
  playNote("A4", 500);
  playNote("G4", 1000);

  playNote("F4", 500);
  playNote("F4", 500);
  playNote("E4", 500);
  playNote("E4", 500);
  playNote("D4", 500);
  playNote("D4", 500);
  playNote("C4", 1000);

  delay(2000); // Pause before looping
}

void loop() {
  setAmpOutVoltage(5.0);
  delay(2000);
}