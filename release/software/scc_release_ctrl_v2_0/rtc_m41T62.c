/*
 * rtc_m41T62.c
 *
 *  Created on: Mar 22, 2017
 *      Author: tm
 *      Many thanks to Bob Herlien for sharing OASIS5 bitbang i2c and RTC routines and
 *      for Jeelabs for their M41T62 open source code - both very helpful
 */


#include <msp430.h>
#include <stdint.h>
#include <time.h>
#include "i2c_bitbang.h"
#include "rtc_m41t62.h"
#include "uart.h"


int   rtc_writeRegs(uint8_t addr, uint8_t numRegs, uint8_t *wdat);
int   rtc_readRegs(uint8_t addr, uint8_t numRegs, uint8_t *rdat);
void  rtc_addr_ptr_reset(void);
void rtc_print_alarm(void);

extern void uart_tx_byte(uint8_t txByte);
extern void uart_tx_byte_ascii(uint8_t txByte);
extern void uart_tx_uint_ascii(uint16_t val);
extern void uart_tx_str(uint8_t *dataPtr);
extern void uart_tx_crlf(void);

extern uint16_t  deployFlg;
/*
 *
 Upon initial power-up, the user should set the ST bit to a '1,' then immediately reset the ST
bit to '0.' This provides an additional “kick-start” to the oscillator circuit.

Bit D7 of register 02h (minute register) contains the oscillator fail interrupt enable bit
(OFIE). When the user sets this bit to '1,' any condition which sets the oscillator fail bit (OF)
(see Section 3.11: "Oscillator stop detection") will also generate an interrupt output.

A WRITE to ANY location within the first eight bytes of the clock register (00h-07h),
including the OFIE bit, RS0-RS3 bit, and CB0-CB1 bits will result in an update of the
system clock and a reset of the divider chain. This could result in an inadvertent change of
the current time. These non-clock related bits should be written prior to setting the clock,
and remain unchanged until such time as a new clock time is also written.

 When the calibration circuit is properly employed, accuracy improves to better
than ±2 ppm at 25 °C.

If the oscillator fail (OF) bit is internally set to a '1,' this indicates that the oscillator has
either stopped, or was stopped for some period of time and can be used to judge the
validity of the clock and date data. This bit will be set to '1' any time the oscillator stops.
In the event the OF bit is found to be set to '1' at any time other than the initial power-up,
the STOP bit (ST)

Initial conditions:
Device ST OF OFIE OUT FT AFE SQWE 32KE RS3-1 RS0 Watchdog
M41T62 0  1   0    1  N/A 0   1   N/A   0    1     0

*
 *



alarmSet(DateTime): Sets alarm to desired time. Uses DateTime variable. Basically the same as adjust() but sets the alarm time instead.
alarmRepeat(int): Returns or sets alarm repeat mode
1: once per second
2: once per minute
3: once per hour
4: once per day
5: once per month
6: once per year
alarmEnable(bool): True/False to enable/disable alarm

 */


time_t     now;
struct tm *ts;

//    char       buf[80];
//
//    /* Get the current time */
//    now = time(NULL);
//
//    /* Format and print the time, "ddd yyyy-mm-dd hh:mm:ss zzz" */
//    ts = localtime(&now);
//    strftime(buf, sizeof(buf), "%a %Y-%m-%d %H:%M:%S %Z", ts);
//    puts(buf);
//
//    return 0;
//}

extern void i2c_init(uint8_t i2cSlaveAddr);
extern void i2c_tx(unsigned char, unsigned char *, unsigned char, unsigned char, unsigned char, unsigned char);
extern void i2c_rx(unsigned char, unsigned char *, unsigned char, unsigned char, unsigned char, unsigned char);
extern void usdelay(int);

extern void uart_tx_byte(uint8_t txByte);

extern unsigned char txdata[];   // defd in main.c
extern unsigned char rxdata[];

// This routine is called when power is first applied - assume RTC is not oscillating
void rtc_init(void)
{

/*
 *
  Upon initial power-up, the user should set the ST bit to a '1,' then immediately reset the ST
bit to '0.' This provides an additional “kick-start” to the oscillator circuit.

Bit D7 of register 02h (minute register) contains the oscillator fail interrupt enable bit
(OFIE). When the user sets this bit to '1,' any condition which sets the oscillator fail bit (OF)
(see Section 3.11: "Oscillator stop detection") will also generate an interrupt output.
 *
 */

    /*  The M41T62 "OUT" bit should be set to 1 (HIGH) by default when
     *  used with this library. This keeps the interrupt pin high
     *  even if alarm, watchdog and oscillator bits are not enabled.
     *  Interrupts are active-low. Default OUT bit is 0 so we set it
     *  to 1 here.
     */

    //Addr  D7  D6  D5  D4  D3  D2  D1  D0
    //00h   0.1 seconds     0.01 seconds
    //01h   ST  10Sec        1Sec
    //02h   OFIE 10Min      1Min
    //03h   0   0   10Hr    1Hr              (24-hour format) Hours 00-23
    //04h   RS3 RS2 RS1 RS0 0   DOW          DOW Day of week 01-7  RS0-RS3 = SQW frequency bits
    //05h   0   0   10DOM   1DOM             Day of Month Date 01-31
    //06h   CB1 CB0 0   10M 1MO              Century/month 0-3/01-1
    //07h   10YR            1YR              Year 00-99
    //08h   OUT 0 S Calibration Calibration
    //09h   RB2 BMB4 BMB3 BMB2 BMB1 BMB0 RB1 RB0 Watchdog
    //0Ah   AFE SQWE 0 Al10M Alarm month Al month 01-12
    //0Bh   RPT4 RPT5 AI 10 date Alarm date Al date 01-31
    //0Ch   RPT3 0 AI 10 hour Alarm hour Al hour 00-23
    //0Dh   RPT2 Alarm 10 minutes Alarm minutes Al min 00-59
    //0Eh   RPT1 Alarm 10 seconds Alarm seconds Al sec 00-59
    //0Fh   WDF AF 0 0 0 OF 0 0 Flags

    // powerup - RTC needs to have it's oscillator kickstarted
    // set the ST bit to a '1', then write to a '0'
    txdata[0] = 0x80;
    rtc_writeRegs(0x01, 1, &txdata[0]);           // regaddr=0x01, 1 reg
//    i2c_tx(M41T62_I2CADDR, txdata, 1, ONEBYTEADDR, 0x01, 0x00);   //i2c TX 1 byte to address 0x01
    txdata[0] = 0x00;
//    i2c_tx(M41T62_I2CADDR, txdata, 1, ONEBYTEADDR, 0x01, 0x00);   //i2c TX 1 byte to address 0x01
    rtc_writeRegs(0x01, 1, &txdata[0]);

    //write the OUT bit in the CAL register
    rtc_readRegs(M41T62_CAL, 1, &txdata[0]);
    txdata[0] |= 0x80;      // set bit7

    //uart_tx_byte_ascii(txdata[0]);  // DEBUG

    rtc_writeRegs(M41T62_CAL, 1, &txdata[0]);



#ifdef NOCODE
    uint8_t RTC_M41T62::begin(void) {
      /*  The M41T62 "OUT" bit should be set to 1 (HIGH) by default when
       *  used with this library. This keeps the interrupt pin high
       *  even if alarm, watchdog and oscillator bits are not enabled.
       *  Interrupts are active-low. Default OUT bit is 0 so we set it
       *  to 1 here.
       */
      int currentByte;

      WIRE.beginTransmission(M41T62_ADDRESS);
      WIRE._I2C_WRITE(M41T62_CAL);
      WIRE.endTransmission();
      WIRE.requestFrom(M41T62_ADDRESS, 1);
      currentByte = WIRE._I2C_READ();

      if (bitRead(currentByte,7)){
        bitWrite(currentByte,7,1);
        WIRE.beginTransmission(M41T62_ADDRESS);
        WIRE._I2C_WRITE(M41T62_CAL);
        WIRE._I2C_WRITE(currentByte);
        WIRE.endTransmission();
      }
      return 1;
#endif

}

void rtc_print_all_regs(void)
{
    uint16_t indx;

    rtc_readRegs(M41T62_TNTH_SEC, 16, &rxdata[0]);

    for(indx=0; indx<8; indx++)
    {
        uart_tx_byte_ascii(rxdata[indx]);
        uart_tx_byte(' ');

    }
    uart_tx_crlf();

    for(indx=8; indx<16; indx++)
    {
        uart_tx_byte_ascii(rxdata[indx]);
        uart_tx_byte(' ');

    }
    uart_tx_crlf();

}


void rtc_print_time(void)
{
//    i2c_rx(M41T62_I2CADDR, rxdata, 7, ONEBYTEADDR, 0x01, 0x00);
//    uart_tx_byte(0x30+(rxdata[0]>>4));      // Seconds
//    uart_tx_byte(0x30+(rxdata[0]&0x0f));
    rtc_readRegs(0x00, 8, &rxdata[0]);

    // 17.04.27 04 11:45:24.55
    uart_tx_byte((rxdata[7]/16)+0x30);          // YY
    uart_tx_byte((rxdata[7] & 0x0f)+0x30);
    uart_tx_byte('.');
    uart_tx_byte((rxdata[6]/16)+0x30);          // MM
    uart_tx_byte((rxdata[6] & 0x0f)+0x30);
    uart_tx_byte('.');
    uart_tx_byte((rxdata[5]/16)+0x30);          // DD
    uart_tx_byte((rxdata[5] & 0x0f)+0x30);
    uart_tx_byte(' ');
    //uart_tx_byte((rxdata[4]/16)+0x30);          // DOW  no need to print 10's on 1 to 7
    uart_tx_byte((rxdata[4] & 0x0f)+0x30);
    uart_tx_byte(' ');
    uart_tx_byte((rxdata[3]/16)+0x30);
    uart_tx_byte((rxdata[3] & 0x0f)+0x30);
    uart_tx_byte(':');
    uart_tx_byte((rxdata[2]/16)+0x30);
    uart_tx_byte((rxdata[2] & 0x0f)+0x30);
    uart_tx_byte(':');
    uart_tx_byte((rxdata[1]/16)+0x30);
    uart_tx_byte((rxdata[1] & 0x0f)+0x30);
    uart_tx_byte('.');
    uart_tx_byte((rxdata[0]/16)+0x30);
    uart_tx_byte((rxdata[0] & 0x0f)+0x30);
    //uart_tx_byte(0x0a);
    //uart_tx_byte(0x0d);


}

void rtc_print_alarm(void)
{
#ifdef TESTCODE
    txdata[0] = 0x92;        // AFE=1 month = 12
    txdata[1] = 0x31;        // day
    txdata[2] = 0x23;        // hour
    txdata[3] = 0x59;       // min
    txdata[4] = 0x58;       // sec
    rtc_writeRegs(0x0a, 5, &txdata[0]);

    uart_tx_str("Alarm Set 12.31 23:59:58\r\n");
#endif



    rtc_readRegs(0x0a, 5, &rxdata[0]);

    rxdata[8] = rxdata[0];      // temp store for AFE reading
#ifdef CHANGECODE0427
    if((rxdata[0] & 0x80) == 0x80)
    {
        uart_tx_str("AFE is set\r\n");
    }
    else
    {
        uart_tx_str("AFE is clear\r\n");
    }
#endif

    //uart_tx_str("aLarm:   ");
    rxdata[0] &= 0x1f;                      // Month
    uart_tx_byte((rxdata[0]>>4)+0x30);
    uart_tx_byte((rxdata[0] & 0x0f)+0x30);
    uart_tx_byte('.');

    rxdata[1] &= 0x3f;
    uart_tx_byte((rxdata[1]>>4)+0x30);      // Day
    uart_tx_byte((rxdata[1] & 0x0f)+0x30);
    uart_tx_byte(' ');

    uart_tx_byte(' ');                      // Align with print_time output
    uart_tx_byte(' ');

    rxdata[2] &= 0x3f;                      // Hour
    uart_tx_byte((rxdata[2]>>4)+0x30);
    uart_tx_byte((rxdata[2] & 0x0f)+0x30);
    uart_tx_byte('.');

    rxdata[3] &= 0x7f;                      // Minute
    uart_tx_byte((rxdata[3]>>4)+0x30);
    uart_tx_byte((rxdata[3] & 0x0f)+0x30);
    uart_tx_byte('.');

    rxdata[4] &= 0x7f;
    uart_tx_byte((rxdata[4]>>4)+0x30);
    uart_tx_byte((rxdata[4] & 0x0f)+0x30);


    if((rxdata[8] & 0x80) == 0x80)      // rxdata[0] which is register 0x0a
    {
        uart_tx_str("    AFE set");
    }
    else
    {
        uart_tx_str("    AFE clear");
    }

    //uart_tx_crlf();



    rtc_addr_ptr_reset();

}

//uint16_t  rtc_set_time(struct rtc_time *tm)
int rtc_set_time(uint8_t *buf)
{
#ifdef NOCODE
    // write todays date
    txdata[0] = 0x00;      // SEC
    txdata[1] = 0x44;      // MIN
    txdata[2] = 0x18;      // HR
    txdata[3] = 0x01;      // DOW
    txdata[4] = 0x03;      // Day of Month
    txdata[5] = 0x04;      // Month
    txdata[6] = 0x17;      // Year
#endif

    rtc_writeRegs(M41T62_SEC, 7, &buf[0]);      // write registers starting at 0x01

    return(1);      // fail = 0
}



int rtc_set_alarm(uint8_t *buf)
{

#ifdef NOCODE
    0Ah AFE SQWE 0 Al 10M Alarm month Al month 01-12
#define M41T62_SQWEN_AMO    0x0A // SQW Enable / Alarm Month
#define M41T62_ADOM         0x0B // Alarm Day of Month
#define M41T62_AHRS         0x0C // Alarm Hour
#define M41T62_AMIN         0x0D // Alarm Minutes
#define M41T62_ASEC         0x0E // Alarm Seconds
#define M41T62_FLAGS        0x0F // Flags: WDF | AF | 0 | 0 | 0 | OF | 0 | 0
#endif

#ifdef NOCOCDE

// TODO verify the correctness of the alarm
    buf[0] &= 0x1f;         // Month (D4 D3 D2 D1 D0)  01 to 12
    buf[0] |= 0x80;         // set the Alarm Enable Flag (AFE) and turn off the SQWE

    uart_tx_byte_ascii(buf[0]);     // DEBUG
    uart_tx_byte(' ');

    buf[1] &= 0x1f;         // Day  (RPT4 RPT4 D5 D4 D3 D2 D1 D0) 01 to 31
    uart_tx_byte_ascii(buf[1]);     // DEBUG
    uart_tx_byte(' ');


    buf[2] &= 0x3f;         // Hour (RPT3 0 D5 D4 D3 D2 D1 D0) 00 to 23
    uart_tx_byte_ascii(buf[2]);     // DEBUG
    uart_tx_byte(' ');


    buf[3] &= 0x7f;         // Minute (RPT2 D6 D5 D4 D3 D2 D1 D0) 00 to 59
    uart_tx_byte_ascii(buf[3]);     // DEBUG
    uart_tx_byte(' ');


    buf[4] &= 0x7f;         // Seconds (RPT1 D6 D5 D4 D3 D2 D1 D0) 00 to 59
    uart_tx_byte_ascii(buf[4]);     // DEBUG
    uart_tx_byte(' ');
    uart_tx_crlf();

#endif


#ifdef WORKS
    txdata[0] &= 0x1f;         // Month (D4 D3 D2 D1 D0)  01 to 12
    txdata[0] |= 0x80;         // set the Alarm Enable Flag (AFE) and turn off the SQWE

    uart_tx_byte_ascii(txdata[0]);     // DEBUG
    uart_tx_byte(' ');

    txdata[1] &= 0x3f;         // Day  (RPT4 RPT4 D5 D4 D3 D2 D1 D0) 01 to 31
    uart_tx_byte_ascii(txdata[1]);     // DEBUG
    uart_tx_byte(' ');


    txdata[2] &= 0x3f;         // Hour (RPT3 0 D5 D4 D3 D2 D1 D0) 00 to 23
    uart_tx_byte_ascii(txdata[2]);     // DEBUG
    uart_tx_byte(' ');


    txdata[3] &= 0x7f;         // Minute (RPT2 D6 D5 D4 D3 D2 D1 D0) 00 to 59
    uart_tx_byte_ascii(txdata[3]);     // DEBUG
    uart_tx_byte(' ');


    txdata[4] &= 0x7f;         // Seconds (RPT1 D6 D5 D4 D3 D2 D1 D0) 00 to 59
    uart_tx_byte_ascii(txdata[4]);     // DEBUG
    uart_tx_byte(' ');
    uart_tx_crlf();


//    i2c_tx(M41T62_I2CADDR, txdata, 7, ONEBYTEADDR, 0x01, 0x00);   //i2c TX 1 byte to address 0x01
    rtc_writeRegs(M41T62_SQWEN_AMO, 5, &txdata[0]);   // write registers starting at 0x0a
#endif

    buf[0] &= 0x1f;         // Month (D4 D3 D2 D1 D0)  01 to 12
    buf[0] |= 0x80;         // set the Alarm Enable Flag (AFE) and turn off the SQWE

    buf[1] &= 0x3f;         // Day  (RPT4 RPT4 D5 D4 D3 D2 D1 D0) 01 to 31

    buf[2] &= 0x3f;         // Hour (RPT3 0 D5 D4 D3 D2 D1 D0) 00 to 23

    buf[3] &= 0x7f;         // Minute (RPT2 D6 D5 D4 D3 D2 D1 D0) 00 to 59

    buf[4] &= 0x7f;         // Seconds (RPT1 D6 D5 D4 D3 D2 D1 D0) 00 to 59

    rtc_writeRegs(M41T62_SQWEN_AMO, 5, &buf[0]);   // write registers starting at 0x0a

    // the above writes to the alarm registers will leave the address pointing to the flags register which prohibits the alarm
    rtc_addr_ptr_reset();   // this action performs a read to locate the register pointer to begining

    return(1);      // fail = 0
}

void rtc_addr_ptr_reset(void)
{
  // reset rtc register address pointer to 0 per datasheet note pg23
    txdata[0] = 0x00;
    rtc_writeRegs(M41T62_TNTH_SEC, 1, &txdata[0]);          // THOM TODO:  should this be a read rather than a write?

}

uint8_t rtc_read_alarmFlag(void)
{

//#define M41T62_FLAGS        0x0F // Flags: WDF | AF | 0 | 0 | 0 | OF | 0 | 0
    rtc_readRegs(M41T62_FLAGS, 1, &rxdata[0]);

    // read flag register (0x0f) - AF is bit.6

    return(rxdata[0]);
}

/************************************************************************/
/* Function    : rtc_readRegs                                            */
/* Purpose     : Read One or More RTC Registers                         */
/* Input       : Register address, num registers to read,  ptr to result*/
/* Outputs     : OK or Error number                                     */
/************************************************************************/
int   rtc_readRegs(uint8_t addr, uint8_t numRegs, uint8_t *rdat)
{
    int     i;
    int   ackOK;

    __disable_interrupt();

    i2c_start();                             /* Send start bit           */

    ackOK = i2c_txByte(RTC_ADDR_WR);       /* Send I2C addr for RTC    */

    if (i2c_txByte(addr) != I2C_SUCCESS)   /* Send addr of RTC register*/
        ackOK = I2C_NO_ACK;

    i2c_start();                             /* Restart                  */

    if (i2c_txByte(RTC_ADDR_RD) != I2C_SUCCESS)
        ackOK = I2C_NO_ACK;                 /* Send I2C addr with read bit*/

    for (i = 0; i < numRegs-1; i++)
    {
        rdat[i] = i2c_rxByte();             /* Read the RTC regs        */
        i2c_sendAck();                       /* Ack each byte            */
    }

    rdat[i] = i2c_rxByte();                 /* Read last RTC reg        */
    i2c_sendNack();                          /* NACK it to indicate end  */
    i2c_stop();                              /* Send stop bit            */
    __enable_interrupt();                       // THOM new 27 Apr

    return(ackOK);
}


/************************************************************************/
/* Function    : rtc_writeRegs                                           */
/* Purpose     : Write One or More RTC Registers                        */
/* Input       : Register address, num regs, ptr to data                */
/* Outputs     : OK or Error number                                     */
/************************************************************************/
int   rtc_writeRegs(uint8_t addr, uint8_t numRegs, uint8_t *wdat)
{
    int     i;
    int   ackOK;

    __disable_interrupt();


    i2c_start();                             /* Send start bit           */

    ackOK = i2c_txByte(RTC_ADDR_WR);       /* Send I2C addr for RTC    */

    if (i2c_txByte(addr) != I2C_SUCCESS)   /* Send addr of RTC register*/
        ackOK = I2C_NO_ACK;

    for (i = 0; i < numRegs; i++)           /* Write the RTC regs       */
        if (i2c_txByte(wdat[i]) != I2C_SUCCESS)
            ackOK = I2C_NO_ACK;

    i2c_stop();                              /* Send stop bit            */
    __enable_interrupt();                       // THOM new 27 Apr


    return(ackOK);
}


/************************************************************************/
/* Function    : toBCD                                                  */
/* Purpose     : Convert binary Byte to BCD                             */
/* Input       : Input byte                                             */
/* Outputs     : BCD Result                                             */
/* Comments    : Works for input range of 00-99                         */
/*               Multiples of 100 get stripped, e.g. 167 converts to 0x67*/
/************************************************************************/
uint8_t toBCD(uint8_t inw)
{
    return((((inw/10)%10) << 4) | (inw%10));
}


/************************************************************************/
/* Function    : fromBCD                                                */
/* Purpose     : Convert from BCD to binary                             */
/* Input       : BCD byte                                               */
/* Outputs     : Result                                                 */
/************************************************************************/
uint8_t fromBCD(uint8_t bcd)
{
    return(10*((bcd >> 4) & 0xf) + (bcd & 0xf));
}


