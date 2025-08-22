#ifndef Player_h
#define Player_h

#include "NotePlayer.h"
#include "TrellisPad.h"

struct Step {
  bool isProgramming;
  float programmedValue;
};

class Player {
    public:
        Player(TrellisPad *_trellisPad, NotePlayer *_notePlayer);
        bool isPlaying;
        int playSlotNumber;
        void stop(void);
        void scheduleSlot(int idx);
        void loop(void);
        unsigned long playSlotStart;
        bool startedSlotPlay;
        Step steps[16];
    private:
        TrellisPad *trellisPad;
        NotePlayer *notePlayer;
};

#endif
