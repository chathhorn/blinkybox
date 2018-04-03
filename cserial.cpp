#include "cserial.h"
#include <stdarg.h>

void serial_begin(int baud) {
      Serial.begin(baud);
}

void serial_printf(char * fmt, ...) {
      char buf[128];


      va_list args;
      va_start(args, fmt);

      vsnprintf(buf, 128, fmt, args);

      va_end(args);

      Serial.print(buf);
}

void serial_puts(char * s) {
      Serial.println(s);
}
