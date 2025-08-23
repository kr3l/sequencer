#include "Arduino.h"
#include "Player.h"

Player::Player(TrellisPad *_trellisPad, NotePlayer *_notePlayer, int _numberOfPlayableSteps) {
    trellisPad = _trellisPad;
    notePlayer = _notePlayer;
    isPlaying = false;
    playSlotNumber = 0;
    playSlotStart = 0;
    startedSlotPlay = false;
    isPlayingSequence = false;
    numberOfPlayableSteps = _numberOfPlayableSteps;

    for (int i=0; i<16; i+=1) {
        steps[i].programmedValue = 0.0f;
        steps[i].isProgramming = false;
        steps[i].duty = 100;
        steps[i].gateOn = true;
    }
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
      int nextSlotNumber = (playSlotNumber + 1) % numberOfPlayableSteps;
      stop();
      if (isPlayingSequence) {
        // advance to next slot
        Serial.print("move to step ");
        Serial.println(nextSlotNumber);
        playSlot(nextSlotNumber);
      }
    }
    
    if (startedSlotPlay) {
        unsigned long gateHighDuration = (unsigned long)((float)SLOT_PLAY_DURATION * (float)steps[playSlotNumber].duty / 100.0);
        if (millis() - playSlotStart > gateHighDuration) {
            // lower the gate!
            // for now we don't have separate gate signal, we just clear dac
            notePlayer->setDacOutVoltage(0.0);
            Serial.println("lower gate");
        }

        return;
    }

    // start slot play
    float outValue = steps[playSlotNumber].programmedValue;
    // for now we don't have separate gate signal, so we clear the dac if gate off
    if (steps[playSlotNumber].gateOn) {
        Serial.print("play step ");
        Serial.print(playSlotNumber);
        Serial.print(" ");
        Serial.print(outValue);
        notePlayer->setDacOutVoltage(outValue);
    } else {
        Serial.print("play muted step ");
        Serial.println(playSlotNumber);
        notePlayer->setDacOutVoltage(0.0);
    }
    Serial.printf("Dac Out: %.1f%V\n", outValue);
    startedSlotPlay = true;

    byte out = (byte) round((outValue / notePlayer->dacOutMax) * 255.0);
    trellisPad->trellis->pixels.setPixelColor(playSlotNumber, trellisPad->Wheel(out));
}