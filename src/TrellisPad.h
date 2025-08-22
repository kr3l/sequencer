#ifndef TrellisPad_h
#define TrellisPad_h
#include "Arduino.h"
#include "Adafruit_NeoTrellis.h"

#define LONG_PRESS_TIME 1000
#define BLINK_INTERVAL 200     // ms per blink
#define SLOT_PLAY_DURATION 1000     // 1 second per slot play

class TrellisPad {
    public:
        using Callback = void (*)(int,int); // pointer to function taking two ints
        TrellisPad(void);
        TrellisCallback buttonCallback(keyEvent evt);
        void setup(void);
        void loop(void);
        void onShortPress(int x, int y);
        void onLongPress(int x, int y);
        Adafruit_NeoTrellis *trellis;
        uint32_t Wheel(byte WheelPos);
        void setShortPressCallback(Callback cb);
        void setLongPressCallback(Callback cb);
        void setBlinking(int idx, bool _blinking);

        // Store press start times for each button
        unsigned long pressStart[16];
        bool isPressed[16];

        // Programming (blinking) state
        bool isBlinking[16];
        unsigned long lastBlink[16];
        bool ledState[16];
        uint32_t color[16];
    private:
        Callback shortCallback = nullptr;
        Callback longCallback = nullptr;

        // static pointer to the single instance, to avoid callback misery
        static TrellisPad* instance;

        static TrellisCallback staticButtonCallback(keyEvent evt) {
            if (instance) {
                return instance->buttonCallback(evt);
            }
            return 0;
        }
};

#endif