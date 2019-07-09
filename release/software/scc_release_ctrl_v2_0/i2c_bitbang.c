/*
 * i2c_bitbang.c
 *
 *  Created on: Mar 30, 2017
 *      Author: tm
 */
#include "i2c_bitbang.h"

extern void delay_usec(uint16_t microSec);

/* Following defines set up I2C clock, nominally for 500 KHz, but       */
/* execution delays actually slow it down to about 300 KHz (measured).  */
/* With logic analyzer, I see a min clk hi time of 1.3us, and low 2.0us */
/* Mins from data sheet are 0.6us and 1.3us, respectively.              */

//#define CLK_TIME        SYSCLK/2000000      /* Clk hi or low = 1us      */
//#define HALFCLK_TIME    CLK_TIME/2          /* Sample data at mid-clk = .5us*/

//#define delayClkTime(stCnt) stCnt = _CP0_GET_COUNT();   \
                            while ((_CP0_GET_COUNT() - stCnt) < CLK_TIME) ;
//#define delayHalfClkTime(stCnt) stCnt = _CP0_GET_COUNT();   \
                                while ((_CP0_GET_COUNT() - stCnt) < HALFCLK_TIME) ;

#define I2C_BIT_TIME_USEC   10          // 100Khz is 10 usec
#define I2C_FULLBIT_TIME    I2C_BIT_TIME_USEC
#define I2C_HALFBIT_TIME    (I2C_BIT_TIME_USEC/2)


//void i2c_init(uint8_t i2cSlaveAddr)
void i2c_init(void)
{

    P1DIR |= SDA | SCL;                     // Set SDA and SCL as GPIO outputs
    P1OUT |= SDA | SCL;
    P1SEL &= ~(BIT6 | BIT7);
    P1SEL2 &= ~(BIT6 | BIT7);


#ifdef NOCODE
    // Done in main.c for now
    P1SEL |= BIT6 | BIT7;                   //Set Port1 Special Function as I2C pins
    P1SEL2 |= BIT6 | BIT7;

    UCB0CTL1 |= UCSWRST;                    //Enable SW reset to config I2C
    // configure i2c
    UCB0CTL0 = UCMST + UCMODE_3 + UCSYNC;   //I2C Master, synchronous mode
    //UCB0CTL0 = UCMST | UCMODE_3 | UCSYNC;   //I2C Master, synchronous mode
    UCB0CTL1 = UCSSEL_2 | UCSWRST;          //Use SMCLK, keep SW reset
    UCB0BR0 = 10;                           //fSCL = SMCLK/10 = ~100kHz
    UCB0BR1 = 0;
    UCB0I2CSA = i2cSlaveAddr;                         // Set slave address                                        // i2cSlaveAddr = 0x20 for PCF8574

    //IFG2 &= ~UCB0TXIFG;                     // clear interrupt flags (this was not done in the PCF8574 example code)
    //IFG2 &= ~UCB0RXIFG;

    UCB0CTL1 &= ~UCSWRST;                   //Clear SW reset, resume operation

    //IE2 |= UCB0RXIE;                          // Enable RX interrupt PCF8574 demo code
    IE2 |= UCB0TXIE | UCB0RXIE;             //Enable TX, RX interrupt
    UCB0I2CIE = UCNACKIE;                   // NACK interrupts
#endif

}

/************************************************************************/
/* Function    : i2c_start                                               */
/* Purpose     : Send I2C Start bit                                     */
/* Input       : None                                                   */
/* Outputs     : None                                                   */
/************************************************************************/
void i2c_start(void)
{
    setSDAOut();
    setSCL();
    setSDA();
    __delay_cycles(2);       // delay_usec(2);
    clrSDA();
    __delay_cycles(2);      //delay_usec(2);
    clrSCL();
    __delay_cycles(2);      //    delay_usec(2);
}


/************************************************************************/
/* Function    : i2c_stop                                                */
/* Purpose     : Send I2C Stop bit                                      */
/* Input       : None                                                   */
/* Outputs     : SUCCESS or I2C error code                              */
/************************************************************************/
void i2c_stop(void)
{
    setSDAOut();
    clrSDA();
    __delay_cycles(2);      //    delay_usec(2);
    setSCL();
    __delay_cycles(2);      //    delay_usec(2);
    setSDA();
    __delay_cycles(2);      //    delay_usec(2);
}


/************************************************************************/
/* Function    : i2c_txByte                                            */
/* Purpose     : Send Byte on I2C Bus 1                                 */
/* Input       : Byte to send                                           */
/* Outputs     : I2C_SUCCESS or I2C_NO_ACK                              */
/************************************************************************/
int i2c_txByte(uint8_t dataByte)          // was Errno
{
//    UINT32      stCnt = _CP0_GET_COUNT();
    UINT16      i, sndByte;
    int         rtn;

    for (i = 0, sndByte = dataByte; i < 8; i++)
    {
        __delay_cycles(I2C_HALFBIT_TIME);       //         delayHalfClkTime(stCnt);
        //__delay_cycles(2);      //
        if (sndByte & 0x80)
        {
            setSDA();                   /* Send next bit                */
        }
        else
        {
            clrSDA();
        }

        __delay_cycles(I2C_HALFBIT_TIME);       //        delayHalfClkTime(stCnt);        /* Data Setup time              */
        setSCL();                       /* Clock high                   */
        __delay_cycles(I2C_FULLBIT_TIME);       //        delayClkTime(stCnt);            /* Data Hold time               */
        clrSCL();                       /* Clock low                    */
        sndByte <<= 1;
    }

    __delay_cycles(I2C_HALFBIT_TIME);       //    delayHalfClkTime(stCnt);            /* Data Hold time               */
    setSDAIn();
    __delay_cycles(I2C_HALFBIT_TIME);       //    delayHalfClkTime(stCnt);
    setSCL();                           /* Clock high                   */
    __delay_cycles(I2C_HALFBIT_TIME);       //    delayHalfClkTime(stCnt);
    rtn = readSDA() ? I2C_NO_ACK : I2C_SUCCESS;     /* Get ACK          */
    // THOM - fix rtn - it's a bit mask BIT6 - not sure how 'TRUE' BIT6=1
    __delay_cycles(I2C_HALFBIT_TIME);       //    delayHalfClkTime(stCnt);
    clrSCL();                           /* Clock low                    */
    clrSDA();                           /* Data pin low                 */
    setSDAOut();

    return(rtn);
}



/************************************************************************/
/* Function    : i2c_rxByte                                             */
/* Purpose     : Receive a Byte from I2C Bus 1                          */
/* Input       : Ptr to where to put byte                               */
/* Outputs     : Received data byte                                     */
/************************************************************************/
uint8_t    i2c_rxByte(void)
{

    UINT16      i;
    uint8_t     rcvByte;

    setSDAIn();

    for (i = rcvByte = 0; i < 8; i++)
    {
        rcvByte <<= 1;                      // Shift read data byte
        __delay_cycles(I2C_FULLBIT_TIME);   //        delayClkTime(stCnt);
        setSCL();                           // Clock high
        __delay_cycles(I2C_HALFBIT_TIME);   // delayHalfClkTime(stCnt);   Data setup time
        if (readSDA())                      // Read data bit
            rcvByte |= 1;
        __delay_cycles(I2C_HALFBIT_TIME);   //        delayHalfClkTime(stCnt);
        clrSCL();                           // Clock low
    }

    return(rcvByte);
}


/************************************************************************/
/* Function    : i2c_sendAck                                             */
/* Purpose     : Send I2C ACK bit                                       */
/* Input       : None                                                   */
/* Outputs     : SUCCESS or I2C error code                              */
/************************************************************************/
void i2c_sendAck(void)
{
    clrSDA();
    _NOP();
    setSDAOut();
    __delay_cycles(2);
    setSCL();               // Clock high
    __delay_cycles(2);
    clrSCL();               // Clock low
}


/************************************************************************/
/* Function    : i2c_sendNack                                            */
/* Purpose     : Send I2C NACK bit                                      */
/* Input       : None                                                   */
/* Outputs     : SUCCESS or I2C error code                              */
/************************************************************************/
void i2c_sendNack(void)
{
    setSDA();
    _NOP();
    setSDAOut();
    __delay_cycles(2);
    setSCL();               // Clock high
    __delay_cycles(2);      //
    clrSCL();               // Clock low
}








