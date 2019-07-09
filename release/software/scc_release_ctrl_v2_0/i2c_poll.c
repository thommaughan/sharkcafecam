/*
 * i2c_poll.c
 *
 *  Created on: Apr 3, 2017
 *      Author: tm
 */


// Basic I2C Driver.
// Runs on MSP432.
// Matt Laubhan (11/15/2015)
//



#include <stdint.h>
#include <msp430.h>
//#include "InputOutput.h"
#ifdef NOCODE

//----------------------------------------
// eUSCI wait until START and address are transmitted
//----------------------------------------
void WaitStart(void)
{
  while ((UCB0CTLW0 & UCTXSTT) != 0);
}

//----------------------------------------
// eUSCI wait until STOP is complete
//----------------------------------------
void WaitStop(void)
{
  while ((UCB0CTLW0 & UCTXSTP) != 0);
}

//----------------------------------------
// eUSCI wait until TX is complete
//----------------------------------------
void WaitTx(void)
{
  while ((UCB0IFG & UCTXIFG0) == 0);
}

//----------------------------------------
// eUSCI wait until RX is complete
//----------------------------------------
void WaitRx(void)
{
  while ((UCB0IFG & UCRXIFG0) == 0);
}

//----------------------------------------
// eUSCI generate start
//----------------------------------------
void GenStart(void)
{
  UCB0CTLW0 |= UCTXSTT;
}

//----------------------------------------
// eUSCI generate stop
//----------------------------------------
void GenStop(void)
{
  UCB0CTLW0 |= UCTXSTP;
}

// No parameters
void InitI2C(void)
{
  //
  // Configure with the following parameters:
  // * 7-bit addresses.
  // * single master environment.
  // * MSP432 is always master.
  // * I2C mode.
  // * Synchronous mode.
  // * SMCLK is source.
  // * Automatically trigger address ACK.
  // * No interrupts.
  //
  UCB0CTLW0 |= UCSWRST;   // hold USCI in reset.
  P1SEL0 |= 0xC0;   // P1.7 (SCL) and P1.6 (SDA) multiplexing.
  P1SEL1 &= ~0xC0;
  UCB0CTLW0 |= (UCMST | UCMODE_3 | UCSYNC | UCSSEL_2);
  UCB0BRW = 400; // Divide SMCLK down by this factor to get SCL.
  UCB0CTLW0 &= ~UCSWRST;   // release USCI reset.
}

//
// Send a byte of data to the I2C target.
// [Note there is no error checking here.]
//
void OutI2C(unsigned char address, unsigned char data)
{
  UCB0I2CSA = address; // UCB0I2CSA = <expander addr w/o R/W bit>.
  UCB0CTLW0 |= UCTR;   // TX.
  GenStart();          // Generate a START.
  WaitStart();         // Wait for START and slave address to be sent.
  WaitTx();            // Wait until it is safe to write to TX buffer.
  UCB0TXBUF = data;    // UCB0TXBUF = <data> (prep data).
  if ((UCB0IFG & UCNACKIFG) == 0) // Check for slave NACK.
  {
    // I2C slave responded.
    WaitTx();          // Wait until data is transferred.
  }
  else
  {
    // I2C slave did not respond.
  }
  GenStop();           // Generate a STOP.
  WaitStop();          // Wait for STOP to be sent/ready to go again.
}

//
// Receive a byte of data from the I2C target.
// [Note there is no error checking here.]
//
unsigned char InI2C(unsigned char address)
{
  unsigned char retVal;

  UCB0I2CSA = address; // UCB0I2CSA = <expander addr w/o R/W bit>.
  UCB0CTLW0 &= ~UCTR;  // RX.
  GenStart();          // Generate a START.
  WaitStart();         // Wait for START and slave address to be sent.
  if ((UCB0IFG & UCNACKIFG) == 0) // Check for slave NACK.
  {
    // I2C slave responded.  Get data.
    WaitRx();            // Wait for data.
    retVal = UCB0RXBUF;  // Read UCB0RXBUF for data.
  }
  else
  {
    // I2C slave did not respond.
  }
  GenStop();           // Generate a STOP.
  WaitStop();          // Wait for STOP to be sent/ready to go again.

  return retVal;
}

int main(void)
{
  // Lookup table (common-anode)
  const unsigned char sevSegTable[16] =
  {
    0x40, // 0
    0x79, // 1
    0x24, // 2
    0x30, // 3
    0x19, // 4
    0x12, // 5
    0x02, // 6
    0x78, // 7
    0x00, // 8
    0x18, // 9
    0x08, // A
    0x03, // B
    0x46, // C
    0x21, // D
    0x06, // E
    0x0E  // F
  };


  // Ex:  address = I2C_8574 | I2C_0
  // Ex:  address = I2C_8574_A | I2C_3
  const unsigned char switchAddress = I2C_8574 | I2C_0;
  const unsigned char sevenSegLedAddress = I2C_8574_A | I2C_3;
  unsigned char data;

  InitI2C();
  while (1)
  {
    data = InI2C(switchAddress);
    data = sevSegTable[data & 0xF];
    OutI2C(sevenSegLedAddress, data);
  }

  // this will never be hit (compiler will probably warn)
  //return 0;
}
#endif
