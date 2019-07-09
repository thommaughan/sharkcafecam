/*
 * Author: tm
 *
 * Credits: a number of folks on e2e, but special thanks to Ahmed Talaat (aa_talaat@yahoo.com) for isr and tx/rx
 *
 */


#include <stdint.h>
#include "msp430g2553.h"
#include "uart.h"
#include "i2c.h"



#ifdef NOCODE

// i2c states
//#define WRITE	0x00
//#define READ	0x01
//#define ADDR16	0x02
//#define ADDRTR	0x04
//#define REPSTT	0x08

extern uint8_t     rxBuf[SIZEOF_RXBUF+1];
extern uint16_t    rxNqIndx;
extern uint16_t    rxDqIndx;
extern uint8_t     txBuf[SIZEOF_TXBUF+1];
extern uint16_t    txNqIndx;
extern uint16_t    txDqIndx;


void i2c_init(uint8_t i2cSlaveAddr);
void i2c_tx(unsigned char, unsigned char *, unsigned char, unsigned char, unsigned char, unsigned char);
void i2c_rx(unsigned char, unsigned char *, unsigned char, unsigned char, unsigned char, unsigned char);
void delay_usec(uint16_t microSec);
//i2c_data_t i2c_data = {0, 0, 0, 0};

typedef struct {
    volatile unsigned char *data_buf;       // address of tx or rx data buffer
    volatile unsigned char buf_size;        // size of the buffer
    volatile unsigned char buf_index;       // index in the buffer
    volatile unsigned char addr_index;      // index of the byte address (0,1)
    volatile unsigned char isr_mode;        // Tx or Rx affects the interrupt logic
    volatile unsigned char addr_high_byte;  // High byte of the address to read/write to
    volatile unsigned char addr_low_byte;   // Low byte of the address to read/write to
    volatile unsigned char addr_type;       // two bytes like eeprom or 1 byte like RTC for example
} i2c_t;

i2c_t   i2c_packet;






u16 swap_bytes(u16 in)
{
    u16 rv;

    u8 hibyte = (in & 0xff00) >> 8;
    u8 lobyte = (in & 0xff);

	rv = lobyte << 8 | hibyte;
	return rv;
}

void i2c_init(uint8_t i2cSlaveAddr)
{
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

}


/*
* This functions writes to any I2C device. It takes as parameters:
*  - The slave address (7 bits)
*  - Pointer to the data buffer to transmit
*  - Size of the buffer
*  - Size of the address location to start writing at. Being 2 bytes for some of the EEPROMs and single byte
*    for other devices. Please note that the ISR of this function assumes that the address bytes are written high
*    byte first, then low byte second.
*    In case of single byte address device, only the high byte will be used to set the address.
*  - The high byte and low byte address for the location to start writing at.
*
*  In case the start condition of the write operation is not ack (for example EEPROM busy with a previous write cycle),
*  the corresponding interrupts detects this condition, generates a stop signal, and a Timer A1 (not Timer A0)
*  is activated for 0.5 ms, then the trial for writing is repeated.
*
*  Please note that this function does not cater for the EEPROM 128byte paging requirements. So, if you are going
*  to write more than 128 bytes to an EEPROM, you will need to write a higher level function to segment the
*  writing into consecutive 128byte chunks.
*
*/

void i2c_tx(unsigned char slave_addr, unsigned char *txdata, unsigned char bufSize, unsigned char addr_size,
                    unsigned char high_byte_addr, unsigned char low_byte_addr)
{
    i2c_packet.isr_mode=WRITE;
    i2c_packet.data_buf=txdata;
    i2c_packet.buf_size=bufSize;
    i2c_packet.buf_index=0;
    i2c_packet.addr_type=addr_size;
    i2c_packet.addr_high_byte=high_byte_addr;
    i2c_packet.addr_low_byte=low_byte_addr;
    i2c_packet.addr_index=0;
    UCB0I2CSA = slave_addr;                 //Slave Address

    while (1)
    {
        UCB0CTL1 |= UCTR + UCTXSTT;         // I2C TX, start condition
        LPM0;                               // Enter LPM0
                                            // and remain until all data is TX'd
        if (i2c_packet.isr_mode == NOACK)  // If no ack received, then sleep for 0.5ms and try again
        {
            i2c_packet.isr_mode = WRITE;
            i2c_packet.addr_index=0;        // Reset the address index for the next write operation
            delay_usec(500);

        } else
        {
            break;                          // Successful write, then quit
        }
    }
}

/*
 * This functions reads from any I2C device. It takes as parameters:
 *  - The slave address (7 bits)
 *  - Pointer to the data buffer to fill with data read.
 *  - Size of the buffer
 *  - Size of the address location to start writing at. Being 2 bytes for some of the EEPROMs and single byte
 *    for other devices. Please note that the ISR of this function assumes that the address bytes are written high
 *    byte first, then low byte second.
 *    In case of single byte address device, only the high byte will be used to set the address.
 *  - The high byte and low byte address for the location to start reading at.
 *
 *  The function starts with a write operation to specify the address at which the read operation with start
 *  In case the start condition of the write operation is not ack (for example EEPROM busy with a a previous write cycle),
 *  the corresponding interrupts detects this condition, generates a stop signal, and a Timer A1 (not Timer A0)
 *  is activated for 0.5 ms, then the trial for writing is repeated.
 *
 *  Once the write address is successful, the functions switch to read mode, and fills the buffer provided
 *
 */


void i2c_rx(unsigned char slave_addr, unsigned char *rxdata, unsigned char bufSize, unsigned char addr_size,
        unsigned char high_byte_addr, unsigned char low_byte_addr)
{
    i2c_packet.isr_mode=READ;               // The ISR will send the address bytes, then wake CPU.
    i2c_packet.addr_type=addr_size;
    i2c_packet.addr_high_byte=high_byte_addr;
    i2c_packet.addr_low_byte=low_byte_addr;
    i2c_packet.addr_index=0;
    UCB0I2CSA = slave_addr;                 // Slave Address

    while (1)
    {
        UCB0CTL1 |= UCTR + UCTXSTT;         // I2C TX, start condition
        LPM0;                               // Enter LPM0
                                            // and remain until all data is TX'd
        if (i2c_packet.isr_mode == NOACK){  // If no ack received, then sleep for 0.5ms and try again
            i2c_packet.isr_mode = READ;
            i2c_packet.addr_index=0;        // Reset the address index for the next write operation
            delay_usec(500);
        } else
        {
            break;                          // Successful write, then quit
        }
    }
                                            // We wrote already the address, so now read only data.
    i2c_packet.addr_index=i2c_packet.addr_type;
    i2c_packet.data_buf=rxdata;
    i2c_packet.buf_size=bufSize;
    i2c_packet.buf_index=0;
    UCB0CTL1 &= ~UCTR;                      // I2C RX
    UCB0CTL1 |= UCTXSTT;                    // I2C re-start condition
    LPM0;                                   // Enter LPM0
                                            // and remain until all data is received
}


//interrupt(USCIAB0RX_VECTOR) state change to trap the no_Ack from slave case
#pragma vector = USCIAB0RX_VECTOR
__interrupt void USCIAB0RX_ISR(void)
{

/*
       USCIA0_RX_USCIB0_RX_ISR
       BIT.B #UCA0RXIFG, &IFG2 ; USCI_A0 Receive Interrupt?
       JNZ USCIA0_RX_ISR
    USCIB0_RX_ISR
       ; Read UCB0RXBUF (clears UCB0RXIFG)
        ...
       RETI

    USCIA0_RX_ISR
       ; Read UCA0RXBUF (clears UCA0RXIFG)
       ...
       RETI
*/

    if((IFG2 & UCA0RXIFG) == UCA0RXIFG)     // UART
    {
        //P1OUT  ^= BIT6;     // light up P1.6 LED on RX
        IFG2 &= ~UCA0RXIFG; // Clear RX flag


      //#ifdef NOCODE
        rxBuf[rxNqIndx] = UCA0RXBUF;
        //UCA0TXBUF = rxBuf[rxNqIndx];
        rxNqIndx++;
        if(rxNqIndx >= SIZEOF_RXBUF)
            rxNqIndx = 0;
      //#endif
    }
    //else
    if((IFG2 & UCB0RXIFG) == UCB0RXIFG)     // i2c
    {
        // i2c receive
        if(UCNACKIFG & UCB0STAT)
        {
            UCB0STAT &= ~UCNACKIFG;             // Clear flag so that not to come here again
            i2c_packet.isr_mode=NOACK;          // The main function needs to act based on noack
            UCB0CTL1 |= UCTXSTP;                // I2C stop condition
            LPM0_EXIT;                          // Exit LPM0
        }
    }
}






/*
* This interrupt is called each time the UCSI_B module is either ready to get a new byte in UCB0TXBUF to send to the I2C device, or
* a new byte is read into UCB0RXBUF and we should pick it up.
* The interrupt is called as both UCB0TXIE and UCB0RXIE are enabled. To stop this interrupt being called indefinitely, the corresponding
* interrupt flag should be cleared.
* These flags are automatically clearly by the USCI_B module if the UCB0XXBUF is access. However, if we are to do something different than reading
* or writing a byte to/from the UCB0XXBUF, we need to clear the corresponding flag by ourselves or the ISR will be called for ever,
* and the whole program will hang.
*/

//interrupt(USCIAB0TX_VECTOR) USCIAB0TX_ISR(void)
#pragma vector = USCIAB0TX_VECTOR
__interrupt void USCIAB0TX_ISR(void)
{
/*
 *    USCIA0_TX_USCIB0_TX_ISR
   BIT.B #UCA0TXIFG, &IFG2 ; USCI_A0 Transmit Interrupt?
   JNZ USCIA0_TX_ISR
USCIB0_TX_ISR
   ; Write UCB0TXBUF (clears UCB0TXIFG)
   ...
   RETI

USCIA0_TX_ISR
   ; Write UCA0TXBUF (clears UCA0TXIFG)
   ...
   RETI
 *
 */

    if((IFG2 & UCA0TXIFG) == UCA0TXIFG)     // UART
    {
        // THOM TODO: turn off the interrupt for uart transmit if you get here
        IE2 &= ~UCA0TXIE;        // disable the interrupt - should never have been enabled!

        IFG2 &= ~UCA0TXIFG;

       // while(1)
       //     ;


    }

    if((IFG2 & UCB0TXIFG) == UCB0TXIFG)     // i2c
    {
        // Transmit address bytes irrespective of send or receive mode.
        if (i2c_packet.addr_index==0)
        {
           UCB0TXBUF = i2c_packet.addr_high_byte;
           i2c_packet.addr_index++;
        }
        else if (i2c_packet.addr_index==1 && i2c_packet.addr_type==TWOBYTEADDR)
        {
            UCB0TXBUF = i2c_packet.addr_low_byte;
            i2c_packet.addr_index++;
        }
        else if(UCB0TXIFG & IFG2 && i2c_packet.isr_mode==READ)
        {
                                                // USCI_B is ready to get a new data byte to transmit it, and we are in READ mode.
                                                // So, we should not continue writing, but should exit to the calling function to
                                                // switch the USCI_B into read mode
            IFG2 &= ~UCB0TXIFG;                 // Clear USCI_B0 TX int flag manually as we did not write to the UCB0TXBUF
            LPM0_EXIT;                          // Exit LPM0
        }

        else if(UCB0TXIFG & IFG2 && i2c_packet.isr_mode==WRITE) // USCI_B is ready to get a new data byte to transmit it, and we are in write mode.
        {

            if(i2c_packet.buf_index == i2c_packet.buf_size)     // If no more data to transmit, then issue stop condition and wake CPU.
            {
                IFG2 &= ~UCB0TXIFG;                             // Clear USCI_B0 TX int flag manually as we did not write to the UCB0TXBUF
                UCB0CTL1 |= UCTXSTP;                            // I2C stop condition
                LPM0_EXIT;                                      // Exit LPM0
            } else
            {
                UCB0TXBUF = i2c_packet.data_buf[i2c_packet.buf_index];
                i2c_packet.buf_index++;                         // Increment TX byte counter
            }
       }
        else if (UCB0RXIFG & IFG2 && i2c_packet.addr_index==i2c_packet.addr_type)
        {
            // Read mode, and we already completed writing the address
            i2c_packet.data_buf[i2c_packet.buf_index]= UCB0RXBUF;
            i2c_packet.buf_index++;                             // Increment RX byte counter
            if(i2c_packet.buf_index == i2c_packet.buf_size)
            {    // If last byte to receive, then issue stop condition and wake CPU.
                IFG2 &= ~UCB0RXIFG;                             // Clear USCI_B0 RX int flag
                UCB0CTL1 |= UCTXSTP;                            // I2C stop condition here to avoid reading any extra bytes
                LPM0_EXIT;                                      // Exit LPM0
            }
        }
    }
}

#endif



#ifdef OLDSERIALINT
#pragma vector = USCIAB0RX_VECTOR
__interrupt void ReceiveInterrupt(void)
{
  //P1OUT  ^= BIT6;     // light up P1.6 LED on RX
  IFG2 &= ~UCA0RXIFG; // Clear RX flag


//#ifdef NOCODE
  rxBuf[rxNqIndx] = UCA0RXBUF;
  //UCA0TXBUF = rxBuf[rxNqIndx];
  rxNqIndx++;
  if(rxNqIndx >= SIZEOF_RXBUF)
      rxNqIndx = 0;
//#endif

}

#pragma vector = USCIAB0TX_VECTOR
__interrupt void TransmitInterrupt(void)
{
    P1OUT  ^= BIT0;//light up P1.0 Led on Tx Launchpad (HARV_EN_LO)
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
#endif


#ifdef LOOKINTIMER
//------------------------------------------------------------------------------
// micro seconds delays
//
void delay_usec(uint16_t microSec)
{
                                // Setup TimerA
    TA1CCTL0 = CCIE;            // interrupt enabled
    TA1CCR0 = TA1R + microSec;  // micro secs @ 1Mhz Clock
    TA1CTL = TASSEL_2 + MC_2;   // SMCLK, continuous mode.
    LPM0;                       // suspend CPU
}

// Timer A1 interrupt service routine. TIMERx_Ay_VECTOR.(x being the index of the timer, y of the vector for this timer)
#pragma vector=TIMER1_A0_VECTOR
__interrupt void Timer1_A0 (void)
{
    TA1CTL = 0;                     // Stop Timer_A1
    LPM0_EXIT;                      // Return active
}


#endif



#ifdef GOODCODEI2CDS3231RTC

/*
* Here's an example using an MSP430G2553 to read/write using I2C to an 24LC512 and DS3231 RTC.
* Two functions are provided for read and write.
*
* The code avoids any while loops, and relies completely on interrupts and low power modes of the MSP430 to reduce the power consumption to the
* minimum.
*
* Author: Ahmed Talaat (aa_talaat@yahoo.com)
* Date: 19 October 2012
*/

    #include  "msp430g2553.h"

    #define ONEBYTEADDR 1
    #define TWOBYTEADDR 2
    #define WRITE       0                       // ISR mode WRITE or READ
    #define READ        1
    #define NOACK       2
    #define EEPROM_ADDR 0x50
    #define DS3231_ADDR 0x68

    unsigned char txdataEEPROM[] = "Here's an example using an MSP430G2553 and a 24LC512 and DS3231 RTC. Two functions are provided for read and write.";
    unsigned char txdataDS3231[7] = {0x20, 0x30, 0x20, 0x04, 0x17, 0x10, 0x12};
    unsigned char rxdata[150];

    typedef struct {
        volatile unsigned char *data_buf;       // address of tx or rx data buffer
        volatile unsigned char buf_size;        // size of the buffer
        volatile unsigned char buf_index;       // index in the buffer
        volatile unsigned char addr_index;      // index of the byte address (0,1)
        volatile unsigned char isr_mode;        // Tx or Rx affects the interrupt logic
        volatile unsigned char addr_high_byte;  // High byte of the address to read/write to
        volatile unsigned char addr_low_byte;   // Low byte of the address to read/write to
        volatile unsigned char addr_type;       // two bytes like eeprom or 1 byte like RTC for example
    } i2c_t;

    i2c_t   i2c_packet;

    void i2c_init(void);
    void i2c_tx(unsigned char, unsigned char *, unsigned char, unsigned char, unsigned char, unsigned char);
    void i2c_rx(unsigned char, unsigned char *, unsigned char, unsigned char, unsigned char, unsigned char);
    void delay_usec(int);

    void main(void)
    {
       WDTCTL = WDTPW + WDTHOLD;                // Stop WDT
       BCSCTL1 = CALBC1_1MHZ;                   // Set DCO to 1MHz
       DCOCTL = CALDCO_1MHZ;
       i2c_init();                              // Initialize I2C
                                                // Address is High byte then low byte
       i2c_tx(EEPROM_ADDR, txdataEEPROM, sizeof(txdataEEPROM)-1,TWOBYTEADDR,0x01,0x00);//i2c TX 115 bytes starting @ address 01:00
       i2c_tx(EEPROM_ADDR, txdataEEPROM, sizeof(txdataEEPROM)-1,TWOBYTEADDR,0x02,0x00);//i2c TX 115 bytes starting @ address 02:00
       i2c_tx(EEPROM_ADDR, txdataEEPROM, sizeof(txdataEEPROM)-1,TWOBYTEADDR,0x03,0x00);//i2c TX 115 bytes starting @ address 03:00
       i2c_tx(DS3231_ADDR, txdataDS3231, 7,ONEBYTEADDR,0x00,0x00);//i2c TX 7 bytes "HELLO WORLD" starting @ address 00
       i2c_tx(DS3231_ADDR, txdataDS3231, 7,ONEBYTEADDR,0x00,0x00);//i2c TX 7 bytes "HELLO WORLD" starting @ address 00

       i2c_rx(EEPROM_ADDR, rxdata, 115,TWOBYTEADDR,0x01,0x00);//i2c RX 115 bytes from EEPROM starting @ address 01:00
       i2c_rx(EEPROM_ADDR, rxdata, 115,TWOBYTEADDR,0x02,0x00);//i2c RX 115 bytes from EEPROM starting @ address 02:00
       i2c_rx(EEPROM_ADDR, rxdata, 115,TWOBYTEADDR,0x03,0x00);//i2c RX 115 bytes from EEPROM starting @ address 03:00

       i2c_rx(DS3231_ADDR, rxdata, 7,ONEBYTEADDR,0x00,0x00);//i2c RX 7 bytes from DS3231 starting @ address 00:00

       LPM0;                                    // Enter LPM0 w/ interrupts
    }

    void i2c_init(void){
        P1SEL |= BIT6 + BIT7;                   //Set I2C pins
        P1SEL2|= BIT6 + BIT7;
        UCB0CTL1 |= UCSWRST;                    //Enable SW reset
        UCB0CTL0 = UCMST + UCMODE_3 + UCSYNC;   //I2C Master, synchronous mode
        UCB0CTL1 = UCSSEL_2 + UCSWRST;          //Use SMCLK, keep SW reset
        UCB0BR0 = 10;                           //fSCL = SMCLK/11 = ~100kHz
        UCB0BR1 = 0;
        UCB0CTL1 &= ~UCSWRST;                   //Clear SW reset, resume operation
        IE2 |= UCB0TXIE;                        //Enable TX interrupt
        IE2 |= UCB0RXIE;                        //Enable RX interrupt
        UCB0I2CIE |= UCNACKIE;                  //Need to enable the status change interrupt
        __enable_interrupt();                   //Enable global interrupt
    }

/*
 * This functions writes to any I2C device. It takes as parameters:
 *  - The slave address (7 bits)
 *  - Pointer to the data buffer to transmit
 *  - Size of the buffer
 *  - Size of the address location to start writing at. Being 2 bytes for some of the EEPROMs and single byte
 *    for other devices. Please note that the ISR of this function assumes that the address bytes are written high
 *    byte first, then low byte second.
 *    In case of single byte address device, only the high byte will be used to set the address.
 *  - The high byte and low byte address for the location to start writing at.
 *
 *  In case the start condition of the write operation is not ack (for example EEPROM busy with a previous write cycle),
 *  the corresponding interrupts detects this condition, generates a stop signal, and a Timer A1 (not Timer A0)
 *  is activated for 0.5 ms, then the trial for writing is repeated.
 *
 *  Please note that this function does not cater for the EEPROM 128byte paging requirements. So, if you are going
 *  to write more than 128 bytes to an EEPROM, you will need to write a higher level function to segment the
 *  writing into consecutive 128byte chunks.
 *
 */

    void i2c_tx(unsigned char slave_addr, unsigned char *txdata, unsigned char bufSize, unsigned char addr_size,
                        unsigned char high_byte_addr, unsigned char low_byte_addr) {
        i2c_packet.isr_mode=WRITE;
        i2c_packet.data_buf=txdata;
        i2c_packet.buf_size=bufSize;
        i2c_packet.buf_index=0;
        i2c_packet.addr_type=addr_size;
        i2c_packet.addr_high_byte=high_byte_addr;
        i2c_packet.addr_low_byte=low_byte_addr;
        i2c_packet.addr_index=0;
        UCB0I2CSA = slave_addr;                 //Slave Address

        while (1) {
            UCB0CTL1 |= UCTR + UCTXSTT;         // I2C TX, start condition
            LPM0;                               // Enter LPM0
                                                // and remain until all data is TX'd
            if (i2c_packet.isr_mode == NOACK){  // If no ack received, then sleep for 0.5ms and try again
                i2c_packet.isr_mode = WRITE;
                i2c_packet.addr_index=0;        // Reset the address index for the next write operation
                delay_usec(500);

            } else {
                break;                          // Successful write, then quit
            }
        }
    }

    /*
     * This functions reads from any I2C device. It takes as parameters:
     *  - The slave address (7 bits)
     *  - Pointer to the data buffer to fill with data read.
     *  - Size of the buffer
     *  - Size of the address location to start writing at. Being 2 bytes for some of the EEPROMs and single byte
     *    for other devices. Please note that the ISR of this function assumes that the address bytes are written high
     *    byte first, then low byte second.
     *    In case of single byte address device, only the high byte will be used to set the address.
     *  - The high byte and low byte address for the location to start reading at.
     *
     *  The function starts with a write operation to specify the address at which the read operation with start
     *  In case the start condition of the write operation is not ack (for example EEPROM busy with a a previous write cycle),
     *  the corresponding interrupts detects this condition, generates a stop signal, and a Timer A1 (not Timer A0)
     *  is activated for 0.5 ms, then the trial for writing is repeated.
     *
     *  Once the write address is successful, the functions switch to read mode, and fills the buffer provided
     *
     */


    void i2c_rx(unsigned char slave_addr, unsigned char *rxdata, unsigned char bufSize, unsigned char addr_size,
            unsigned char high_byte_addr, unsigned char low_byte_addr) {
        i2c_packet.isr_mode=READ;               // The ISR will send the address bytes, then wake CPU.
        i2c_packet.addr_type=addr_size;
        i2c_packet.addr_high_byte=high_byte_addr;
        i2c_packet.addr_low_byte=low_byte_addr;
        i2c_packet.addr_index=0;
        UCB0I2CSA = slave_addr;                 // Slave Address

        while (1) {
            UCB0CTL1 |= UCTR + UCTXSTT;         // I2C TX, start condition
            LPM0;                               // Enter LPM0
                                                // and remain until all data is TX'd
            if (i2c_packet.isr_mode == NOACK){  // If no ack received, then sleep for 0.5ms and try again
                i2c_packet.isr_mode = READ;
                i2c_packet.addr_index=0;        // Reset the address index for the next write operation
                delay_usec(500);
            } else {
                break;                          // Successful write, then quit
            }
        }
                                                // We wrote already the address, so now read only data.
        i2c_packet.addr_index=i2c_packet.addr_type;
        i2c_packet.data_buf=rxdata;
        i2c_packet.buf_size=bufSize;
        i2c_packet.buf_index=0;
        UCB0CTL1 &= ~UCTR;                      // I2C RX
        UCB0CTL1 |= UCTXSTT;                    // I2C re-start condition
        LPM0;                                   // Enter LPM0
                                                // and remain until all data is received
    }


    //interrupt(USCIAB0RX_VECTOR) state change to trap the no_Ack from slave case
    #pragma vector = USCIAB0RX_VECTOR
    __interrupt void USCIAB0RX_ISR(void)
    {
        if(UCNACKIFG & UCB0STAT) {
            UCB0STAT &= ~UCNACKIFG;             // Clear flag so that not to come here again
            i2c_packet.isr_mode=NOACK;          // The main function needs to act based on noack
            UCB0CTL1 |= UCTXSTP;                // I2C stop condition
            LPM0_EXIT;                          // Exit LPM0
        }
    }

/*
 * This interrupt is called each time the UCSI_B module is either ready to get a new byte in UCB0TXBUF to send to the I2C device, or
 * a new byte is read into UCB0RXBUF and we should pick it up.
 * The interrupt is called as both UCB0TXIE and UCB0RXIE are enabled. To stop this interrupt being called indefinitely, the corresponding
 * interrupt flag should be cleared.
 * These flags are automatically clearly by the USCI_B module if the UCB0XXBUF is access. However, if we are to do something different than reading
 * or writing a byte to/from the UCB0XXBUF, we need to clear the corresponding flag by ourselves or the ISR will be called for ever,
 * and the whole program will hang.
 */

    //interrupt(USCIAB0TX_VECTOR) USCIAB0TX_ISR(void)
    #pragma vector = USCIAB0TX_VECTOR
    __interrupt void USCIAB0TX_ISR(void)
    {
        // Transmit address bytes irrespective of send or receive mode.
        if (i2c_packet.addr_index==0){
           UCB0TXBUF = i2c_packet.addr_high_byte;
           i2c_packet.addr_index++;
        }
        else if (i2c_packet.addr_index==1 && i2c_packet.addr_type==TWOBYTEADDR){
            UCB0TXBUF = i2c_packet.addr_low_byte;
            i2c_packet.addr_index++;
        }
        else if(UCB0TXIFG & IFG2 && i2c_packet.isr_mode==READ) {
                                                // USCI_B is ready to get a new data byte to transmit it, and we are in READ mode.
                                                // So, we should not continue writing, but should exit to the calling function to
                                                // switch the USCI_B into read mode
            IFG2 &= ~UCB0TXIFG;                 // Clear USCI_B0 TX int flag manually as we did not write to the UCB0TXBUF
            LPM0_EXIT;                          // Exit LPM0
        }

        else if(UCB0TXIFG & IFG2 && i2c_packet.isr_mode==WRITE){// USCI_B is ready to get a new data byte to transmit it, and we are in write mode.

            if(i2c_packet.buf_index == i2c_packet.buf_size){    // If no more data to transmit, then issue stop condition and wake CPU.
                IFG2 &= ~UCB0TXIFG;                             // Clear USCI_B0 TX int flag manually as we did not write to the UCB0TXBUF
                UCB0CTL1 |= UCTXSTP;                            // I2C stop condition
                LPM0_EXIT;                                      // Exit LPM0
            } else {
                UCB0TXBUF = i2c_packet.data_buf[i2c_packet.buf_index];
                i2c_packet.buf_index++;                         // Increment TX byte counter
            }
       }
        else if (UCB0RXIFG & IFG2 && i2c_packet.addr_index==i2c_packet.addr_type) {
                                                                // Read mode, and we already completed writing the address
        i2c_packet.data_buf[i2c_packet.buf_index]= UCB0RXBUF;
        i2c_packet.buf_index++;                             // Increment RX byte counter
        if(i2c_packet.buf_index == i2c_packet.buf_size){    // If last byte to receive, then issue stop condition and wake CPU.
            IFG2 &= ~UCB0RXIFG;                             // Clear USCI_B0 RX int flag
            UCB0CTL1 |= UCTXSTP;                            // I2C stop condition here to avoid reading any extra bytes
            LPM0_EXIT;                                      // Exit LPM0
        }
    }
   }

    //------------------------------------------------------------------------------
    // micro seconds delays
    //
    void delay_usec(int interval){
                                    // Setup TimerA
        TA1CCTL0 = CCIE;            // interrupt enabled
        TA1CCR0 = TA1R + interval;  // micro secs @ 1Mhz Clock
        TA1CTL = TASSEL_2 + MC_2;   // SMCLK, continuous mode.
        LPM0;                       // suspend CPU
    }

    // Timer A1 interrupt service routine. TIMERx_Ay_VECTOR.(x being the index of the timer, y of the vector for this timer)
    #pragma vector=TIMER1_A0_VECTOR
    __interrupt void Timer1_A0 (void)
    {
        TA1CTL = 0;                     // Stop Timer_A1
        LPM0_EXIT;                      // Return active
    }



#endif


//-------------------------------------------------------------------------------------------------------------------


#ifdef GOODLOOKINGCODE

http://www.kerrywong.com/2013/01/09/interfacing-ds3232-rtc-with-msp430g2452/

#ifndef I2C_H
#define I2C_H

void i2c_init(void);
void i2c_start(void);
void i2c_stop(void);
unsigned char i2c_write8(unsigned char c);
unsigned char i2c_read8(unsigned char acknack);

#endif

//code adapted from http://www.43oh.com/forum/viewtopic.php?f=9&t=139
//Kerry D. Wong
//http://www.kerrywong.com
#include "i2c.h"
#include <msp430.h>

#define SDA  BIT7 //P1.7
#define SCL  BIT6 //P1.6

void i2c_init(void)
{
    P1DIR |= SCL | SDA; // Set SCL, SDA as Output
    P1REN |= SCL | SDA; // Set Pull-Ups on SCL and SDA


    // enable SDA, SCL, SCLK, i2c mode, MSB, output enabled, hold in reset
    USICTL0 = USIPE7 | USIPE6 | USIMST | USIOE | USISWRST;

    // USICTL0 Upper 8bit Register of 16bit USICTL Register
    // USIPE7   = P1.7 USI Mode, i2c SDA enabled
    // USIPE6   = P1.6 USI Mode, i2c SCL enabled
    // USIPE5   = P1.5 USI Mode, i2c Clock Input? (Not Set)
    // USILSB   = LSB Mode (Not Set = MSB)
    // USIMST   = Master Mode
    // USIGE    = Output Latch (Not Set = Clock Controlled)
    // USIOE    = Data Output Enable
    // USISWRST = USI Software Reset (Set to allow changing settings)


    // SMCLK / 4, and Reverse Clock Polarity
    USICKCTL = USIDIV_1 + USISSEL_2 + USICKPL;

    // USICKCTL 8bit USI Clock Control Register
    // USIDIVx  = Clock Divider (Bit7-5, USIDIV_2 = Divide by 4)
    // USISSELx = Clock Source (For Master Mode, Bit4-2, USISSEL_2 = SMCLK)
    // USICKPL  = Clock Polarity (0 = Inactive Low, 1 = Inactive High)
    // USISWCLK = Software Clock State

    // I2C Mode
    USICTL1 = USII2C;

    // USICTL1 Lower 8bit Register of 16bit USICTL Register
    // USICKPH   = Clock Phase (0 = Data Changed, then Captured, 1 = Data Captured, then Changed)
    // USII2C    = I2C mode
    // USISTTIE  = START condition Interrupt
    // USIIE     = USI Counter Interrupt
    // USIAL     = Arbitration Lost Notification
    // USISTP    = STOP condition Notification
    // USISTTIFG = START condition Int. Flag
    // USIIFG    = USI Counter Int. Flag

    // release from reset
    USICTL0 &= ~USISWRST;
}

void i2c_start(void)
{
    // Send i2c START condition
    USISRL = 0x00; // Load USISRL Lower Byte Shift Register MSB with 0 for i2c START
    USICTL0 |= USIGE | USIOE; // Force Output Latch, And Enable Data Output Bit (High to Low SDA while SCL is High)
    USICTL0 &= ~USIGE; // Clear Output Latch (Return to Clock Control)
}

void i2c_stop(void)
{
    // Prepare i2c STOP condition
    USICTL0 |= USIOE; // Enable Data Output Bit (Turn SDA into Output)
    USISRL = 0x00; // Load USISRL Lower Byte Shift Register MSB with 0 for i2c STOP
    USICNT = 1; // Load USICNT Counter with number of Bits to Send. USIIFG Auto-Cleared
    // Data TXed by USI I2C
    while((USICTL1 & USIIFG) != 0x01); // Delay, Wait for USIIFG, Counter down to 0

    // Send i2c STOP condition
    USISRL = 0xFF; // Load USISRL Lower Byte Shift Register MSB with 1 for i2c STOP
    USICTL0 |= USIGE; // Force Output Latch (Low to High SDA while SCL is High)
    USICTL0 &= ~USIOE & ~USIGE ; // Clear Data Output Enable Bit and Output Latch (Release SCL)
}

unsigned char i2c_write8(unsigned char c)
{
// TX
    USICTL0 |= USIOE; // Enable Data Output Bit (Turn SDA into Output)
    USISRL = c; // Load USISRL Lower Byte Shift Register with 8 Bit data (Byte)
    USICNT = 8; // Load USICNT Counter with number of Bits to Send. USIIFG Auto-Cleared
    // Data TXed by USI I2C
    while((USICTL1 & USIIFG) != 0x01); // Delay, Wait for USIIFG, Counter down to 0

// RX
    // Data TXed. Ready to Receive (n)ACK from i2c Slave
    USICTL0 &= ~USIOE; // Clear Data Output Enable Bit (Turn SDA into Input)
    USICNT = 1; // Load USICNT Counter with number of Bits to Receive. USIIFG Auto-Cleared
    // Data RXed by USI I2C
    while((USICTL1 & USIIFG) != 0x01); // Delay, Wait for USIIFG, Counter down to 0

// Return Data
    c = USISRL; // LSB of USISRL Holds Ack Status of 0 = ACK (0x00) or 1 = NACK (0x01)
    return c;
}

unsigned char i2c_read8(unsigned char acknack)
{
// RX
    USICTL0 &= ~USIOE; // Clear Data Output Enable Bit (Turn SDA into Input)
    USISRL = 0x00; // Clear USISRL Lower Byte Shift Register (Byte)
    USICNT = 8; // Load USICNT Counter with number of Bits to Receive. USIIFG Auto-Cleared
    // Data RXed by USI I2C
    while((USICTL1 & USIIFG) != 0x01); // Delay, Wait for USIIFG, Counter down to 0

// Copy Data to c
    unsigned char c;
    c = USISRL; // USISRL Holds Received Data

// TX
    // Data RXed. Ready to Send (n)ACK to i2c Slave
    USICTL0 |= USIOE; // Enable Data Output Bit (Turn SDA into Output)
    USISRL = acknack; // Load USISRL Lower Byte Shift Register MSB with acknack (0x00 = Ack, 0xFF = Nack)
    USICNT = 1; // Load USICNT Counter with number of Bits to Send. USIIFG Auto-Cleared
    // Data TXed by USI I2C
    while((USICTL1 & USIIFG) != 0x01); // Delay, Wait for USIIFG, Counter down to 0

// Return Data
    return c;
}


//---------------- MAIN.c ----------------------------------------

#include <msp430.h>
#include "i2c.h"

/**
 * Kerr D. Wong
 * http://www.kerrywong.com
 *
 * D3232 RTC Example Using MSP430G2452
 */

#define HIGH 1
#define LOW  0
#define OUTPUT 1
#define INPUT 0

#define DATETIME_REG_ADDR 0x00
#define CONTROL_REG_ADDR 0x0E
#define STATUS_REG_ADDR 0x0F
#define AGING_REG_ADDR 0x10
#define TEMPARATURE_REG_ADDR 0x11
#define SRAM_ADDR 0x14

#define _BV(bit) (1 << (bit))

#define DS3232_I2C_ADDRESS 0x68

//pins on PORT 1
// 4-7seg display pins
#define pinOE  0
#define pinLatch  1
#define pinClk  2
#define pinData 3

//pins on PORT 2
#define pinSecondSense  1

//depends on how the 7-seg display
//is implemented, you will likely
//need to change this
const unsigned char NUMS[] = {
        255-252, 255-96, 255-218, 255-242, 255-102,
        255-182, 255-190, 255-224, 255-254, 255-246,
        255 //blank
};

typedef struct {
  unsigned char second; //00H
  unsigned char minute; //01H
  unsigned char hour; //02H
  unsigned char dayOfWeek; //03H
  unsigned char dayOfMonth; //04H
  unsigned char month; //05H
  unsigned char year; //06H
} Date;

Date dt;

/**
 * Stop the Watch Dog Timer
 */
void inline stopWatchDogTimer() {
    WDTCTL = WDTPW + WDTHOLD;
}

/**
 * Set the pin mode
 * portNo: port (1 or 2)
 * pinNo: pin number in the port specified
 * pinMode: INPUT/OUTPUT
 */
void pinMode(unsigned char portNo, unsigned char pinNo, unsigned char pinMode) {
    if (portNo == 1) {
        if (pinMode)
            P1DIR |= (1 << pinNo);
        else
            P1DIR &= ~(1 << pinNo);
    } else if (portNo == 2) {
        if (pinMode)
            P2DIR |= (1 << pinNo);
        else
            P2DIR &= ~(1 << pinNo);
    }
}

/**
 * Write HIGH or LOW to a pin previously set as output
 * portNo: port (1 or 2)
 * pinNo: pin number in the port specified
 * value: HIGH/LOW
 */
void digitalWrite(unsigned char portNo, unsigned char pinNo, unsigned char value) {
    if (portNo == 1) {
        if (value)
            P1OUT |= 1 << pinNo;
        else
            P1OUT &= ~(1 << pinNo);
    } else if (portNo == 2) {
        if (value)
            P2OUT |= 1 << pinNo;
        else
            P2OUT &= ~(1 << pinNo);
    }
}

/**
 * Read logic level from pin
 * portNo: port (1 or 2)
 * pinNo: pin number in the port specified
 */
unsigned char digitalRead(unsigned char portNo, unsigned char pinNo) {
    int result;

    result = 0;

    if (portNo == 1) {
        result = (P1IN & (1 << pinNo)) >> pinNo;
    } else if (portNo == 2) {
        result = (P2IN & (1 << pinNo)) >> pinNo;
    }

    return result;
}

/**
 * Shift out using Port 1
 */
void shiftOut1(unsigned char dataPin, unsigned char clockPin, unsigned char value)
{
  int i;

  for (i = 0; i < 8; i++)  {
    digitalWrite(1, dataPin, (value & _BV(i)));
    digitalWrite(1, clockPin, HIGH);
    digitalWrite(1, clockPin, LOW);
  }
}

//Display the curent time
//secInd is fed from the SQW pin (pin 5) which is used to create the
//flashing effect for the second indicator ":"
void displayTime(unsigned char hour, unsigned char min, unsigned char secInd)
{
        unsigned char d1 = 0, d2 = 0, d3 = 0, d4 = 0;

        d1 = hour/10;
        d2 = hour - d1 * 10 ;
        d3 = min/10;
        d4 = min - d3 * 10;

        if (d1 == 0) d1 = 10;

        digitalWrite(1, pinLatch, LOW);
        shiftOut1(pinData, pinClk, NUMS[d4]);
        if (secInd > 0) {
            shiftOut1(pinData, pinClk, NUMS[d3] - 1);
            shiftOut1(pinData, pinClk, NUMS[d2] - 1);
        } else {
            shiftOut1(pinData, pinClk, NUMS[d3]);
            shiftOut1(pinData, pinClk, NUMS[d2]);
        }

        shiftOut1(pinData, pinClk, NUMS[d1]);
        digitalWrite(1, pinLatch, HIGH);
}

// Convert normal decimal numbers to binary coded decimal
unsigned char decToBcd(unsigned char val)
{
  return ( (val/10*16) + (val%10) );
}

// Convert binary coded decimal to normal decimal numbers
unsigned char bcdToDec(unsigned char val)
{
  return ( (val/16*10) + (val%16) );
}

// Gets the date and time from the ds1307 and prints result
void getDateDS3232()
{
    i2c_start();
    i2c_write8(DS3232_I2C_ADDRESS << 1);
    i2c_write8(DATETIME_REG_ADDR);
    i2c_stop();

    i2c_start();
    i2c_write8(DS3232_I2C_ADDRESS << 1 | 1);

    dt.second     = bcdToDec(i2c_read8(0x00));
    dt.minute     = bcdToDec(i2c_read8(0x00));
    dt.hour       = bcdToDec(i2c_read8(0x00)); // Need to change this if 12 hour am/pm
    dt.dayOfWeek  = bcdToDec(i2c_read8(0x00)); // 0= Sunday
    dt.dayOfMonth = bcdToDec(i2c_read8(0x00));
    dt.month      = bcdToDec(i2c_read8(0x00));
    dt.year       = bcdToDec(i2c_read8(0x00));

    i2c_stop();
}

void setDate3232()
{
    i2c_start();
    i2c_write8(DS3232_I2C_ADDRESS << 1);
    i2c_write8(DATETIME_REG_ADDR);

    i2c_write8(decToBcd(00)); //sec
    i2c_write8(decToBcd(43)); //min
    i2c_write8(decToBcd(20)); //hour
    i2c_write8(decToBcd(7)); //day
    i2c_write8(decToBcd(05)); //date
    i2c_write8(decToBcd(01)); //month
    i2c_write8(decToBcd(13)); //year

    i2c_stop();
}

void setControlRegisters(){
    i2c_start();
    i2c_write8(DS3232_I2C_ADDRESS << 1);

    i2c_write8(CONTROL_REG_ADDR); //Goto register 0Eh
    i2c_write8(0x00);

    i2c_stop();
}

void setupRTC3232(){
    i2c_init();
    //setDate3232(); // call this initially to set the date
    setControlRegisters();
}

void init()
{
    stopWatchDogTimer();

    pinMode(1, pinOE, OUTPUT);
    pinMode(1, pinLatch, OUTPUT);
    pinMode(1, pinClk, OUTPUT);
    pinMode(1, pinData, OUTPUT);
    pinMode(2, pinSecondSense, INPUT);

    digitalWrite(1, pinOE, LOW);
    digitalWrite(1, pinData, HIGH);
    digitalWrite(1, pinLatch, HIGH);
    digitalWrite(1, pinClk, HIGH);

    setupRTC3232();
}

void main(void) {
    int r = 0;

    init();

    for(;;) {
        getDateDS3232();

        r = digitalRead(2, pinSecondSense);
        displayTime(dt.hour, dt.minute, r);
    }
}


#endif





#ifdef  NOCODE

#ifdef NOCODE
    VOID I2C_Init(VOID)
    {
        //select GPIO P1.6 and P1.7 as I2C pin
        P1SEL  |= BIT6 + BIT7;
        P1SEL2 |= BIT6 + BIT7;
        // Enable SW set to config I2C;
        UCB0CTL1 |= UCSWRST;
        // Config I2C
        UCB0CTL0 = UCMST + UCMODE_3 + UCSYNC; // I2C master, synchronous mode
        UCB0CTL1 = UCSSEL_2 + UCSWRST; // Use SMCLK, keep SW reset;
        // Config baudrate I2C SCL = SMCLK/DIV
        // LSB of DIV stored in UCB0BR0, MSB stored in BCB0BR1
        UCB0BR0 = 0x19;//0x0A; // 0x05, 0x19, 0x0A;
        UCB0BR1 = 0;
        IFG2 &= ~UCB0TXIFG;
        IFG2 &= ~UCB0RXIFG;
        UCB0CTL1 &= ~UCSWRST; // Clear SW reset.
    }
#endif

}

void i2c_SetSlaveAddress(uint8_t i2cSlaveAddr)
{
    // Set slave address
    UCB0I2CSA = i2cSlaveAddr;
}


// Transmit i2c stop signal
void i2c_Stop(void)
{
    while((UCB0STAT & BUSY)!= 0);
    UCB0CTL1 |= UCTXSTP;
}


/*--------------------------------------------------------------------------------
Function    : I2C_StartWrite
Purpose     : Set slaver address and send a byte to slave
Parameters  : I2C Address of Slaver
Return      : 0 if success, 1 if failt
--------------------------------------------------------------------------------*/
uint8_t i2c_StartWrite(uint16_t i2cSlaveAddr, uint8_t txByte)
{
    while((UCB0STAT & BUSY)!= 0);

    UCB0I2CSA = i2cSlaveAddr;

    UCB0CTL1 |= UCTR;                      // I2C TX

    UCB0CTL1 |= UCTXSTT;

    UCB0TXBUF = txByte;

    while((IFG2 & UCB0TXIFG) == 0);
    if((UCB0STAT & UCNACKIFG)!=0)
        return 0;
    else
        return 1;
}


/*--------------------------------------------------------------------------------
Function    : I2C_WriteByte
Purpose     : Write single Byte from slave, call after called I2C_StartWrite
Parameters  : byData
Return      : 0 if success, 1 if fault
--------------------------------------------------------------------------------*/
uint8_t I2C_WriteByte(uint8_t txByte)
{
    while((UCB0STAT & BUSY)!= 0);

    UCB0TXBUF = txByte;
    while((IFG2 & UCB0TXIFG) == 0);

    if((UCB0STAT & UCNACKIFG)!=0)
        return 0;
    else
        return 1;
}


/*--------------------------------------------------------------------------------
Function    : I2C_StartRead
Purpose     : Set slave Address and Start read a byte to the slave
Parameters  : I2C Address of Slaver
Return      : 0 if success, 1 if fault
--------------------------------------------------------------------------------*/
uint8_t i2c_StartRead(uint16_t i2cSlaveAddr)
{
    while((UCB0STAT & BUSY)!= 0);

    UCB0I2CSA = i2cSlaveAddr;

    UCB0CTL1 &= ~UCTR;

    UCB0CTL1 |= UCTXSTT;

    while((UCB0CTL1 & UCTXSTT)!= 0);
    if((UCB0STAT & UCNACKIFG)!=0)
        return 0;
     else
        return 1;
}

/*--------------------------------------------------------------------------------
Function    : I2C_ReadByte
Purpose     : Read single Byte from slave
Parameters  : None
Return      : byData
--------------------------------------------------------------------------------*/
uint8_t i2c_ReadByte(void)
{
    uint8_t rxByte;

    while((UCB0STAT & BUSY)!= 0);

    while((IFG2 & UCB0RXIFG) == 0);
    rxByte = UCB0RXBUF;

    return rxByte;
}

/*--------------------------------------------------------------------------------
Function    : I2C_ReadData
Purpose     : Read multi byte to register of slave device
Parameters  : pData - Pointer to data
                  byDevideAddr  - I2C address of slave device
                  byRegister    - Register address need to read data.
Return      : None
--------------------------------------------------------------------------------*/
VOID I2C_ReadData(uint8_t *pBuf, uint8_t byDevideAddr, uint8_t byRegister, uint8_t nLength)
{
    BYTE nIndex;
    //I2C_Init();
    I2C_StartWrite(byDevideAddr, byRegister);
    I2C_StartRead(byDevideAddr);

    for(nIndex = 0; nIndex < nLength; nIndex++)
    {
        *(pBuf + nIndex) = I2C_ReadByte();
    }
    UCB0CTL1 |= UCTXNACK;
    I2C_Stop();
    I2C_ReadByte();
    __delay_cycles(15);
    I2C_Stop();
}


/*--------------------------------------------------------------------------------
Function    : I2C_WriteData
Purpose     : Write Multi byte to register of slave device
Parameters  : pData - Pointer to data
                  byDevideAddr  - I2C address of slave device
                  byRegister    - Register address need to read data.
Return      : None
--------------------------------------------------------------------------------*/

VOID I2C_WriteData(uint8_t *pData, uint8_t byDevideAddr, uint8_t byRegister, uint8_t nLength)
{
    BYTE nIndex;
    //I2C_Init();
    I2C_StartWrite(byDevideAddr, byRegister);
    for(nIndex = 0; nIndex < nLength; nIndex++)
    {
        I2C_WriteByte(*(pData + nIndex));
    }
    __delay_cycles(15);
    I2C_Stop();
}

//-------------------------------------------------------------------
#ifdef NOCODE

// see header file, this routine is used for both tx and rx (i2c_rx and i2c_tx defs)
void i2c_trans(u8 size, u8 id, u8 *buffer, u16 address)
{
    i2c_data.state = (id & READ) | ADDRTR; // byte counter
    if (size & 0x01) i2c_data.state |= ADDR16;
    i2c_data.count = (size >> 1) + 1; // byte counter
    i2c_data.address = address; // byte counter
    i2c_data.buffer = buffer; // byte counter
    UCB0I2CSA = id >> 1; // Slave Address 01101000 0x68 RTC //Slave Address 01010000 0x50 EEPROM
    UCB0CTL1 |= UCTR + UCTXSTT; // I2C TX, start condition
    __bis_SR_register(CPUOFF + GIE); // Enter LPM0 w/ interrupts
    // Remain in LPM0 until all data is transferred
}

u8 i2c_int(void) {
    if (i2c_data.state == WRITE) {
        if (i2c_data.count > 0) { //Check TX byte counter
            UCB0TXBUF = *i2c_data.buffer++; // Load TX buffer
            i2c_data.count--; //Decrement TX byte counter
        } else if (i2c_data.count == 0) { //last byte transferred
            UCB0CTL1 |= UCTXSTP; //I2C stop condition
            while (UCB0CTL1 & UCTXSTP); //Ensure stop condition got sent
            IFG2 &= ~UCB0TXIFG; //Clear USCI_B0 TX int flag
            return 1; //Exit LPM0
        }
    } else if (i2c_data.state == READ) {
        *i2c_data.buffer++ = UCB0RXBUF;
        i2c_data.count--; //Decrement RX byte counter
        if (i2c_data.count == 1) { //Check RX byte counter, 1 byte remaining
            UCB0CTL1 |= UCTXSTP; // I2C stop condition
        }
        if (i2c_data.count == 0) { //Check RX byte counter, last byte received
            while (UCB0CTL1 & UCTXSTP); // Ensure stop condition got sent
            return 1; // Exit LPM0
        }
    } else if (i2c_data.state & ADDR16) { // high byte address transmit
        UCB0TXBUF = swap_bytes(i2c_data.address);
        i2c_data.state &= ~ADDR16;
    } else if (i2c_data.state & ADDRTR) { // low byte address transmit
        UCB0TXBUF = i2c_data.address;
        i2c_data.state &= ~ADDRTR;
        if (i2c_data.state) { // repeated Start for RX
            i2c_data.state |= REPSTT;
        }
    } else if (i2c_data.state & REPSTT) { //  repeated start required
        i2c_data.state &= ~REPSTT;
        UCB0CTL1 &= ~UCTR; // I2C RX
        UCB0CTL1 |= UCTXSTT; // I2C repeated Start condition for RX
        IFG2 &= ~UCB0TXIFG; //Clear USCI_B0 TX int flag
    }
    return 0;
}

u8 i2c_eint(void) {
    if (UCB0STAT & UCNACKIFG) { // send STOP if slave sends NACK
        UCB0CTL1 |= UCTXSTP;
        UCB0STAT &= ~UCNACKIFG;
        return 1;
    }
    return 0;
}

#endif




// Another I2C

#ifdef NOCODE

#include <msp430g2553.h>
#include <inttypes.h>

//Gyro Memory addresses;
char GYRO_XOUT_H = 0x1D;
char GYRO_XOUT_L = 0x1E;
char GYRO_YOUT_H = 0x1F;
char GYRO_YOUT_L = 0x20;
char GYRO_ZOUT_H = 0x21;
char GYRO_ZOUT_L = 0x22;
char TEMP_OUT_H = 0x1B;
char TEMP_OUT_L = 0x1C;
char itgAddress = 0x69;

//Other Gyro Addresses
char WHO_AM_I = 0x00;   // Gyro register where it helds the slave address 0x69 | 0x68
char SMPLRT_DIV= 0x15; // Gyro register where it helds the divider value for sample rate
char DLPF_FS = 0x16;  // Gyro register where it helds the low pass filter config

//Gyro configuration constants
char DLPF_CFG_0 = 1<<0;        // 1
char DLPF_CFG_1 = 1<<1;       // 10
char DLPF_CFG_2 = 1<<2;      // 100
char DLPF_FS_SEL_0 = 1<<3;  // 1000
char DLPF_FS_SEL_1 = 1<<4; //10000

void Transmit(char registerAddr, char data);
void init_I2C(void);
void initGyro(void);
int i2c_notready();
uint8_t Receive(char registerAddr);
void setSampleRateDivider(int div);
void setLowPassFilter(int config);
int8_t readLowPassFilter();
int8_t readSampleRateDivider();
int8_t readWhoAmI();
int16_t readX();
int16_t readY();
int16_t readZ();

/*
* This function is used to initialize USCI_B I2C module. Since our SMCLK uses 1 MHz,
* our divide register will be 10 to make it ~100kHz.
*
*/
void init_I2C(void) {
         UCB0CTL1 |= UCSWRST;                      // Enable SW reset
         UCB0CTL0 = UCMST + UCMODE_3 + UCSYNC;     // I2C Master, synchronous mode
         UCB0CTL1 = UCSSEL_2 + UCSWRST;            // Use SMCLK, keep SW reset
         UCB0BR0 = 10;                             // fSCL = 1Mhz/10 = ~100kHz
         UCB0BR1 = 0;
         UCB0I2CSA = itgAddress;                   // Slave Address is 069h
         UCB0CTL1 &= ~UCSWRST;                     // **Initialize USCI state machine**
         IE2 |= UCB0RXIE + UCB0TXIE;    // Enable RX and TX interrupt
}

/*
* This function is used to write the configuration values into the gyroscope
* configuration registers.
*/
void initGyro(void) {
       setLowPassFilter((DLPF_FS_SEL_0|DLPF_FS_SEL_1|DLPF_CFG_0));
       setSampleRateDivider(9);
}

/*
* This function is used to check if I2C bus is busy or not.
* 1: busy - 0: not busy
*/
int i2c_notready(){
       if(UCB0STAT & UCBBUSY) return 1;
       else return 0;
}
/*
* This function is used to receive a single byte from a specific register
*
*/
uint8_t Receive(char registerAddr){
       uint8_t receivedByte;
       while (UCB0CTL1 & UCTXSTP);             // Ensure stop condition got sent
       UCB0CTL1 |= UCTR + UCTXSTT;             // I2C start condition with UCTR flag for transmit
       while((IFG2 & UCB0TXIFG) == 0);     //UCB0TXIFG is set immidiately
       UCB0TXBUF = registerAddr;           //write registerAddr in TX buffer
       while((IFG2 & UCB0TXIFG) == 0);     // wait until TX buffer is empty and transmitted
       UCB0CTL1 &= ~UCTR ;                // Clear I2C TX flag for receive
       UCB0CTL1 |= UCTXSTT + UCTXNACK;    // I2C start condition with NACK for single byte reading
       while (UCB0CTL1 & UCTXSTT);             // Start condition sent? RXBuffer full?
       receivedByte = UCB0RXBUF;
       UCB0CTL1 |= UCTXSTP;                    // I2C stop condition
       return receivedByte;
}

/*
* This function is used to transmit a single byte into a specific register
*
*/
void Transmit(char registerAddr, char data){
   while (UCB0CTL1 & UCTXSTP);             // Ensure stop condition got sent
   UCB0CTL1 |= UCTR + UCTXSTT;             // I2C start condition with UCTR flag for transmit
   while((IFG2 & UCB0TXIFG) == 0);         //UCB0TXIFG is set immidiately
   UCB0TXBUF = registerAddr;               //write registerAddr in TX buffer
   while((IFG2 & UCB0TXIFG) == 0);         // wait until TX buffer is empty and transmitted
   UCB0TXBUF = data;                       //Write data in register
   while((IFG2 & UCB0TXIFG) == 0);         // wait until TX buffer is empty and transmitted
   UCB0CTL1 |= UCTXSTP;                    // I2C stop condition
   IFG2 &= ~UCB0TXIFG;                     // Clear TX interrupt flag
}

/*
* This function is used to read X motion rate. Note that Gyro has 16 bit value which
* divided into 2 registers with 8 bits. Reading the high register, left shifting with 8
* and OR with low register will result in 16 bit motion value in 2's complement representation.
*/
int16_t readX() {
       int16_t xRate = 0;
       xRate = Receive(GYRO_XOUT_H);
       xRate = xRate << 8;
       xRate |= Receive(GYRO_XOUT_L);
       return xRate;
}
/*
* This function is used to read Y motion rate. Note that Gyro has 16 bit value which
* divided into 2 registers with 8 bits. Reading the high register, left shifting with 8
* and OR with low register will result in 16 bit motion value in 2's complement representation.
*/
int16_t readY() {
       int16_t yRate = 0;
       yRate = Receive(GYRO_YOUT_H);
       yRate = yRate << 8;
       yRate |= Receive(GYRO_YOUT_L);
       return yRate;
}
/*
* This function is used to read Z motion rate. Note that Gyro has 16 bit value which
* divided into 2 registers with 8 bits. Reading the high register, left shifting with 8
* and OR with low register will result in 16 bit motion value in 2's complement representation.
*/
int16_t readZ() {
       int16_t zRate = 0;
       zRate = Receive(GYRO_ZOUT_H);
       zRate = zRate << 8;
       zRate |= Receive(GYRO_ZOUT_L);
       return zRate;
}
/*
* This function is used to read the value from WHO_AM_I register. It should return either 0x69
* or 0x68 depending on the circuit on gyroscope.
*/
int8_t readWhoAmI() {
       int8_t whoami = 0;
       whoami = Receive(WHO_AM_I);
       return whoami;
}
/*
* This function is used to read SampleRateDivider register. It should return the configuration value
* that you provided in initGyro().
*/
int8_t readSampleRateDivider() {
       int8_t srd = 0;
       srd = Receive(SMPLRT_DIV);
       return srd;
}
/*
* This function is used to read readLowPassFilter register. It should return the configuration value
* that you provided in initGyro().
*/
int8_t readLowPassFilter() {
       int8_t lpf = 0;
       lpf = Receive(DLPF_FS);
       return lpf;
}
/*
* This function is used to write the configuration value inside SMPLRT_DIV register of gyro.
* Default= fs/(9+1) = 800hz
*/
void setSampleRateDivider(int div) {
   Transmit(SMPLRT_DIV, div);
}
/*
* This function is used to write the configuration value inside DLPF_FS register of gyro.
* fs = 8khz
* FS_SEL = 2000 dagree/sec
*/
void setLowPassFilter(int config) {
   Transmit(DLPF_FS, config);
}
#endif

#endif
