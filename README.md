ESP32 + TL074

the TL074 opamp is fed with -12 and +12V

GPIO25 -> pin3 of opamp
5.6kOhm + 1kOhm precision trimmer between pin 1 and 2 of opamp (feedback resistor)
10kOhm resistor between pin 2 and GND
-> opamp gain 1 + 5.6/10 -> 1.56
-> output max goes from 3.3V to 5.148V

100nF between pin 1 and GND

precision trimmer calibrated so the max outputted voltage on DAC results in exactly 5V