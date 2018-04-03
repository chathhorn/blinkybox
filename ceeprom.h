#ifndef CEEPROM_H_
#define CEEPROM_H_

#ifdef __cplusplus
#include "EEPROM.h"
extern "C" {
#endif

unsigned char eeprom_readb(int);
void eeprom_writeb(int, unsigned char);
void eeprom_updateb(int, unsigned char);
int eeprom_size(void);

#ifdef __cplusplus
}
#endif

#endif
