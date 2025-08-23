#ifndef Player_h
#define Player_h

#include "NotePlayer.h"
#include "TrellisPad.h"

struct Step {
  bool isProgramming;
  float programmedValue; // DAC out voltage 0-3.3V
  bool gateOn;      // true = gate enabled
  uint8_t duty;     // % of step length (0–100)
};

class Player {
    public:
        Player(TrellisPad *_trellisPad, NotePlayer *_notePlayer, int _numberOfPlayableSteps);
        bool isPlaying;
        int playSlotNumber;
        void stop(void);
        void playSlot(int idx);
        void loop(void);
        unsigned long playSlotStart;
        bool startedSlotPlay;
        bool isPlayingSequence;
        Step steps[16];
        int numberOfPlayableSteps;
    private:
        TrellisPad *trellisPad;
        NotePlayer *notePlayer;
};

#endif
