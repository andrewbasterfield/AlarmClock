ARDUINO_DIR  ?= /usr/share/arduino
TARGET       ?= AlarmClock
ARDUINO_LIBS ?= LiquidCrystal LCDKeypad Time Wire Wire/utility DS1307RTC EEPROM
BOARD_TAG    ?= uno
ARDUINO_PORT ?= /dev/ttyACM0
include /usr/share/arduino/Arduino.mk
