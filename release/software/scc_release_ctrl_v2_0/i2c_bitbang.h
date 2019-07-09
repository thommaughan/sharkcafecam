/*
 * i2c_bitbang.h
 *
 *  Created on: Mar 30, 2017
 *      Author: tm
 */

#ifndef I2C_BITBANG_H_
#define I2C_BITBANG_H_

#include <MSP430.h>
#include <stdint.h>

//#define BYTE  unsigned char
#define UINT32 uint32_t
#define UINT16 uint16_t


//0000  From BobH
//#ifndef I2C_H
//#define I2C_H

//#include <GenericTypeDefs.h>
//#include <oasis.h>

/********************************************/
/*      Macros to bit-bang I2C bus 1        */
/********************************************/

#define I2C_PxSEL       P1SEL
#define I2C_PxSEL2      P1SEL2
#define I2C_PxDIR       P1DIR
#define I2C_PxOUT       P1OUT
#define I2C_PxIN        P1IN

#define SCL             BIT6
#define SDA             BIT7

#define setSCL()        P1OUT |= SCL        //        LATASET = 0x4000
#define clrSCL()        P1OUT &= ~SCL       // LATACLR = 0x4000
#define setSDA()        P1OUT |= SDA        // LATASET = 0x8000
#define clrSDA()        P1OUT &= ~SDA       // LATACLR = 0x8000
#define setSDAIn()      P1DIR &= ~SDA       // P1DIR = 1 means output, =0 is input            // TRISASET = 0x8000
#define setSDAOut()     P1DIR |= SDA        // TRISACLR = 0x8000
#define readSDA()       P1IN &= SDA         // PORTAbits.RA15


/********************************************/
/*      Typedefs and Constants              */
/********************************************/

/* I2C Errors start at -10      */
#define I2C_SUCCESS               0
#define I2C_ERROR                (-10)
#define I2C_BUS_BUSY             (-11)
#define I2C_MASTER_BUS_COLLISION (-12)
#define I2C_RECEIVE_OVERFLOW     (-13)
#define I2C_NO_ACK               (-14)

#define i2c1BusIsBusy()  (I2C1CON & 0x1f)


/********************************************/
/*      Function Prototypes                 */
/********************************************/

void    i2c_start(void);
void    i2c_stop(void);
int     i2c_txByte(uint8_t dataByte);     // was Errno
uint8_t    i2c_rxByte(void);
void    i2c_sendAck(void);
void    i2c_sendNack(void);

//#endif


//0000
#ifdef NOCODE
/********************************************************************************
Module      : I2C_SW
Author      : 05/04/2015, by KienLTb - https://kienltb.wordpress.com/
Description : I2C software using bit-banging.
********************************************************************************/

/*-----------------------------------------------------------------------------*/
/* Macro definitions  */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Local Macro definitions */
/*-----------------------------------------------------------------------------*/
#define I2C_PxSEL       P1SEL
#define I2C_PxSEL2      P1SEL2
#define I2C_PxDIR       P1DIR
#define I2C_PxOUT       P1OUT
#define I2C_PxIN        P1IN

#define SCL             BIT6
#define SDA             BIT7

#define ACK             0x00
#define NACK            0x01

#define TIME_DELAY 100
#define I2C_DELAY() __delay_cycles(TIME_DELAY)
/*-----------------------------------------------------------------------------*/
/* Function prototypes  */
/*-----------------------------------------------------------------------------*/

//NOTE: Need custom read_SCL(), read_SDA(), clear_SCL(), clear_SDA() to compatible Hardware.

unsigned char read_SCL(void); // Set SCL as input and return current level of line, 0 or 1, nomal is 1 because pullup by res
unsigned char read_SDA(void); // Set SDA as input and return current level of line, 0 or 1, nomal is 0 because pull by res

void clear_SCL(void); // Actively drive SCL signal Low
void clear_SDA(void); // Actively drive SDA signal Low

void i2c_bitbang_init(void);
void i2c_bitbang_start(void);
void i2c_bitbang_stop(void);

void i2c_bitbang_writebit(unsigned char bit);
unsigned char i2c_bitbang_readbit(void);

void i2c_bitbang_writebyte(unsigned char Data);
unsigned char i2c_bitbang_readbyte(void);

void i2c_bitbang_writebuf(unsigned char *Data, unsigned char DevideAddr, unsigned char Register, unsigned char nLength);
void i2c_bitbang_readbuf(unsigned char *Buff, unsigned char DevideAddr, unsigned char Register, unsigned char nLength);


#endif

#endif /* I2C_BITBANG_H_ */
