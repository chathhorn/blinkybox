#ifndef CTONE_H_
#define CTONE_H_

#include "Arduino.h"

#ifdef __cplusplus
extern "C" void toneC(uint8_t, unsigned, unsigned long);
extern "C" void noToneC(uint8_t);
#else
void toneC(uint8_t, unsigned, unsigned long);
void noToneC(uint8_t);
#endif

#endif
