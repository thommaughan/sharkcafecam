/*
 * rtc_m41t62.h
 *
 *  Created on: Mar 22, 2017
 *      Author: tm
 */

#ifndef RTC_M41T62_H_
#define RTC_M41T62_H_

#define M41T62_ADDRESS      0x68 // I2C Address
#define M41T62_I2CADDR      M41T62_ADDRESS
#define RTC_ADDR_WR         (M41T62_I2CADDR<<1)      // 0xd0
#define RTC_ADDR_RD         ((M41T62_I2CADDR<<1) | 0x01)      // 0xd1

/* M41T62 Register Map  (jeelabs)
 * http://www.st.com/st-web-ui/static/active/en/resource/technical/document/datasheet/CD00019860.pdf
 */

#define M41T62_TNTH_SEC     0x00 // 10ths/100ths Seconds
#define M41T62_SEC          0x01 // Seconds
#define M41T62_MIN          0x02 // Minutes
#define M41T62_HRS          0x03 // Hours
#define M41T62_SQWFQ_DOW    0x04 // SQW Frequency / Day of Week
#define M41T62_DAY          0x05 // Day of Month
#define M41T62_MON          0x06 // Month
#define M41T62_YRS          0x07 // Year
#define M41T62_CAL          0x08 // Calibration
#define M41T62_WDOG         0x09 // Watchdog
#define M41T62_SQWEN_AMO    0x0A // SQW Enable / Alarm Month
#define M41T62_ADOM         0x0B // Alarm Day
#define M41T62_AHRS         0x0C // Alarm Hour
#define M41T62_AMIN         0x0D // Alarm Minutes
#define M41T62_ASEC         0x0E // Alarm Seconds
#define M41T62_FLAGS        0x0F // Flags: WDF | AF | 0 | 0 | 0 | OF | 0 | 0

#define SECONDS_PER_DAY             86400L
#define SECONDS_FROM_1970_TO_2000   946684800


// RTC based on the M41T62 chip connected via I2C and the Wire library
enum M41T62SqwPinMode { SqwNONE = 0x00, Sqw1Hz = 0x0F, Sqw2Hz = 0x0E, Sqw4Hz = 0x0D, Sqw8Hz = 0x0C,
    Sqw16Hz = 0x0B, Sqw32Hz = 0x0A, Sqw64Hz = 0x09, Sqw128Hz = 0x08, Sqw256Hz = 0x07, Sqw512Hz = 0x06,
    Sqw1kHz = 0x05, Sqw2kHz = 0x04, Sqw4kHz = 0x03, Sqw8kHz = 0x02, Sqw32kHz = 0x01 };

struct rtc_time {
        int tm_sec;
        int tm_min;
        int tm_hour;
        int tm_mday;
        int tm_mon;
        int tm_year;
        int tm_wday;
        int tm_yday;
        int tm_isdst;
};


// Code by JeeLabs http://news.jeelabs.org/code/
// Released to the public domain! Enjoy!

#ifdef NOCODE

#include <Arduino.h>

class TimeSpan;

// Simple general-purpose date/time class (no TZ / DST / leap second handling!)
class DateTime {
public:
    DateTime (uint32_t t =0);
    DateTime (uint16_t year, uint8_t month, uint8_t day,
                uint8_t hour =0, uint8_t min =0, uint8_t sec =0);
    DateTime (const DateTime& copy);
    DateTime (const char* date, const char* time);
    DateTime (const __FlashStringHelper* date, const __FlashStringHelper* time);
    uint16_t year() const       { return 2000 + yOff; }
    uint8_t month() const       { return m; }
    uint8_t day() const         { return d; }
    uint8_t hour() const        { return hh; }
    uint8_t minute() const      { return mm; }
    uint8_t second() const      { return ss; }
    uint8_t dayOfWeek() const;

    // 32-bit times as seconds since 1/1/2000
    long secondstime() const;
    // 32-bit times as seconds since 1/1/1970
    uint32_t unixtime(void) const;

    DateTime operator+(const TimeSpan& span);
    DateTime operator-(const TimeSpan& span);
    TimeSpan operator-(const DateTime& right);

protected:
    uint8_t yOff, m, d, hh, mm, ss;
};

// Timespan which can represent changes in time with seconds accuracy.
class TimeSpan {
public:
    TimeSpan (int32_t seconds = 0);
    TimeSpan (int16_t days, int8_t hours, int8_t minutes, int8_t seconds);
    TimeSpan (const TimeSpan& copy);
    int16_t days() const         { return _seconds / 86400L; }
    int8_t  hours() const        { return _seconds / 3600 % 24; }
    int8_t  minutes() const      { return _seconds / 60 % 60; }
    int8_t  seconds() const      { return _seconds % 60; }
    int32_t totalseconds() const { return _seconds; }

    TimeSpan operator+(const TimeSpan& right);
    TimeSpan operator-(const TimeSpan& right);

protected:
    int32_t _seconds;
};

// RTC based on the M41T62 chip connected via I2C and the Wire library
enum M41T62SqwPinMode { SqwNONE = 0x00, Sqw1Hz = 0x0F, Sqw2Hz = 0x0E, Sqw4Hz = 0x0D, Sqw8Hz = 0x0C,
    Sqw16Hz = 0x0B, Sqw32Hz = 0x0A, Sqw64Hz = 0x09, Sqw128Hz = 0x08, Sqw256Hz = 0x07, Sqw512Hz = 0x06,
    Sqw1kHz = 0x05, Sqw2kHz = 0x04, Sqw4kHz = 0x03, Sqw8kHz = 0x02, Sqw32kHz = 0x01 };

class RTC_M41T62 {
public:
    static uint8_t begin(void);
    static void adjust(const DateTime& dt);
    static DateTime now();
    static M41T62SqwPinMode readSqwPinMode();
    static void writeSqwPinMode(M41T62SqwPinMode mode);
    void alarmEnable(bool onOff);
    void alarmRepeat(int mode);
    int alarmRepeat();
    static void alarmSet(const DateTime& dt);
    int checkFlags();

    // Functions for testing only:
    static void printAllBits();
    static void printBits(byte myByte);

private:
    static void pointerReset();
};

// RTC using the internal millis() clock, has to be initialized before use
// NOTE: this clock won't be correct once the millis() timer rolls over (>49d?)
class RTC_Millis {
public:
    static void begin(const DateTime& dt) { adjust(dt); }
    static void adjust(const DateTime& dt);
    static DateTime now();

protected:
    static long offset;
};
#endif

#endif // _RTCLIB_H_




