#include "Arduino.h"
#include "NotePlayer.h"

const Note NotePlayer::noteTable[] = {
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
const int NotePlayer::numNotes = sizeof(NotePlayer::noteTable) / sizeof(Note);

NotePlayer::NotePlayer(int _dacPin) {
    dacPin = _dacPin;
    gain = ampOutMax / dacOutMax;
    autotuneActive = true;
}

float NotePlayer::frequencyToVoltage(float freq) {
  float f0 = noteTable[0].frequency;
  return log(freq / f0) / log(2); // log base 2
}

float NotePlayer::frequencyToDacVoltage(float freq) {
  float voltage = frequencyToVoltage(freq);
  return voltage / gain;
}

// Set raw DAC voltage (0–3.3V)
void NotePlayer::setDacOutVoltage(float voltage) {
  voltage = constrain(voltage, 0.0, dacOutMax);
  int dacValue = (int)(voltage / dacOutMax * 255.0);
  dacWrite(dacPin, dacValue);
}

// Set amplified output voltage (0–5V), auto-converted to DAC
void NotePlayer::setAmpOutVoltage(float voltage) {
  voltage = constrain(voltage, 0.0, ampOutMax);
  float requiredDacVoltage = voltage / gain;
  setDacOutVoltage(requiredDacVoltage);
}

float NotePlayer::setNoteVoltage(String noteName) {
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

void NotePlayer::playNote(const char* note, int durationMs) {
  setNoteVoltage(note);
  delay(durationMs);
  setAmpOutVoltage(0); // Silence between notes
  delay(50);           // Short gap
}

void NotePlayer::playMelody(const char* melody[], int length, int noteDuration) {
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

float NotePlayer::autotune(float dacOutVoltage) {
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