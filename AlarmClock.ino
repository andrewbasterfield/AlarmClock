/*
 *    Copyright 2014 Andrew Basterfield
 * 
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 * 
 *        http://www.apache.org/licenses/LICENSE-2.0
 * 
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#include <LiquidCrystal.h>
#include <LCDKeypad.h>
#include <Time.h>
#include <Wire.h>
#include <DS1307RTC.h>
#include <EEPROM.h>
#include "EEPROMAnything.h"

/*
 * Timeouts to revert back to time display
 */
#define SET_MODE_TIMEOUT_LOOPS (1000) /* How many loops before set mode changes back to display mode */
#define DISPLAY_TIMEOUT_LOOPS  (2000) /* How many loops before alarm displays change back to real time display */
#define LOOP_SLEEP (10)               /* How many millis to sleep for in loop() */
#define BUTTON_DEBOUNCE_LOOPS (10)    /* How many loops must a button be held down for before it registers */
/*
 * Alarm
 */
#define QTY_ALARMS (16)

typedef struct {
  tmElements_t tm;
  uint8_t checksum;
} alarm_t;

uint8_t getAlarmChecksum(alarm_t alarm) {
  uint8_t c = 0;
  c += alarm.tm.Second;
  c += alarm.tm.Minute;
  c += alarm.tm.Hour;
  c += alarm.tm.Wday;
  c += alarm.tm.Day;
  c += alarm.tm.Month;
  c += alarm.tm.Year;  

  return c;
}

void saveAlarmTime(alarm_t alarm,uint8_t i) {
  Serial.print("Saving alarm ");
  Serial.print(i);
  Serial.print("\n");
    
  alarm.checksum = getAlarmChecksum(alarm);
  
  Serial.print("Using checksum: ");
  Serial.print(alarm.checksum);
  Serial.print("\n");
  
  EEPROM_writeAnything(i * sizeof(alarm_t), alarm);
}

alarm_t retrieveAlarmTime(uint8_t i) {
  tmElements_t alarm_time = { 0, 0, 0, 1, 1, 1, 1 };
  
  alarm_t alarm;

  EEPROM_readAnything(i * sizeof(alarm_t), alarm);
  Serial.print("Loaded alarm ");
  Serial.print(i);
  Serial.print(" : ");
  Serial.print("\n");
  
  if (alarm.checksum == getAlarmChecksum(alarm)) {
    Serial.print("Checksum valid: ");
    Serial.print(alarm.checksum);
    Serial.print("\n");
    return alarm;
  }

  Serial.print("Checksum invalid: ROM: ");
  Serial.print(alarm.checksum);
  Serial.print(" Calculated: ");
  Serial.print(getAlarmChecksum(alarm));
  Serial.print("\n");
  
  alarm.tm = alarm_time;
  alarm.checksum = 0;
  
  return alarm;
}

uint8_t checkAlarm(alarm_t alarm, tmElements_t cur) {
  if (alarm.tm.Second < 60 && alarm.tm.Second != cur.Second) return 0;
  if (alarm.tm.Minute < 60 && alarm.tm.Minute != cur.Minute) return 0;
  if (alarm.tm.Hour   < 60 && alarm.tm.Hour   != cur.Hour)   return 0;
  if (alarm.tm.Wday   >  0 && alarm.tm.Wday   != cur.Wday)   return 0;
  if (alarm.tm.Day    >  0 && alarm.tm.Day    != cur.Day)    return 0;
  if (alarm.tm.Month  >  0 && alarm.tm.Month  != cur.Month)  return 0;
  if (alarm.tm.Year   >  0 && alarm.tm.Year   != cur.Year)   return 0;  
  return 1;
}

/*
 * Time sync
 */
#define TIME_MSG_LEN  11   // time sync to PC is HEADER followed by unix time_t as ten ascii digits
#define TIME_HEADER  'T'   // Header tag for serial time sync message
#define TIME_HEADER2 'U'
#define TIME_REQUEST 'A'   // ASCII bell character requests a time sync message 

void setCurrentTime(time_t this_time) {
  setTime(this_time);   // Sync Arduino software clock
  RTC.set(this_time);   // Sync RTC
  Serial.print("Writing ");
  Serial.print(this_time);
  Serial.print(" to RTC\n");
}

time_t requestSync() {
  Serial.print("Request Sync\n");
  //return 0; // the time will be sent later in response to serial mesg
  time_t rtctime = RTC.get();
  Serial.print("Got ");
  Serial.print(rtctime);
  Serial.print(" from RTC\n");
  return rtctime;
}

/*
 * Read a byte blocking when Serial.read() returns -1
 */
uint8_t readbyte() {
  int16_t c = -1;
  
  while (c < 0) {
    c = Serial.read();
  }
  
  return c;
}

void processSyncMessage() {

  time_t pctime = 0;
  tmElements_t pc_tm;
  memset(&pc_tm,0,sizeof(tmElements_t));
  uint8_t enable = 0;

  while(Serial.available()) {

    char c = Serial.read();    

    switch (c) {

     case 'T': // second SS
      enable = 1;
      break;

     case 'S': // second SS
      for (uint8_t i=0; i< 2; i++) {
        c = readbyte();
        if (c >= '0' && c <= '9') {
          pc_tm.Second = (10 * pc_tm.Second) + (c - '0');
        }
      }
      Serial.print("Got Second ");
      Serial.print(pc_tm.Second);
      Serial.print(" from Serial\n");
      break;

     case 'M': // minute MM
      for (uint8_t i=0; i< 2; i++) {
        c = readbyte();
        if (c >= '0' && c <= '9') {
          pc_tm.Minute = (10 * pc_tm.Minute) + (c - '0');
        }
      }
      Serial.print("Got Minute ");
      Serial.print(pc_tm.Minute);
      Serial.print(" from Serial\n");
      break;

     case 'H': // hour HH
      for (uint8_t i=0; i< 2; i++) {
        c = readbyte();
        if (c >= '0' && c <= '9') {
          pc_tm.Hour = (10 * pc_tm.Hour) + (c - '0');
        }
      }
      Serial.print("Got Hour ");
      Serial.print(pc_tm.Hour);
      Serial.print(" from Serial\n");
      break;

     case 'd': // day DD
      for (uint8_t i=0; i< 2; i++) {
        c = readbyte();
        if (c >= '0' && c <= '9') {
          pc_tm.Day = (10 * pc_tm.Day) + (c - '0');
        }
      }
      Serial.print("Got Day ");
      Serial.print(pc_tm.Day);
      Serial.print(" from Serial\n");
      break;

     case 'm': // month mm
      for (uint8_t i=0; i< 2; i++) {
        c = readbyte();
        if (c >= '0' && c <= '9') {
          pc_tm.Month = (10 * pc_tm.Month) + (c - '0');
        }
      }
      Serial.print("Got Month ");
      Serial.print(pc_tm.Month);
      Serial.print(" from Serial\n");
      break;

     case 'Y': // year YYYY
      unsigned int year = 0;
      for (uint8_t i=0; i< 4; i++) {
        c = readbyte();
        if (c >= '0' && c <= '9') {
          year = (10 * year) + (c - '0');
        }
      }
      
      pc_tm.Year = year - 1970;
      Serial.print("Got Year ");
      Serial.print(pc_tm.Year);
      Serial.print(" from Serial\n");
      break;
    }
    
  }

  if (enable) {
    pctime = makeTime(pc_tm);
    Serial.print("Received time ");
    Serial.print(pctime);
    Serial.print(" from Serial\n");
    setCurrentTime(pctime);
  }
}

/*
 * UI
 */
const char* myDayShortStr(uint8_t day) {
  switch (day) {
    case 1:
      return "Sun";
    case 2:
      return "Mon";
    case 3:
      return "Tue";
    case 4:
      return "Wed";
    case 5:
      return "Thu";
    case 6:
      return "Fri";
    case 7:
      return "Sat";
    default:
      return "???";
  }
}

/*
 * Update the parts of the display that need it
 */
LCDKeypad lcd;
tmElements_t last_tm = { 255, 255, 255, 255, 255, 255, 255 };
uint8_t last_time_status = 255;
uint8_t last_alarm = 0;
void digitalClockDisplay(tmElements_t* tm, uint8_t alarm) {

  if (alarm != last_alarm) {
    lcd.setCursor(0,0);
    if (alarm == 0) {
      lcd.print("   ");
    } else {
      lcd.print(alarm);
    }
    last_alarm = alarm;
  }

  if (tm->Hour != last_tm.Hour) {
    lcd.setCursor(4,0);
    if (tm->Hour > 23) {
      lcd.print("**");
    } else {
      if (tm->Hour < 10) {
        lcd.print("0");
      }
      lcd.print(tm->Hour);
    }
    lcd.print(":");
    last_tm.Hour = tm->Hour;
  }
  
  if (tm->Minute != last_tm.Minute) {
    lcd.setCursor(7,0);
    if (tm->Minute > 59) {
      lcd.print("**");
    } else {
      if (tm->Minute < 10) {
        lcd.print("0");
      }
      lcd.print(tm->Minute);
    }
    lcd.print(":");
    last_tm.Minute = tm->Minute;
  }
  
  if (tm->Second != last_tm.Second) {
    lcd.setCursor(10,0);
    if (tm->Second > 59) {
      lcd.print("**");
    } else {
      if (tm->Second < 10) {
        lcd.print("0");
      }
      lcd.print(tm->Second);
    }
    last_tm.Second = tm->Second;
  }
  
  uint8_t needsSync = (timeStatus() == timeNeedsSync) ? 1 : 0;
  
  if (needsSync != last_time_status) {
    lcd.setCursor(15,0);
    if (needsSync) {
      lcd.print("!");
    } else {
      lcd.print(" ");
    }
    last_time_status = needsSync;
  }

  if (tm->Wday != last_tm.Wday) {
    lcd.setCursor(0,1);
    if (tm->Wday == 0) {
      lcd.print("***");
    } else {
      lcd.print(myDayShortStr(tm->Wday));
    }
    last_tm.Wday = tm->Wday;
  }
  
  if (tm->Day != last_tm.Day) {
    lcd.setCursor(4,1);
    if (tm->Day == 0) {
      lcd.print("**");
    } else {
      if (tm->Day < 10) {
        lcd.print("0");
      }
      lcd.print(tm->Day);
    }
    last_tm.Day = tm->Day;
  }

  if (tm->Month != last_tm.Month) {
    lcd.setCursor(7,1);
    if (tm->Month == 0) {
      lcd.print("***");
    } else {
      lcd.print(monthShortStr(tm->Month));
    }
    last_tm.Month = tm->Month;
  }

  if (tm->Year != last_tm.Year) {
    lcd.setCursor(11,1);
    if (tm->Year == 0) {
      lcd.print("****");
    } else {
      lcd.print(tm->Year + 1970);
    }
    last_tm.Year = tm->Year;
  }
}

int lastButton = KEYPAD_NONE;
uint8_t counter = 0;
/*
 * Check for buttons that are pushed
 */
int checkButton() {
  int res = KEYPAD_NONE;

  int thisButton = lcd.button();

  if (thisButton == lastButton) {
    // button state is unchanged, increment unchanged counter
    counter++;
  } else {
    counter = 0;
  }

  if (counter > BUTTON_DEBOUNCE_LOOPS) {
    // register the button push, restart counting
    res = thisButton;
    counter = 0;
  }

  lastButton = thisButton;

  return res;
}

uint8_t selectedDigitCounter = 0;
uint8_t processTimeSetButtons(tmElements_t* tm, int button, int wildcards) {

  switch (button) {
    case KEYPAD_LEFT:
      selectedDigitCounter--;
      break;
    case KEYPAD_RIGHT:
      selectedDigitCounter++;
      break;
  }

  selectedDigitCounter %= 8;
  
  /*
   * Both of these values have to go negative
   */
  int8_t delta = 0;
  int8_t value = 0;
  
  if (selectedDigitCounter != 0) {
    switch (button) {
      case KEYPAD_UP:
        delta = 1;
        break;
      case KEYPAD_DOWN:
        delta = -1;
        break;
    }
  }
  
  
  switch(selectedDigitCounter) {
    case 0:
      /*
       * Nothing selected
       */
      delta = 0;
      lcd.noBlink();
      break;
    case 1:
      /*
       * Hours
       */
      lcd.setCursor(5, 0);
      lcd.blink();
      value = tm->Hour;
      value += delta;
      if (wildcards) {
        if (value > 24) value = 0;
        if (value < 0) value = 24;
      } else {
        if (value > 23) value = 0;
        if (value < 0) value = 23;
      }
      tm->Hour = value;
      break;
    case 2:
      /*
       * Minutes
       */
      lcd.setCursor(8, 0);
      lcd.blink();
      value = tm->Minute;
      value += delta;
      if (wildcards) {
        if (value > 60) value = 0;
        if (value < 0) value = 60;
      } else {
        if (value > 59) value = 0;
        if (value < 0) value = 59;
      }
      tm->Minute = value;
      break;
    case 3:
      /*
       * Seconds
       */
      lcd.setCursor(11, 0);
      lcd.blink();
      value = tm->Second;
      value += delta;
      if (wildcards) {
        if (value > 60) value = 0;
        if (value < 0) value = 60;
      } else {
        if (value > 59) value = 0;
        if (value < 0) value = 59;
      }
      tm->Second = value;
      break;
    case 4:
      /*
       * Weekday
       */
      if (wildcards) {
        lcd.setCursor(2,1);
        lcd.blink();
        value = tm->Wday;
        value += delta;
        if (value > 7) value = 0;
        if (value < 0) value = 7;
        tm->Wday = value;
      } else {
        selectedDigitCounter++;
      }
      break;
    case 5:
      /*
       * Day of month
       */
      lcd.setCursor(5, 1);
      lcd.blink();
      value = tm->Day;
      value += delta;
      if (wildcards) {
        if (value > 31) value = 0;
        if (value < 0) value = 31;
      } else {
        if (value > 31) value = 1;
        if (value < 1) value = 31;
      }
      tm->Day = value;
      break;
    case 6:
      /*
       * Month
       */
      lcd.setCursor(9, 1);
      lcd.blink();
      value = tm->Month;
      value += delta;
      if (wildcards) {
        if (value > 12) value = 0;
        if (value < 0) value = 12;
      } else {
        if (value > 12) value = 1;
        if (value < 1) value = 12;
      }
      tm->Month = value;
      break;
    case 7:
      /*
       * Year
       */
      lcd.setCursor(14, 1);
      lcd.blink();
      tm->Year += delta;
      break;
  }
  
  /*
   * Something has changed so update the arduino time
   */
  if (delta != 0) {
    return 1;
  }
  return 0;
}


alarm_t alarm_times[QTY_ALARMS];

void setup() {

  lcd.begin(16, 2);
  lcd.clear();
  
  Serial.begin(115200);
  Serial.print("Initialising\n");

  for (uint8_t i=0; i<QTY_ALARMS; i++) {
    alarm_times[i] = retrieveAlarmTime(i);    
  }

  setSyncProvider(requestSync);  //set function to call when sync required
}

uint8_t display = 0;
uint16_t inactivity_loops = 0;

void loop() {

  if(Serial.available()) {
    processSyncMessage();
  }
  
  int button = checkButton();
  
  if (button == KEYPAD_NONE) {
    inactivity_loops++;
  } else {
    inactivity_loops = 0;
  }
  
  if (button == KEYPAD_SELECT) {
    display++;
    display %= QTY_ALARMS + 1; // 1 extra because 0 is the current time display
    selectedDigitCounter = 0; // come out of set mode
  }

  /*
   * Exit from set mode to display mode
   */
  if (selectedDigitCounter && inactivity_loops > SET_MODE_TIMEOUT_LOOPS) {
    selectedDigitCounter = 0;
  }

  /*
   * Reset back to real-time mode
   */
  if (display && inactivity_loops > DISPLAY_TIMEOUT_LOOPS) {
    display = 0;
    selectedDigitCounter = 0; // come out of set mode - should not be necessary
  }
  
  tmElements_t tm;
  breakTime(now(),tm);
  uint8_t updated;
  
  if (display) {
  
    updated = processTimeSetButtons(&alarm_times[display-1].tm,button,1);
    digitalClockDisplay(&alarm_times[display-1].tm,display);
    
    if (updated) {
      saveAlarmTime(alarm_times[display-1],display-1);
    }
  } else {
  
    updated = processTimeSetButtons(&tm,button,0);
    digitalClockDisplay(&tm,0);

    if (updated) {
      setCurrentTime(makeTime(tm));
    }
  }

  for (uint8_t i=0; i<QTY_ALARMS; i++) {
    if (checkAlarm(alarm_times[i],tm)) {
      Serial.print("Alarm: ");
      Serial.print(i);
      Serial.print("\n");
    }
  }

  delay(LOOP_SLEEP);
}
