/*
 * Arduino IDE does not like this definition inline in Arduino.ino due to the pre-processing it does
 */

typedef struct {
  tmElements_t tm;
  uint8_t checksum;
} alarm_t;

