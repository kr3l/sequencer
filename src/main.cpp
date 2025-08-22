#include <Arduino.h>

#include "NotePlayer.h"
#include "TrellisPad.h"
#include "Player.h"

bool isProgramming[16];

const int potPin = 36; // GPIO36 = ADC1_CH0 on ESP32 D1 mini

NotePlayer notePlayer = NotePlayer(25); // GPIO25 supports DAC on ESP32
TrellisPad trellisPad = TrellisPad();
Player player = Player(&trellisPad, &notePlayer);

const char* melodyOdeToJoy[] = {
  "E4", "E4", "F4", "G4", "G4", "F4", "E4", "D4",
  "C4", "C4", "D4", "E4", "E4", "D4", "D4", "",

  "E4", "E4", "F4", "G4", "G4", "F4", "E4", "D4",
  "C4", "C4", "D4", "E4", "D4", "C4", "C4", ""
};
const int melodyOdeToJoyLength = sizeof(melodyOdeToJoy) / sizeof(melodyOdeToJoy[0]);

const char* melodyATeam[] = {
  "E4", "G4", "C5", "G4", "E4", "G4", "C5", "G4",
  "E4", "G4", "C5", "D5", "E4", "D5", "C5", "",

  "G4", "A4", "B4", "C5", "B4", "A4", "G4", ""
};
const int melodyATeamLength = sizeof(melodyATeam) / sizeof(melodyATeam[0]);

const char* melodyTwinkle[] = {
  "C4", "C4", "G4", "G4", "A4", "A4", "G4", "F4",
  "F4", "E4", "E4", "D4", "D4", "C4"
};
const int melodyTwinkleLength = sizeof(melodyTwinkle) / sizeof(melodyTwinkle[0]);

const char* melodyFurElise[] = {
  "E5", "D#5", "E5", "D#5", "E5", "B4", "D5", "C5",
  "A4", "", "C4", "E4", "A4", "B4", "",

  "E4", "G#4", "B4", "C5", "", "E4", "E5", "D#5", "E5", "D#5", "E5",
  "B4", "D5", "C5", "A4", ""
};
const int melodyFurEliseLength = sizeof(melodyFurElise) / sizeof(melodyFurElise[0]);

float analogPercent = 0.0f;
bool isPotMode = true;

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
    int raw = analogRead(potPin); // read raw ADC value
    // Map raw value (0–4095) to percentage
    float dacOut =  (raw / 4095.0f) * notePlayer.dacOutMax;

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

  if (player.steps[idx].isProgramming) {
    savePotToStep(idx);
  } else {
    // short press to request play note of slox idx
    player.playSlot(idx);
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("Send note name (e.g., C4, D#3), or 'off' to stop.");
  analogReadResolution(12); // 12-bit: 0–4095
  analogSetAttenuation(ADC_11db); // for input range up to ~3.3V

  trellisPad.setup();
  trellisPad.setShortPressCallback(onShortPress);
  trellisPad.setLongPressCallback(onLongPress);
  for(int i=0; i<16; i++){
    player.steps[i].programmedValue = 0.0f;
    player.steps[i].isProgramming = false;
  }
}

void loop() {
  unsigned long now = millis();

  isPotMode = false;
  trellisPad.loop();

  // Check for long presses
  for (int i = 0; i < 16; i++) {
    if (player.steps[i].isProgramming) {
      isPotMode = true;
    }
  }

  uint32_t outColor = 0xFF0000;

  if (isPotMode) {
    int raw = analogRead(potPin); // read raw ADC value
    // Map raw value (0–4095) to percentage
    float dacOut =  (raw / 4095.0f) * notePlayer.dacOutMax;
    dacOut = notePlayer.autotune(dacOut);
    notePlayer.setDacOutVoltage(dacOut);
    Serial.printf("Dac Out: %.1f%V\n", dacOut);

    byte out = (byte) round((dacOut / notePlayer.dacOutMax) * 255.0);
    outColor = trellisPad.Wheel(out);

    for (int i = 0; i < 16; i++) {
      if (player.steps[i].isProgramming) {
        trellisPad.color[i] = outColor;
      }
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
    isPotMode = false;
  } else if (input.equalsIgnoreCase("star")) {
    notePlayer.playMelody(melodyTwinkle, melodyTwinkleLength);
    isPotMode = false;
  } else if (input.equalsIgnoreCase("ode")) {
    notePlayer.playMelody(melodyOdeToJoy, melodyOdeToJoyLength, 400);
    isPotMode = false;
  } else if (input.equalsIgnoreCase("ateam")) {
    notePlayer.playMelody(melodyATeam, melodyATeamLength);
    isPotMode = false;
  } else if (input.equalsIgnoreCase("elise")) {
    notePlayer.playMelody(melodyFurElise, melodyFurEliseLength);
    isPotMode = false;
  } else if (input.equalsIgnoreCase("pot")) {
    isPotMode = true;
  } else {
    notePlayer.setNoteVoltage(input);
    isPotMode = false;
  }
}

