#ifndef Melodies_h
#define Melodies_h

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

#endif