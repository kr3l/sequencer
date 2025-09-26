#include "Arduino.h"
#include "Player.h"

Player::Player(TrellisPad *_trellisPad, NotePlayer *_notePlayer, int _numberOfPlayableSteps, int _gatePin, int _triggerPin) {
    trellisPad = _trellisPad;
    notePlayer = _notePlayer;
    isPlayingSequence = false;
    numberOfPlayableSteps = _numberOfPlayableSteps;
    stepDurationMs = 500;
    gatePin = _gatePin;
    triggerPin = _triggerPin;
    pinMode(gatePin, OUTPUT); 
    pinMode(triggerPin, OUTPUT); 

    for (int i=0; i<16; i+=1) {
        steps[i].programmedValue = 0.0f;
        steps[i].isProgramming = false;
        steps[i].duty = 100;
        steps[i].gateOn = true;
        // "now" properties relate to actual state (not programmed state)
        steps[i].nowPlaying = false;
        steps[i].nowPlayingSince = 0;
        steps[i].nowGateHigh = false;
        steps[i].writtenDac = false;
    }
}

void Player::stop(void) {
    int playSlotNumber = getPlayingStepNumber();
    if (playSlotNumber < 0) {
        return;
    }
    steps[playSlotNumber].nowPlaying = false;
    steps[playSlotNumber].nowPlayingSince = 0;
    steps[playSlotNumber].nowGateHigh = 0;
    steps[playSlotNumber].writtenDac = false;
    trellisPad->trellis->pixels.setPixelColor(playSlotNumber, 0x000000); // off
    Serial.print("Stop play slot ");
    Serial.println(playSlotNumber);
    //notePlayer->setDacOutVoltage(0.0);
}

void Player::playSlot(int idx) {
    stop();
    steps[idx].nowPlaying = true;
    steps[idx].nowPlayingSince = millis();
    if (steps[idx].duty > 0) {
        steps[idx].nowGateHigh = true;
    } else {
        steps[idx].nowGateHigh = false;
    }
    steps[idx].writtenDac = false;
    Serial.print("Play slot ");
    Serial.println(idx);
}

bool Player::isPlaying(void) {
    for (int i=0; i<16; i+=1) {
        if (steps[i].nowPlaying) {
            return true;
        }
    }
    return false;
}

int Player::getPlayingStepNumber(void) {
    for (int i=0; i<16; i+=1) {
        if (steps[i].nowPlaying) {
            return i;
        }
    }
    return -1;
}

void Player::updateColorForStep(int idx) {
    Step *step = &steps[idx];
    if (!step->gateOn || idx >= numberOfPlayableSteps) {
        trellisPad->trellis->pixels.setPixelColor(idx, 0x000000);
        return;
    }
    float dacOut = step->programmedValue;
    if (notePlayer->autotuneActive) {
        int noteNum = constrain(step->programmedValue, 0, NotePlayer::numNotes - 1);
        float freqOut = NotePlayer::noteTable[noteNum].frequency;
        dacOut = notePlayer->frequencyToDacVoltage(freqOut);
    }
    byte out = (byte) round((dacOut / notePlayer->dacOutMax) * 255.0);
    uint32_t outColor = 0xFF0000;
    if (step->nowPlaying) {
        outColor = trellisPad->Wheel(out);
    } else {
        outColor = trellisPad->Wheel(out, 255 / 6);
    }
    trellisPad->trellis->pixels.setPixelColor(idx, outColor);
}

void Player::loop(void) {
    int nowPlayingStepNumber = getPlayingStepNumber();
    if (nowPlayingStepNumber < 0) {
        return;
    }
    Step *step = &steps[nowPlayingStepNumber];
    if ((millis() - step->nowPlayingSince) > stepDurationMs) {
      int nextSlotNumber = (nowPlayingStepNumber + 1) % numberOfPlayableSteps;
      stop();
      if (isPlayingSequence) {
        // advance to next slot
        //Serial.print("move to step ");
        Serial.println(nextSlotNumber);
        playSlot(nextSlotNumber);
      } else {
         digitalWrite(gatePin, LOW);
      }
      return;
    }
    
    if (step->writtenDac) {
        unsigned long gateHighDuration = (unsigned long)((float)stepDurationMs * (float)steps[nowPlayingStepNumber].duty / 100.0);
        // trigger goes low immediately
        digitalWrite(triggerPin, LOW);
        if (step->nowGateHigh && (millis() - steps[nowPlayingStepNumber].nowPlayingSince) > gateHighDuration) {
            // lower the gate!
            // for now we don't have separate gate signal, we just clear dac
            //notePlayer->setDacOutVoltage(0.0); // TODO remove this line

            digitalWrite(gatePin, LOW);

            //Serial.println("lower gate");
            step->nowGateHigh = false;
        }

        return;
    }

    // start slot play
    float outValue = step->programmedValue;
    // for now we don't have separate gate signal, so we clear the dac if gate off
    if (step->gateOn) {
        //Serial.print("play step ");
        //Serial.print(nowPlayingStepNumber);
        if (notePlayer->autotuneActive) {
            int noteNum = constrain((int)outValue, 0, NotePlayer::numNotes - 1);
            float freqOut = NotePlayer::noteTable[noteNum].frequency;
            float dacOut = notePlayer->frequencyToDacVoltage(freqOut);
            notePlayer->setDacOutVoltage(dacOut);
        } else {
            notePlayer->setDacOutVoltage(outValue);
        }
        
        if (step->duty > 0) {
            digitalWrite(gatePin, HIGH);
            digitalWrite(triggerPin, HIGH);
        }
    } else {
        //Serial.print("play muted step ");
        //Serial.println(nowPlayingStepNumber);
        //notePlayer->setDacOutVoltage(0.0); // TODO remove this line
        digitalWrite(gatePin, LOW);
        digitalWrite(triggerPin, LOW);
    }
    
    step->writtenDac = true;
    Serial.printf("Dac Out: %.1f%V\n", outValue);
}