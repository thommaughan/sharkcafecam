/*
 * serial.c
 *
 *  Created on: Mar 22, 2017
 *      Author: tm
 */

#include <msp430.h>
#include <stdint.h>
#include "uart.h"


//#define SIZEOF_RXBUF    20
//#define SIZEOF_TXBUF    20

uint8_t     rxBuf[SIZEOF_RXBUF+1];      // size defd in uart.h
uint16_t    rxNqIndx;
uint16_t    rxDqIndx;
uint8_t     txBuf[SIZEOF_TXBUF+1];
uint16_t    txNqIndx;
uint16_t    txDqIndx;

extern volatile uint32_t wdtCounter;

//void serial_setup (unsigned out_mask, unsigned in_mask, unsigned duration)  for bit banging

void uart_init (unsigned long baud);
void uart_tx_byte(uint8_t txByte);
void uart_tx_byte_ascii(uint8_t txByte);
void uart_tx_uint_ascii(uint16_t val);
void uart_tx_str(uint8_t *dataPtr);
int uart_rx_rdy(void);
uint8_t uart_rx_byte(void);
void uart_tx_crlf(void);
/************************************************************************/
/* Function    :                                                        */
/* Purpose     :                                                        */
/* Input       :                                                        */
/* Outputs     :                                                        */
/************************************************************************/
void uart_init (unsigned long baud)
{
    P1SEL = BIT1 + BIT2 ;                     // P1.1 = RXD, P1.2=TXD
    P1SEL2 = BIT1 + BIT2;

    // UART
    //UCA0CTL1 |= UCSSEL_2;                     // SMCLK
    //UCA0BR0 = 8;                              // 1MHz 115200
    //UCA0BR1 = 0;                              // 1MHz 115200
    //UCA0MCTL = UCBRS2 + UCBRS0;               // Modulation UCBRSx = 5
    //UCA0CTL1 &= ~UCSWRST;                     // **Initialize USCI state machine**
    //IE2 |= UCA0RXIE;                          // Enable USCI_A0 RX interrupt

    // init nq dq vars
    rxNqIndx = 0;
    rxDqIndx = 0;
    txNqIndx = 0;
    txDqIndx = 0;


    //------------ initialize the UART(USCI_A0) (assumes port bits config'd in main)----------------//
     UCA0CTL1 |=  UCSSEL_2 + UCSWRST;  // USCI Clock = SMCLK,USCI_A0 disabled

     // default will be 4800 for configured HC-12 RF module compatibility with low power FU2 mode
     UCA0BR0 = 208;                            // 1MHz 4800
     UCA0BR1 = 0;                              // 1MHz
     UCA0MCTL = UCBRS1 + UCBRS0;               // Modulation UCBRSx = 3

     //UCA0BR0   =  104;                 // 104 From datasheet table-
     //UCA0BR1   =  0;                   // -selects baudrate =9600,clk = SMCLK
     //UCA0MCTL  =  UCBRS_1;             // Modulation value = 1 from datasheet
     //For fBRCLK=1MHz, BR=1200: N=1000000/1200 = 833.33333333333
     //UCBRx = INT(N) = 833
     //UCBRSx = round (0,33333333 * 8) = round (2,6666666) = 3
     if(baud == 1200L)
     {
         // 833 -> 0x0341
         UCA0BR0 = 0x41;                           // 65 1MHz 1200
         UCA0BR1 = 0x03;                           // 1MHz
         UCA0MCTL = UCBRS1 + UCBRS0;               // Modulation UCBRSx = 3
     }

     //For fBRCLK=1MHz, BR=2400: N=1000000/2400 = 416.66666666
     //UCBRx = INT(N) = 416
     //UCBRSx = round (0,66666666 * 8) = round (5,33333333) = 5
     if(baud == 2400L)
     {
         // 416 -> 0x01A0
         UCA0BR0 = 0xA0;                              // 1MHz 2400
         UCA0BR1 = 0x01;                              // 1MHz
         UCA0MCTL = UCBRS2 + UCBRS0;               // Modulation UCBRSx = 5
     }

     //For fBRCLK=1MHz, BR=4800: N=1000000/4800 = 208.33333333333
     //UCBRx = INT(N) = 208
     //UCBRSx = round (0,33333333 * 8) = round (2,6666666) = 3
     if(baud == 4800L)
     {
         UCA0BR0 = 208;                            // 1MHz 4800
         UCA0BR1 = 0;                              // 1MHz
         UCA0MCTL = UCBRS1 + UCBRS0;               // Modulation UCBRSx = 3
     }
     //For fBRCLK=1MHz, BR=9600: N=1000000/9600 = 104,16666667
     //UCBRx = INT(N) = 104
     //UCBRSx = round (0,16666667 * 8) = round (1,33333333) = 1
     if(baud == 9600L)
     {
         UCA0BR0   =  104;                 // 1MHz 9600
         UCA0BR1   =  0;                   //
         UCA0MCTL  =  UCBRS_1;             // Modulation UCBRSx = 1
     }
     //For fBRCLK=1MHz, BR=19200: N=1000000/19200 = 52.083333333333
     //UCBRx = INT(N) = 52
     //UCBRSx = round (0.083333333333 * 8) = round (0.666666) = 1
     if(baud == 19200L)
     {
         UCA0BR0   =  52;                 // 1MHz 19200
         UCA0BR1   =  0;                   //
         UCA0MCTL  =  UCBRS_1;             // Modulation UCBRSx = 1
     }
     //For fBRCLK=1MHz, BR=38400: N=1000000/38400 = 26,0416666666666666666667
     //UCBRx = INT(N) = 26
     //UCBRSx = round (0,041666666666666666666666666667 * 8) = round (0,33333333333333333333333333333333) = 0
     if(baud == 38400L)
     {
         UCA0BR0   =  26;                 // 1MHz 38400
         UCA0BR1   =  0;                   //
         UCA0MCTL  =  0;                // Modulation UCBRSx = 0
     }
     //For fBRCLK=1MHz, BR=57600: N=1000000/57600 = 17,361111111111111111111111111111
     //UCBRx = INT(N) = 17
     //UCBRSx = round (0,361111111111111111111111111111 * 8) = round (2,8888888888888888888888888888889) = 3
     if(baud == 57600L)
     {
         UCA0BR0 = 17;                              // 1MHz 57600
         UCA0BR1 = 0;                              //
         UCA0MCTL = UCBRS1 + UCBRS0;               // Modulation UCBRSx = 3
     }

     //For fBRCLK=1MHz, BR=115200: N=1000000/115200 = 8,6805555555555555555555555555556
     //UCBRx = INT(N) = 8
     //UCBRSx = round (0,6805555555555555555555555555556 * 8) = round (5,4444444444444444444444444444444) = 5
     if(baud == 115200L)
     {
         UCA0BR0 = 8;                              // 1MHz 115200
         UCA0BR1 = 0;                              // 1MHz 11520
         UCA0MCTL = UCBRS2 + UCBRS0;               // Modulation UCBRSx = 5
     }

     // NOTE: comment the loopback out...
     //UCA0STAT |=  UCLISTEN;            // loop back mode enabled
     UCA0CTL1 &= ~UCSWRST;             // Clear UCSWRST to enable USCI_A0

    //---------------- Enabling the uart interrupts ----------------//

//     IE2 |= UCA0TXIE;                  // Enable the Transmit interrupt
     IE2 |= UCA0RXIE;                  // Enable the Receive  interrupt

}

void uart_disable_interrupt(void)
{
    IE2 &= ~UCA0RXIE;                  // Disable the Receive  interrupt
}


void uart_enable_interrupt(void)
{
    // first clear any pending
    IFG2 &= ~UCA0RXIFG; // Clear RX flag

    IE2 |= UCA0RXIE;                  // Enable the Receive  interrupt
}

void uart_passivate(void)
{

}

void uart_activate(void)
{

}

/************************************************************************/
/* Function    :                                                        */
/* Purpose     :                                                        */
/* Input       :                                                        */
/* Outputs     :                                                        */
/************************************************************************/
void uart_tx_byte(uint8_t txByte)
{
    while (!(IFG2 & UCA0TXIFG))
        ;

    UCA0TXBUF = txByte;
}

/************************************************************************/
/* Function    :                                                        */
/* Purpose     :                                                        */
/* Input       :                                                        */
/* Outputs     :                                                        */
/************************************************************************/
void uart_tx_byte_ascii(uint8_t txByte)
{
    uart_tx_byte((txByte/16)+0x30);
    uart_tx_byte((txByte&0x0f)+0x30);

}

/************************************************************************/
/* Function    :                                                        */
/* Purpose     :                                                        */
/* Input       :                                                        */
/* Outputs     :                                                        */
/************************************************************************/
void uart_tx_uint_ascii(uint16_t val)
{
    uint8_t bVal, supZer;

    //val = 0;
    supZer = 0;

    bVal = (uint8_t)((val/10000));
    val -= bVal*10000;
    if(bVal != 0)
        supZer = 1;
    if(supZer)
        uart_tx_byte(bVal+0x30);

    bVal = (uint8_t)((val/1000));
    val -= bVal*1000;
    if(bVal != 0)
        supZer = 1;
    if(supZer)
        uart_tx_byte(bVal+0x30);


    bVal = (uint8_t)((val/100));
    val -= bVal*100;
    if(bVal != 0)
        supZer = 1;
    if(supZer)
        uart_tx_byte(bVal+0x30);

    bVal = (uint8_t)((val/10));
    val -= bVal*10;
    if(bVal != 0)
        supZer = 1;
    if(supZer)
        uart_tx_byte(bVal+0x30);


    bVal = (uint8_t)((val));
    uart_tx_byte(bVal+0x30);

#ifdef NOCODE

    bVal = (uint8_t)(val>>8);

    uart_tx_byte((bVal / 16) + 0x30);
    uart_tx_byte((bVal & 0x0f) + 0x30);

    bVal = (uint8_t)(val);
    uart_tx_byte((bVal / 16) + 0x30);
    uart_tx_byte((bVal & 0x0f) + 0x30);

    unsigned digit, tenth;

    tenth = val / 10;
    digit = val - 10 * tenth;
    if (tenth != 0)
    {
    s = sput_u(tenth, s);
    }
    *s = (char)(digit + '0');
    return s + 1;
#endif

}
/************************************************************************/
/* Function    :                                                        */
/* Purpose     :                                                        */
/* Input       :                                                        */
/* Outputs     :                                                        */
/************************************************************************/
void uart_tx_crlf(void)
{
    uart_tx_byte(0x0a);
    uart_tx_byte(0x0d);
}
/************************************************************************/
/* Function    :                                                        */
/* Purpose     :                                                        */
/* Input       :                                                        */
/* Outputs     :                                                        */
/************************************************************************/
void uart_tx_nq(uint8_t txByte)
{
    if(txNqIndx == rxNqIndx)
        UCA0TXBUF = txByte;             // prime the tx pump by loading first byte
    txBuf[txNqIndx++] = txByte;
    if(txNqIndx >= SIZEOF_TXBUF)
        txNqIndx = 0;
}

/************************************************************************/
/* Function    :                                                        */
/* Purpose     :                                                        */
/* Input       :                                                        */
/* Outputs     :                                                        */
/************************************************************************/
void uart_tx_str(uint8_t *dataPtr)
{
    while(*dataPtr)
    {
        //uart_tx_nq(*dataPtr);
        uart_tx_byte(*dataPtr);
        dataPtr++;
    }
}






/************************************************************************/
/* Function    :                                                        */
/* Purpose     :                                                        */
/* Input       :                                                        */
/* Outputs     :                                                        */
/************************************************************************/
int uart_rx_rdy(void)
{
    if(rxNqIndx != rxDqIndx)
        return 1;
    else
        return 0;
}

/************************************************************************/
/* Function    :                                                        */
/* Purpose     :                                                        */
/* Input       :                                                        */
/* Outputs     :                                                        */
/************************************************************************/
uint8_t uart_rx_byte(void)
{
    uint8_t rxByte;

    rxByte = rxBuf[rxDqIndx++];
    if(rxDqIndx >= SIZEOF_RXBUF)
        rxDqIndx = 0;
    return(rxByte);
}

/************************************************************************/
/* Function    :                                                        */
/* Purpose     :                                                        */
/* Input       :                                                        */
/* Outputs     :                                                        */
/************************************************************************/
void print(char *data)
{
    while(*data)
    {
        uart_tx_byte(*data);
        data++;
    }
}
/************************************************************************/
/* Function    :                                                        */
/* Purpose     :                                                        */
/* Input       :                                                        */
/* Outputs     :                                                        */
/************************************************************************/
void println(char *data)
{
    while(*data)
    {
        uart_tx_byte(*data);
        data++;
    }
    uart_tx_byte('\n');
    uart_tx_byte('\r');
}

#ifdef NOCODE

/************************************************************************/
/* Function    :                                                        */
/* Purpose     :                                                        */
/* Input       :                                                        */
/* Outputs     :                                                        */
/************************************************************************/
void serial_putc (unsigned txByte)
{

}
/************************************************************************/
/* Function    :                                                        */
/* Purpose     :                                                        */
/* Input       :                                                        */
/* Outputs     :                                                        */
/************************************************************************/
void serial_puts (char *txString)
{

}
unsigned serial_getc (void)
{


}
#endif

//-----------------------------------------------------------------------//
//                Transmit and Receive interrupts                        //
//-----------------------------------------------------------------------//
//#ifdef NOCODE
#pragma vector = USCIAB0TX_VECTOR
__interrupt void TransmitInterrupt(void)
{
    //P1OUT  ^= BIT0;//light up P1.0 Led on Tx Launchpad (HARV_EN_LO)
//    UCA0TXBUF = 'A';
#ifdef NOCODE
  if(txDqIndx != txNqIndx)
  {
      P1OUT  ^= BIT0;//light up P1.0 Led on Tx Launchpad (HARV_EN_LO)

      UCA0TXBUF = txBuf[txDqIndx++];
      if(txDqIndx >= SIZEOF_TXBUF)
          txDqIndx = 0;
  }
#endif
}
//#endif

//#ifdef NOCODE
#pragma vector = USCIAB0RX_VECTOR
__interrupt void ReceiveInterrupt(void)
{
//    _BIC_SR(LPM4_EXIT); // wake up from low power mode  (THOM changed from LPM3 exit)
	_BIC_SR(LPM3_EXIT); // wake up from low power mode  (THOM changed from LPM3 exit)

  //P1OUT  ^= BIT6;     // light up P1.6 LED on RX
  IFG2 &= ~UCA0RXIFG; // Clear RX flag

  wdtCounter = 0L;          // reset watchdog 'go to sleep' counter

//#ifdef NOCODE
  rxBuf[rxNqIndx] = UCA0RXBUF;
  //UCA0TXBUF = rxBuf[rxNqIndx];
  rxNqIndx++;
  if(rxNqIndx >= SIZEOF_RXBUF)
      rxNqIndx = 0;
//#endif

}
//#endif


