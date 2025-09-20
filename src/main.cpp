#include <Arduino.h>

#include "NotePlayer.h"
#include "TrellisPad.h"
#include "Player.h"
#include "Melodies.h"
#include <ESP32Encoder.h>

bool isProgramming[16];

//const int potPin = 36; // GPIO36 = ADC1_CH0 on ESP32 D1 mini
const int gatePin = 19;
const int triggerPin = 18;

NotePlayer notePlayer = NotePlayer(25); // GPIO25 supports DAC on ESP32
TrellisPad trellisPad = TrellisPad();
Player player = Player(&trellisPad, &notePlayer, 12, gatePin, triggerPin);

ESP32Encoder knobLeft;
ESP32Encoder knobRight;

enum EditMode {
    MODE_PITCH,
    MODE_DUTY,
    MODE_VELOCITY,
    // (future: MODE_PROBABILITY, MODE_VELOCITY, etc.)
};
const u_int8_t NUM_MODES = 3;
EditMode currentMode = MODE_PITCH;

// the last row of pads on neotrellis are reserved for functions
// and are not playable pads
int playButtonNumber = 12;
int modeButtonNumber = 13;

unsigned long minSlotDurationMs = 50;
unsigned long maxSlotDurationMs = 4000;

int getKnobLeftCount() {
  int count = (int32_t)knobLeft.getCount();

  return constrain(count, 0, 256);
}

void onLongPress(int x, int y) {
  int idx = y * 4 + x;
  if (player.steps[idx].isProgramming) {
    Serial.print("Stop Programming slot ");
  } else {
    Serial.print("Start Programming slot ");
  }
  Serial.println(idx);
  // Enter or leave programming mode (start blinking)
  player.steps[idx].isProgramming = !player.steps[idx].isProgramming;
  trellisPad.setBlinking(idx, player.steps[idx].isProgramming);
  trellisPad.lastBlink[idx] = millis();
  trellisPad.ledState[idx] = false;  // start off
}

void savePotToStep(int idx) {
    // short press to confirm writing pot value to slot idx
    int raw = getKnobLeftCount(); // read raw ADC value
    // Map raw value (0–4095) to percentage
    float dacOut =  (raw / 255.0f) * notePlayer.dacOutMax;

    // autotune to nearest note
    dacOut = notePlayer.autotune(dacOut);

    player.steps[idx].programmedValue = dacOut;
    player.steps[idx].isProgramming = false;
    trellisPad.setBlinking(idx, false);
    Serial.print("Programmed slot ");
    Serial.println(idx);
    Serial.print("to value ");
    Serial.println(dacOut);
    trellisPad.trellis->pixels.setPixelColor(idx, 0x000000); // off
}

void onShortPress(int x, int y) {
  int idx = y * 4 + x;
  Serial.print("Short press detected on button ");
  Serial.println(idx);

  if (idx == modeButtonNumber) {
    currentMode = (EditMode)((currentMode + 1) % NUM_MODES);
    Serial.print("Set mode=");
    Serial.println(currentMode);
  }
  if (idx == playButtonNumber) {
    player.isPlayingSequence = !player.isPlayingSequence;
    
    if (player.isPlayingSequence) {
      player.playSlot(0);
      Serial.println("Play!");
      trellisPad.trellis->pixels.setPixelColor(playButtonNumber, 0xff1493); // on
    } else {
      Serial.println("Stop!");
      trellisPad.trellis->pixels.setPixelColor(playButtonNumber, 0x000000); // off
    }
  }

  if (idx >= player.numberOfPlayableSteps) {
    return;
  }

  if (player.steps[idx].isProgramming) {
    savePotToStep(idx);
  } else {
    // short press to turn gate on or off
    player.steps[idx].gateOn = !player.steps[idx].gateOn;
    // TODO: light led too, but dimmed
    // short press to request play note of slox idx
    //player.playSlot(idx);
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("Send note name (e.g., C4, D#3), or 'off' to stop.");
  analogReadResolution(12); // 12-bit: 0–4095
  analogSetAttenuation(ADC_11db); // for input range up to ~3.3V

  trellisPad.setup();
  trellisPad.setShortPressCallback(onShortPress);
  // trellisPad.setLongPressCallback(onLongPress);

  //ESP32Encoder::useInternalWeakPullResistors = puType::down;
	// Enable the weak pull up resistors
	ESP32Encoder::useInternalWeakPullResistors = puType::up;
  knobLeft.attachHalfQuad(32, 33);
  knobRight.attachHalfQuad(13, 14);

  // clear the encoder's raw count and set the tracked count to zero
	knobLeft.clearCount();
  knobRight.clearCount();
	Serial.println("knobLeft Start = " + String((int32_t)knobLeft.getCount()));
  Serial.println("knobRight Start = " + String((int32_t)knobRight.getCount()));
}



void loop() {
  unsigned long now = millis();

  trellisPad.loop();

  uint32_t outColor = 0xFF0000;

  // each led outputs the color corresponding to programmed value with low brightness
  // by default. This can be overwritten by following code, before pixels.show() is called.
  for (int i = 0; i < player.numberOfPlayableSteps; i ++) {
    player.updateColorForStep(i);
  }

  // turning the pot while holding down a pad sets the pitch of that step
  if (currentMode == MODE_PITCH) {
    for (int i = 0; i < player.numberOfPlayableSteps; i++) {
      if (!trellisPad.isPressed[i]) {
        continue;
      }
      int raw = getKnobLeftCount(); // read raw ADC value
      Serial.print("knobLeft=");
      Serial.println(raw);
      // Map raw value (255) to dac out
      float dacOut =  (raw / 255.0f) * notePlayer.dacOutMax;
      dacOut = notePlayer.autotune(dacOut);

      // program value immediately
      player.steps[i].programmedValue = dacOut;

      // preview value by playing it out
      notePlayer.setDacOutVoltage(dacOut);

      // also light led with color corresponding to dacOut
      byte out = (byte) round((dacOut / notePlayer.dacOutMax) * 255.0);
      outColor = trellisPad.Wheel(out);
      trellisPad.trellis->pixels.setPixelColor(i, outColor);

      // only the first pressed pad is set
      break;
    }
  }

  if (currentMode == MODE_DUTY) {
    for (int i = 0; i < player.numberOfPlayableSteps; i++) {
      if (!trellisPad.isPressed[i]) {
        continue;
      }
      int raw = getKnobLeftCount(); // read raw ADC value
      // Map raw value (0–4095) to percentage 0..100
      float duty =  (raw / 255.0f) * 100.0;

      // program value immediately
      player.steps[i].duty = (uint8_t)duty;

      // also light led with color corresponding to percentage
      // (note: blinking seems more common preview)
      byte out = (byte) round((duty / 100.0) * 255.0);
      outColor = trellisPad.Wheel(out);
      trellisPad.trellis->pixels.setPixelColor(i, outColor);

      // only the first pressed pad is set
      break;
    }
  }

  if (currentMode == MODE_VELOCITY) {
    int raw = getKnobLeftCount(); // read raw ADC value
    float percent =  (raw / 255.0f) * 100.0;
    player.stepDurationMs = percent * (float)maxSlotDurationMs / 100.0;
    if (player.stepDurationMs < minSlotDurationMs) {
      player.stepDurationMs = minSlotDurationMs;
    }
  }

  player.loop();
  trellisPad.trellis->pixels.show();
  delay(20);
  if (!Serial.available()) {
    delay(50);
    return;
  }
  String input = Serial.readStringUntil('\n');
  input.trim();
  if (input.equalsIgnoreCase("off")) {
    notePlayer.setAmpOutVoltage(0);
    Serial.println("Output off.");
  } else if (input.equalsIgnoreCase("star")) {
    notePlayer.playMelody(melodyTwinkle, melodyTwinkleLength);
  } else if (input.equalsIgnoreCase("ode")) {
    notePlayer.playMelody(melodyOdeToJoy, melodyOdeToJoyLength, 400);
  } else if (input.equalsIgnoreCase("ateam")) {
    notePlayer.playMelody(melodyATeam, melodyATeamLength);
  } else if (input.equalsIgnoreCase("elise")) {
    notePlayer.playMelody(melodyFurElise, melodyFurEliseLength);
  } else if (input.equalsIgnoreCase("pot")) {
  } else {
    notePlayer.setNoteVoltage(input);
  }
}

