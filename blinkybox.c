#define NDEBUG

#include "Arduino.h"
#include "ctone.h"
#include "cserial.h"
#include "ceeprom.h"

// Stored in EEPROM.
static unsigned pot1_key;
static unsigned pot2_key;
static unsigned pot3_key;
static unsigned rot1_key;
static unsigned rot2_key;

static const int LED_BASE = 55;
static const int LED_RAND = 100;

static const int BUZ_BASE = 100;
static const int BUZ_RAND = 1000;

static const int SHORT_DELAY = 2;
static const int DELAY = 40;

static const int SWITCH_THRESH = 100;
static const int JACK_THRESH = 910;

// Number of consecutive low ticks before we consider the button plugged.
static const int PLUGGED_THRESH = 30;

// Number of ticks to buzz and beep when solved.
static const int SOLVED_TIMER = 90;

// Port mappings.
static const int BUTTON = 2;
static const int BUZ1 = 5;
static const int BUZ2 = 6;
static const int LED1 = 9;
static const int LED2 = 11;
static const int LED3 = 10;
#define SWITCH A0
#define POT1 A1
#define POT2 A2
#define POT3 A3
#define ROT1 A4
#define ROT2 A5
#define JACK A7
static const int VIB = 3;
static const int RELAY = 12;

enum {
      DORMANT = 0,
      ACTIVE,
      SOLVED,
      RESET,
      CALIBRATE,
      CALIBRATE_DONE,
};

volatile static int state = DORMANT;
volatile static int plugged;

static inline bool in_range(int, int);
static inline bool switch_flipped(void);
static inline long random_upto(long);
static inline void short_delay(void);
static inline void long_delay(void);

static void leds_on(void);
static void leds_off(void);
static void buzz_left(void);
static void buzz_right(void);
static void buzz_off(void);

static void button_pressed(void);
static void check_plugged(void);
static void write_keys(void);
static void read_keys(void);

static inline bool in_range(int x, int key) {
      return (((key > 1000) || x <= (key + 15)) && ((key < 15) || x >= (key - 15)));
}

static inline bool switch_flipped(void) {
      return analogRead(SWITCH) > SWITCH_THRESH;
}

static inline long random_upto(long howbig) {
      if (howbig == 0) return 0;
      return random() % howbig;
}

static inline void short_delay(void) {
      delay(random_upto(SHORT_DELAY));
}

static inline void long_delay(void) {
      delay(random_upto(DELAY));
}

static void leds_on(void) {
      analogWrite(LED1, LED_BASE + random_upto(LED_RAND));
      digitalWrite(LED2, HIGH);
      analogWrite(LED3, LED_BASE + random_upto(LED_RAND));
}

static void leds_off(void) {
      analogWrite(LED1, 0);
      digitalWrite(LED2, LOW);
      analogWrite(LED3, 0);
}

static void buzz_left(void) {
      analogWrite(BUZ1, LED_BASE + random_upto(LED_RAND));
      toneC(BUZ2, BUZ_BASE + random_upto(BUZ_RAND), 0);
}

static void buzz_right(void) {
      analogWrite(BUZ2, LED_BASE + random_upto(LED_RAND));
      toneC(BUZ1, BUZ_BASE + random_upto(BUZ_RAND), 0);
}

static void buzz_off(void) {
      noToneC(BUZ1);
      noToneC(BUZ2);
      analogWrite(BUZ1, 0);
      analogWrite(BUZ2, 0);
}

static inline unsigned char lo(unsigned x) {
      return x & 0xff;
}

static inline unsigned char hi(unsigned x) {
      return (x >> 8) & 0xff;
}

static inline unsigned assemble(unsigned char hi, unsigned char lo) {
      return (hi << 8) | lo;
}

static void write_keys(void) {
      pot1_key = analogRead(POT1);
      pot2_key = analogRead(POT2);
      pot3_key = analogRead(POT3);
      rot1_key = analogRead(ROT1);
      rot2_key = analogRead(ROT2);

      // Sometimes the first read returns 0?
      pot1_key = analogRead(POT1);
      pot2_key = analogRead(POT2);
      pot3_key = analogRead(POT3);
      rot1_key = analogRead(ROT1);
      rot2_key = analogRead(ROT2);

      eeprom_updateb(0, hi(pot1_key));
      eeprom_updateb(1, lo(pot1_key));
      eeprom_updateb(2, hi(pot2_key));
      eeprom_updateb(3, lo(pot2_key));
      eeprom_updateb(4, hi(pot3_key));
      eeprom_updateb(5, lo(pot3_key));
      eeprom_updateb(6, hi(rot1_key));
      eeprom_updateb(7, lo(rot1_key));
      eeprom_updateb(8, hi(rot2_key));
      eeprom_updateb(9, lo(rot2_key));
}

static void read_keys(void) {
      pot1_key = assemble(eeprom_readb(0), eeprom_readb(1));
      pot2_key = assemble(eeprom_readb(2), eeprom_readb(3));
      pot3_key = assemble(eeprom_readb(4), eeprom_readb(5));
      rot1_key = assemble(eeprom_readb(6), eeprom_readb(7));
      rot2_key = assemble(eeprom_readb(8), eeprom_readb(9));
}

// Button plug:
//
//  ^^^^^^^^^^\_/\/\_____/\/^^^^^\__
// unplugged^ plugged^  pushed^
//          INT^     INT^

static void check_plugged(void) {
      static int plugged_count;

      if (digitalRead(BUTTON) == LOW) {
            if (!plugged) {
                  ++plugged_count;

                  if (plugged_count >= PLUGGED_THRESH) {
                        plugged = true;
                  }
            }
      } else {
            plugged = false;
            plugged_count = 0;
      }
}

static void button_pressed(void) {
      if (!plugged) return;

      if (state == CALIBRATE) {
            write_keys();
            state = CALIBRATE_DONE;

#ifndef NDEBUG
            serial_puts("ISR: BUTTON PRESSED (CALIBRATION)");
            serial_printf("pot_keys: (%d, %d, %d)\n", pot1_key, pot2_key, pot3_key);
            serial_printf("rot_keys: (%d, %d)\n", rot1_key, rot2_key);
#endif

            return;
      }

      if (state != ACTIVE) return;

      int pot1 = analogRead(POT1);
      int pot2 = analogRead(POT2);
      int pot3 = analogRead(POT3);
      int rot1 = analogRead(ROT1);
      int rot2 = analogRead(ROT2);

      int jack = analogRead(JACK);

      if (
            jack <= JACK_THRESH
            && in_range(pot1, pot1_key)
            && in_range(pot2, pot2_key)
            && in_range(pot3, pot3_key)
            && in_range(rot1, rot1_key)
            && in_range(rot2, rot2_key)
      ) state = SOLVED;

#ifndef NDEBUG
      serial_puts("ISR: BUTTON PRESSED");
      serial_printf("pots: (%d, %d, %d)\n", pot1, pot2, pot3);
      serial_printf("rots: (%d, %d)\n", rot1, rot2);
      serial_printf("jack: %d\n", jack);
      serial_printf("state: %d\n", state);
#endif

}

void setup(void) {
      pinMode(LED1, OUTPUT);
      pinMode(LED2, OUTPUT);
      pinMode(LED3, OUTPUT);
      pinMode(BUZ1, OUTPUT);
      pinMode(BUZ2, OUTPUT);
      pinMode(VIB, OUTPUT);
      pinMode(RELAY, OUTPUT);

      pinMode(SWITCH, INPUT);
      pinMode(POT1, INPUT);
      pinMode(POT2, INPUT);
      pinMode(POT3, INPUT);
      pinMode(ROT1, INPUT);
      pinMode(ROT2, INPUT);

      pinMode(BUTTON, INPUT_PULLUP);

      digitalWrite(RELAY, HIGH);
      digitalWrite(VIB, HIGH);

      if (digitalRead(BUTTON) == LOW) {
            state = CALIBRATE;
      } else {
            state = DORMANT;
      }

#ifndef NDEBUG
      serial_begin(9600);
#endif

      read_keys();

#ifndef NDEBUG
      serial_printf("pots_key: (%d, %d, %d)\n", pot1_key, pot2_key, pot3_key);
      serial_printf("rots_key: (%d, %d)\n", rot1_key, rot2_key);
      serial_printf("eeprom size: %d\n", eeprom_size());
#endif

      attachInterrupt(digitalPinToInterrupt(BUTTON), button_pressed, RISING);
}

// State:
// DORMANT <-> ACTIVE -> SOLVED -> RESET
// CALIBRATE -> CALIBRATE_DONE
void loop(void) {
      switch (state) {
            // Calibration mode.
            case CALIBRATE: ;

                  check_plugged();
                  leds_on();

                  return;

            // Calibration mode finished.
            case CALIBRATE_DONE: ;

                  leds_off();
                  delay(150);
                  leds_on();
                  delay(150);

                  return;

            // Unsolved, switch in off position (everything off).
            case DORMANT: ;

                  if (switch_flipped()) {
                        state = ACTIVE;
                  }

                  return;

            // Unsolved, switch in on position (flickering lights).
            case ACTIVE: ;

                  if (!switch_flipped()) {
                        state = DORMANT;
                        return;
                  }

                  check_plugged();
                  leds_on();

                  short_delay();

                  leds_off();

                  long_delay();

                  return;

            // Switch on, solved (brighter flickering lights, vibration, sound).
            case SOLVED: ;

                  static long long int counter = SOLVED_TIMER;

                  if (!counter) {
                        state = RESET;
                        return;
                  }
                  --counter;

                  digitalWrite(VIB, LOW);

                  leds_on();
                  buzz_left();

                  long_delay();

                  leds_off();
                  buzz_off();

                  long_delay();

                  leds_on();
                  buzz_right();

                  long_delay();

                  leds_off();
                  buzz_off();

                  long_delay();

                  return;

            // Everything off, unresponsive.
            case RESET: ;

                  digitalWrite(RELAY, LOW);

                  digitalWrite(VIB, HIGH);
                  leds_off();
                  buzz_off();

                  return;
      }
}
