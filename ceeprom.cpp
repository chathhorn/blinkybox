#include "ceeprom.h"
#include "cserial.h"

unsigned char eeprom_readb(int addr) {
      unsigned char val = EEPROM.read(addr);
#ifndef NDEBUG
      serial_printf("eeprom_readb: %d @ %d\n", val, addr);
#endif
      return val;
}

void eeprom_writeb(int addr, unsigned char val) {
      EEPROM.write(addr, val);
#ifndef NDEBUG
      serial_printf("eeprom_writeb: %d @ %d\n", val, addr);
#endif
}

void eeprom_updateb(int addr, unsigned char val) {
      EEPROM.update(addr, val);
#ifndef NDEBUG
      serial_printf("eeprom_updateb: %d @ %d\n", val, addr);
#endif
}

int eeprom_size(void) {
      return EEPROM.length();
}

