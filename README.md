AlarmClock
==========

An Arduino + LCD Keypad alarm clock.

The timekeeping is done using the excellent Time Library by Michael
Margolis.  There are a few limitations to running on an Arduino; time zones
and daylight time saving are not implemented!  The time can be synced
automatically from a (preferably Linux) PC with a fairly accurate clock
running the supplied setTime.pl script.  I use a DS1307 real-time clock in
order to allow the arduino to free-wheel when it does not have a serial
connection.  Without this the time has to be set manually every time the
Arduino is reset.

Display
-------

The display on the LCD keypad is similar to this;

        19:25:11   
    Wed 19 Mar 2014

Redrawing the entire screen every second is slow and flickery hence the
display is only redrawn in the digits that need it.

Setting the time
----------------

The time can be set using the front panel buttons or synced from a PC over
serial port by using the setTime.pl perl script in this repository.

The time can be set using the right and left buttons to move a cursor
between fields on the display and the up and down buttons will allow the
value to be incremented or decremented.

The display will revert back to the normal time display (the cursor will
disappear) after a few seconds with no button presses, or when the cursor is
be moved left of the first field (hours) or right of the last field (years).

Alarms
------

The Select button will cycle through 16 alarms, which can be set in a
similar way, except that for every field you can now increment or decrement
until you reach an asterisk wildcard value just before the field rolls over
or rolls under.

This allows you to set alarms in a similar manner to cron.

         7:00:00   
    Wed ** *** ****

The above alarm goes off every Wednesday at 7:00:00

        00:00:00   
    *** 20 *** ****

The above alarm would go off on the 20th of each month at midnight.

The alarms have no effect when they go off apart from printing "Alarm: N"
(where N is an integer) on the serial connection.  Hook your own code in
here!

The alarms are persisted in the eeprom of the AVR microcontroller to survive
power cycles.

Dependencies
------------

Uses this Arduino time library

https://www.pjrc.com/teensy/td_libs_Time.html

I am using a DS1307 I2C clock chip; so you will need the library for it

https://www.pjrc.com/teensy/td_libs_DS1307RTC.html

Uses the LCDKeypad to display the time and read the buttons

http://www.sainsmart.com/sainsmart-1602-lcd-keypad-shield-for-arduino-duemilanove-uno-mega2560-mega1280.html

http://www.dfrobot.com/wiki/index.php?title=Arduino_LCD_KeyPad_Shield_(SKU:_DFR0009)

http://linksprite.com/wiki/index.php5?title=16_X_2_LCD_Keypad_Shield_for_Arduino

http://forum.arduino.cc/index.php?topic=38061.0
