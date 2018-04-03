#ifndef CSERIAL_H_
#define CSERIAL_H_

#include "Arduino.h"

#ifdef __cplusplus
extern "C" {
#endif

void serial_begin(int);
void serial_printf(char *, ...);
void serial_puts(char *);

#ifdef __cplusplus
}
#endif

#endif
