#include "Arduino.h"
#include "Player.h"

Player::Player(TrellisPad *_trellisPad, NotePlayer *_notePlayer) {
    trellisPad = _trellisPad;
    notePlayer = _notePlayer;
    isPlaying = false;
    playSlotNumber = 0;
    playSlotStart = 0;
    startedSlotPlay = false;
}

void Player::stop(void) {
    isPlaying = false;
    trellisPad->trellis->pixels.setPixelColor(playSlotNumber, 0x000000); // off
    Serial.print("Stop play slot ");
    Serial.println(playSlotNumber);
    notePlayer->setDacOutVoltage(0.0);
}

void Player::playSlot(int idx) {
    if (isPlaying) {
        stop();
    }
    isPlaying = true;
    playSlotNumber = idx;
    playSlotStart = millis();
    startedSlotPlay = false; // dac value not written yet
    Serial.print("Play slot ");
    Serial.println(idx);
}

void Player::loop(void) {
    if (!isPlaying) {
        return;
    }
    if (millis() - playSlotStart > SLOT_PLAY_DURATION) {
      stop();
    }
    if (startedSlotPlay) {
        return;
    }

    // start slot play
    float outValue = steps[playSlotNumber].programmedValue;
    notePlayer->setDacOutVoltage(outValue);
    Serial.printf("Dac Out: %.1f%V\n", outValue);
    startedSlotPlay = true;

    byte out = (byte) round((outValue / notePlayer->dacOutMax) * 255.0);
    trellisPad->trellis->pixels.setPixelColor(playSlotNumber, trellisPad->Wheel(out));
}