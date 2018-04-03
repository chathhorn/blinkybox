#include "ctone.h"

void toneC(uint8_t pin, unsigned frequency, unsigned long duration) {
      tone(pin, frequency, duration);
}

void noToneC(uint8_t pin) {
      noTone(pin);
}
