#ifndef NotePlayer_h
#define NotePlayer_h
#include "Arduino.h"

struct Note {
  const char* name;
  float frequency;
};

class NotePlayer {
	public:
		NotePlayer(int _dacPin);
        float frequencyToVoltage(float);
        void setDacOutVoltage(float voltage);
        void setAmpOutVoltage(float voltage);
        float setNoteVoltage(String noteName);
        void playNote(const char* note, int durationMs);
        const char* voltageToNoteName(float voltage);
        void playMelody(const char* melody[], int length, int noteDuration = 300);
        float autotune(float dacOutVoltage);
        float dacOutMax = 3.3;
        float ampOutMax = 5.0;
        float gain;
        int dacPin;
	private:

};

#endif
