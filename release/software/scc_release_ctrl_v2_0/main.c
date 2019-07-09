#include <msp430.h> 
#include <stdint.h>
#include <time.h>
#include "uart.h"
#include "i2c.h"
#include "rtc_m41t62.h"

// 7.3uA with comm_energy harven=0, harv_en=1 59uA in room light, with LEDlight -125uA (charging)
// Mag swipe to turn on, inrush to 100mA plus, then 2.1mA, with radio comms around 16mA

#define PRINT_VERSION      "\r\n\r\nSharkCafeCam Release v2.0.3 \r\n"         // deploy 1.1.0,  deploy v1.0.1 (v1.0.9.2 deployed x6) unless bugs are found  27 Apr 2017
#define PRINT_AUTHOR       "Thom Maughan, MBARI, 9 Apr 2018\r\n\r\n"

// v1.0.9.2  Built 6 deployment units with scc_release_v2_2 and scc_comm_energy_2_0 (mod with cut and 40K on VIN to CT05)
//   Manufacturing test procedure:
//      1) burn firmware (run, then terminate in ccs v7,
//      2) short rst to gnd on jtag to reset, watch for version string on HC-12 connected to ftdi serial on teraterm
// v1.1.0    TODO:  uart passivate to drop txd vampire current (gpio as output lo), refine bat/sol/cont scaling
// v2.0   scc_release_v2_3 and scc_comm_energy_3_0 (wireless power).   scc_comm_energy should have SW3 and R4 removed and a jumper from top of C11 to top of COMM2 connector (trace not connected on PCB)
// v2.0.2  build 6 to 10 for deploy at Shark Cafe off RV Falkor

//#define WDT_GOSLEEP_TIME    250L         // 32ms * 2500 = 80 seconds, 250 = 8 seconds
#define WDT_GOSLEEP_TIME    2000L   // THOM TODO: make this 2000 for deployment      // 32ms * 2500 = 80 seconds, 1000 = 32 seconds, 2000 = 64 seconds
#define RELEASE_TIME_SECS    25     // was 15

//Continuity Check (10 msec):  3 ohm nichrome, vbat range 3.6 to 4.2v
// 30ohm: 4.2v = I * 33 ohm => I = 127mA => vdrop = 0.127*30 = 3.82v  assuming battery does not droop
// 30ohm: 3.6v = I * 33 ohm => I = 109mA => vdrop = 0.109*30 = 3.27v  assuming battery does not droop

// Measure vbat, turn on REL_LO_CONT

// LO_REL  | HI_REL_LO_CONT  |  Action
//--------------------------------------------------------------------------------------------------
//    0    |      0          |  NOTHING
//    0    |      1          |  Measure VCONT - continuity check of Nichrome and connections (~120mA)
//    1    |      0          |  Measure VSOL, VBAT
//    1    |      1          |  RELEASE - 1amp to the 3 ohm Nichrome release (melt fishing line)

// RTC STmicro M41T62
// Address b7=1,b6=1,b5=0,b4=1,b3=0,b2=0,b1=0, b0=R/W
//#define M41T62_I2CADDR  0x68        // I2C address

// PORT1
#define HARV_EN_LO      BIT0        // O Active Lo, turns on Energy Harvesting, Hi turns on VSOL to VBAT shunt
#define RXD             BIT1        // I SFR rxd
#define TXD             BIT2        // O SFR txd
#define VBATMON         BIT3        // I ADC VBAT monitor   0.5x actual
#define VSOLMON         BIT4        // I ADC VSOL monitor   0.5x actual
#define VCONTMON        BIT5        // I ADC measure continuity of nichrome  0.5x actual
#define SCL             BIT6        // SCL to M41T62 RTC
#define SDA             BIT7        // SDA to M41T62 RTC



// PORT2
#define HI_REL_LO_CONT  BIT0        // O Hi = Release (turns on high side and lo side switch to put 1A thru nichrome wire), Lo = Continuity check
#define ALARM_LO        BIT1        // I interrupt input
#define LO_REL          BIT2        // Active hi to turn on lo side of release, also asserted for VSOL and VBAT measurements
#define VOUT_ACT        BIT3        // Measure 3.3v
#define VOUT_EN         BIT4        // Turn on 3.3v buck regulator on COMM_ENERGY board
#define SPARE0          BIT5        // Unused
#define LED0            BIT6        // GREEN
#define LED1            BIT7        // RED


int handle_serial_comm(void);
int handle_release(void);
int handle_sleep(void);
void sleep_now(void);

void display_menu(int level);
void display_help(void);

void reset_sleep_timer(void);
void stop_sleep_timer(void);
void start_sleep_timer(void);

void print_battery(void);
void print_solar(void);
void print_continuity(void);

void gpio_harv_en_lo(int state);
//void gpio_vout_en(int state);
void gpio_lo_rel(int state);
void gpio_hi_rel_lo_cont(int state);
void gpio_vout_en_input(void);
void gpio_vout_en_output(int state);
int gpio_vout_act(void);

void led1_toggle(void);
void led1_on(void);
void led1_off(void);
void led0_toggle(void);
void led0_on(void);
void led0_off(void);

extern void adc_init(void);
extern void adc_read(unsigned int chan);
extern void adc_read_temperature(void);
extern void adc_read_battery(void);
extern void adc_read_solar(void);
extern void adc_read_cont(void);
extern void adc_read_vcc(void);

extern void uart_init (unsigned long baud);           // defd in uart.c
extern void uart_tx_byte(uint8_t txByte);
extern void uart_tx_str(uint8_t *dataPtr);
extern int uart_rx_rdy(void);
extern uint8_t uart_rx_byte(void);
extern void uart_tx_byte_ascii(uint8_t txByte);
extern void uart_tx_uint_ascii(uint16_t val);
extern void uart_tx_crlf(void);
extern void uart_passivate(void);
extern void uart_activate(void);

extern void uart_disable_interrupt(void);
extern void uart_enable_interrupt(void);

uint16_t state_commandkey(uint8_t rxByte);
uint16_t state_getbuf(uint8_t rxByte);
void rx_statevar_init();

extern void i2c_init(void);
extern void i2c_tx(unsigned char, unsigned char *, unsigned char, unsigned char, unsigned char, unsigned char);
extern void i2c_rx(unsigned char, unsigned char *, unsigned char, unsigned char, unsigned char, unsigned char);
extern void delay_usec(uint16_t microSec);

extern void rtc_init(void);
extern void rtc_print_time(void);
extern int rtc_set_time(uint8_t *buf);
extern uint8_t rtc_read_alarmFlag(void);
extern void rtc_print_all_regs(void);
extern void rtc_print_alarm(void);
extern void  rtc_addr_ptr_reset(void);
extern int rtc_set_alarm(uint8_t *buf);

void print_firmware_version(void);

void release_fire(uint16_t);
void release_test(uint16_t);



unsigned char txdata[7] = {0x20, 0x30, 0x20, 0x04, 0x17, 0x10, 0x12};
unsigned char rxdata[30];

//char code my_date[] = "Compile Date = " __DATE__;
//char code my_time[] = "Compile Time = " __TIME__;

#define ST_INIT         0
#define ST_COMMANDKEY   1            // single letter commands
#define ST_GETBUF       2            // CR-LF terminated ascii characters (with backspace and delete support)

#define MAX_SIZEOF_RXBUF 32
typedef struct
{
    uint16_t state;
    uint16_t cmd;
    uint16_t indx;
    uint16_t cnt;
    uint8_t  buf[MAX_SIZEOF_RXBUF+2];
} commVar;


commVar rx;
uint8_t   rxByte;

#define CMD_SETTIME     0
#define CMD_SETALARM    1
#define CMD_MISSIONPREP 2
#define CMD_RELEASE     3
#define CMD_RELEASETIME 4

struct tm *timeinfo;

volatile uint16_t rtcAlarmFlg = 0;       // P2.1 is ALARM_LO, ISR sets the rtcAlarmFlg
volatile uint32_t wdtCounter = 0L;
volatile int wdtGoSleep = 0;
volatile int wdtActivateSleep = 0;
volatile int magnetSwipe = 0;

volatile int voutEnDir;

extern volatile uint16_t adcValue;           // defd in adc.c
extern volatile uint16_t adcRdy;

uint16_t  batVal;
uint16_t  solVal;

float floatVal;

volatile uint16_t   deployFlg = 0;       // this global governs the test/release mode (=0 will test, =1 will fire release)
volatile uint16_t   releaseTimeSec = RELEASE_TIME_SECS;        // Initialize with default


#define INPUT_DIR       0
#define OUTPUT_DIR      1

/************************************************************************/
/* Function    :                                                        */
/* Purpose     :                                                        */
/* Input       :                                                        */
/* Outputs     :                                                        */
/************************************************************************/
void main(void)
{

    //uint8_t   rxByte;

    // Disable watchdog
    WDTCTL = WDTPW + WDTHOLD;

    // Use 1 MHz DCO factory calibration
    DCOCTL = 0;
    BCSCTL1 = CALBC1_1MHZ;
    DCOCTL = CALDCO_1MHZ;       //SMCLK = DCO = 1MHz

    // Initialize Port bits as GPIO here, then call module inits which setup special function as needed

    // PORT1
    // P1.0 = HARV_EN_LO, P1.1 = RXD, P1.2 = TXD, P1.3 = VBATMON
    // P1.4 = VSOLMON, P1.5 = VCONTMON, P1.6 = SCL, P1.7 = SDA
    P1DIR |= BIT0;                          // P1.0 = HARV_EN_LO OUTPUT
    P1OUT &= ~BIT0;                         // P1.0 = 0 (HARV_EN_LO active)
    //P1SEL |= BIT1 | BIT2 | BIT6 + BIT7;     // P1.1 = RXD, P1.2=TXD, P1.6 = SCL, P1.7 = SDA
    //P1SEL2 |= BIT1 | BIT2 | BIT6 + BIT7;    // NOTE: not sure why on P1SEL2 - need to read datasheet


    // PORT2
    // P2.0 = HI_REL_LO_CONT, P2.1=ALARM_LO, P2.2=LO_REL, P2.3=VOUT_ACT
    // P2.4=VOUT_EN, P2.5=SPARE0, P2.6=LED0 Green, P2.7=LED1 Red
    //P2SEL &= ~(LED0 + LED1);            // switch off special function of XIN, make it GPIO
    P2SEL &= ~(LED0 | LED1);            // switch off special function of XIN, make it GPIO
    P2DIR |= HI_REL_LO_CONT | LO_REL | VOUT_EN | LED0 | LED1;  // Outputs: P2.0, P2.2, P2.4, P2.6, P2.7
    P2OUT &= ~(HI_REL_LO_CONT | LO_REL | VOUT_EN | LED0 | LED1);   // VOUT_EN as an output, write as low (it will be an input later)
    voutEnDir = OUTPUT_DIR;                      // global var for vout_en direction (1 is Output)

    // Inputs
    P2DIR &= ~(ALARM_LO | VOUT_ACT);    // Inputs: P2.1, P2.3
    //P2DIR &= ~(ALARM_LO | VOUT_EN | VOUT_ACT);    // Inputs: P2.1, P2.3, P2.4
    //voutEnDir = 1;                      // global var for vout_en direction (0 is input)

    // Interrupt input from M41T62 RTC = ALARM_LO
    P2REN |= ALARM_LO;                  // P2.1 pullup (can eliminate on board pullup resistor and use port bit pullup)
    P2OUT |= ALARM_LO;                  // write output register to 1 for pullup
    P2IES |= ALARM_LO;                  // P2.1 Hi/lo edge  P2IES=1 means H->L transition
    P2IFG &= ~ALARM_LO;                 // P2.1 IFG cleared
    P2IE |= ALARM_LO;                   // P2.1 interrupt enabled

    P2REN &= ~VOUT_ACT;                  // P2.3 pullup (can eliminate on board pullup resistor and use port bit pullup)  NO PULLUP - 100k problem
    P2OUT &= ~VOUT_ACT;                  // write output register to 1 for pullup
    P2IES &= ~VOUT_ACT;                  // P2.3 Hi/lo edge  P2IES=0 means L->H transition
    P2IFG &= ~VOUT_ACT;                 // P2.3 IFG cleared
    P2IE |= VOUT_ACT;                   // P2.3 interrupt enabled

    /* The code below is to save power on 20-pin packages */
    P3DIR = 0;                                // All Port 3 pins initialized to be inputs
    P3REN = 255;                              // All Port 3 pull up/down resistors enabled
    P3OUT = 255;                              // All Port 3 pins are pulled up

    WDTCTL = WDT_MDLY_32;           // Set Watchdog Timer interval to ~32ms
    IE1 |= WDTIE;                   // Enable WDT interrupt

    //------------ Configuring the UART(USCI_A0) ----------------//
    uart_init(4800L);                // HC-12 RF module - 4800,2400,1200 in FU2 mode  11dbm (12mW)
    rx.state = ST_INIT;

    i2c_init();              // i2c_init(M41T62_I2CADDR);          // SlaveAddr = 0x68  defd in i2c.c

    rtc_init();

    adc_init();            // defd in adc.c


    //TACCTL0 = CCIE;                    // TACCR0 interrupt enabled
    //TACTL = TASSEL_2 + MC_2;           // SMCLK, contmode

    __enable_interrupt();                   //Enable global interrupt  //_BIS_SR(GIE);

    gpio_harv_en_lo(0);  // turn on energy harvester (BQ comm_energy v1) (turn off Solar switch)

    //gpio_vout_en(0);     // turn on 3.3v supply for rf comms   (DELETE THIS LINE FOR PRODCTION)
    //gpio_vout_en(1);     // turn on 3.3v supply for rf comms   (DELETE THIS LINE FOR PRODCTION)
    //gpio_vout_en_output(0);     // turn off 3.3v regulator on comm_energy board
    //gpio_vout_en_input();

    gpio_vout_en_output(1);     // turn on 3.3v for RF transmit string output

    __delay_cycles(50000);     //100000 works, 10000 is missing some chars
    uart_tx_str(PRINT_VERSION);
    uart_tx_str(PRINT_AUTHOR);
    __delay_cycles(10000);      // 10msec


    led0_off();
    led1_off();
    wdtGoSleep = 1;
    magnetSwipe = 0;

    //gpio_vout_en_output(0);     // DEBUG turn off 3.3v   This line blocks programming until first sleep cycle

    while(1)
    {
        led1_on();          // watchdog timer puts processor to sleep after 60 seconds of inactivity

        handle_release();

        handle_serial_comm();

        handle_sleep();

    } // while(1)

} // main()

/************************************************************************/
/* Function    : handle_sleep()                                         */
/* Purpose     : if nothing todo, go to sleep after wdt timer           */
/* Input       :                                                        */
/* Outputs     :                                                        */
/************************************************************************/
int handle_sleep(void)
{
    if(wdtActivateSleep == 1)
    {
        uart_tx_str("Sleeping...\r\n\r\n");     // DEBUG

        wdtActivateSleep = 0;

        led1_off();
        gpio_vout_en_output(0);     // turn off 3.3v regulator on comm_energy board
        gpio_vout_en_input();       // reconfig as input

        uart_disable_interrupt();
        __delay_cycles(10000);      // maybe not needed, but a small 10ms delay to allow settling
        uart_passivate();           // turn special fxn into gpio to keep from powering the radio


        //_BIS_SR(LPM3_bits + GIE); // Enter LPM3 w/interrupt (CPU, MCLK, SMCLK and DCO are disabled, DC gen disabled, ACLK is active
        _BIS_SR(LPM4_bits + GIE); // Enter LPM4 w/interrupt for gpio wakeup (CPU and all clocks are disabled)

        gpio_vout_en_output(1);     // turn on regulator for rf comm
        __delay_cycles(100);
        uart_activate();            // gpio reconfig as uart

        __delay_cycles(40000);
        uart_enable_interrupt();
        uart_tx_str("Awake!\r\n");     // DEBUG
        if(magnetSwipe == 1)
        {
            uart_tx_str("Magnet Swipe\r\n");
        }
        return(1);
    }

    return(0);
}


void sleep_now(void)
{
    wdtActivateSleep = 1;
}


/************************************************************************/
/* Function    : handle_release()                                       */
/* Purpose     : Turn on 1.1amps to nichrome for a period of time       */
/* Input       :                                                        */
/* Outputs     :                                                        */
/************************************************************************/
int handle_release(void)
{
    int retVal = 0;
    uint16_t indx;

    if(rtcAlarmFlg == 1)        // set by PORT2 isr
    {
        reset_sleep_timer();

        uart_tx_str("\r\nAlarm Active\r\n");
        rtc_print_time();

        if(deployFlg == 1)
        {
            uart_tx_str("\r\nRelease Fired!\r\n");

            release_fire(releaseTimeSec);    //release_fire(RELEASE_TIME_SECS);  new 5 Apr 2018

            uart_tx_str("\r\nRelease Done\r\n");
        }
        else
        {
            uart_tx_str("\r\nRelease Testing\r\n");

            release_test(releaseTimeSec);        //release_test(RELEASE_TIME_SECS);        // was 10  new 5 Apr 2018

            uart_tx_str("\r\nRelease Test Done\r\n");
        }

        retVal = 1;

        rtcAlarmFlg = 0;            // set sema4 to indicate rtcAlarm has fired
        led0_off();
        rtc_read_alarmFlag();   // read flags to clear alarm
        __delay_cycles(100);
        rtc_addr_ptr_reset();

        for(indx=0; indx<10; indx++)
        {
            __delay_cycles(1000000);
            if(P2IN&ALARM_LO == 0x00)
            {
                uart_tx_str("ALARM is low, this is an error!\r\n");
            }
            else
            {
                //uart_tx_str("rtc ALARM bit is high (off), as it should be\r\n");
                uart_tx_str("rtc ALARM is off, as it should be\r\n");
                break;
            }
        }

    }

    return(retVal);
}

/************************************************************************/
/* Function    :                                                        */
/* Purpose     :                                                        */
/* Input       :                                                        */
/* Outputs     :                                                        */
/************************************************************************/
int handle_serial_comm(void)
{
    int retVal = 0;

    if(uart_rx_rdy() == 1)
    {
        reset_sleep_timer();

        retVal = 1;
        rxByte = uart_rx_byte();

        switch(rx.state)
        {
        case ST_INIT:
            rx_statevar_init();
            rx.state = ST_COMMANDKEY;
            //break;                        // drop right into commandkey
        case ST_COMMANDKEY:
            rx.state = state_commandkey(rxByte);
            break;
        case ST_GETBUF:
            rx.state = state_getbuf(rxByte);
            break;
        default:
            rx.state = ST_INIT;
            break;
        } // switch rx.state

    } // if uart

    return(retVal);
}



/************************************************************************/
/* Function    : state_commandkey                                             */
/* Purpose     :                           */
/* Input       : rx byte                               */
/* Outputs     :                                      */
/************************************************************************/
uint16_t state_commandkey(uint8_t rxByte)
{
    uint16_t indx;

    switch(rxByte)
    {

    case 'a':
    case 'A':
        uart_tx_str("set alarm\r\nMM.DD HH:MM:SS\r\n");
        rx.cmd = CMD_SETALARM;           // call rtc_set_alarm
        rx.state = ST_GETBUF;
        rx_statevar_init();
        break;



#ifdef TESTCODE
    case 'b':
    case 'B':
        rtc_print_all_regs();
        break;
#endif
    case 'B':
    case 'b':
        gpio_lo_rel(1);                 // P2.2 Turn on low side FET
        __delay_cycles(100000);
        print_battery();
        gpio_lo_rel(0);
#ifdef NOCODE
        adc_read_battery();

        // 600 is 3630 mV
        for(indx=0; indx<1000; indx++)      // while(1)
        {
            if(adcRdy == 1)
            {
                gpio_lo_rel(0);         // LO_REL is required to read battery and solar

                uart_tx_str("battery: ");
                uart_tx_uint_ascii(adcValue);

                //floatVal = (2.033 * adcValue) + 2410;       // 836x + y = 4110,   600x + y = 3630  -> x=2.033, y=2410
                floatVal = (4.075 * adcValue) + 703;       // 836x + y = 4110,   716x + y = 3621  -> x=2.033, y=2410

                //floatVal = (float)adcValue;
                //floatVal = floatVal * 5.98;   //6.05;        // 3.66 should be floatVBAT;  600 counts is equal to a battery of 3.630v  factor is 6.05

                adcValue = (uint16_t)floatVal;
                uart_tx_str(",  mV: ");
                uart_tx_uint_ascii(adcValue);

                uart_tx_crlf();
                break;
            }
        }
#endif

        break;

    case 'C':
    case 'c':
        gpio_hi_rel_lo_cont(1);
        __delay_cycles(100000);
        print_continuity();
        gpio_hi_rel_lo_cont(0);

#ifdef NOCODE
        adc_read_cont();
        for(indx=0; indx<1000; indx++)      // while(1)
        {
            if(adcRdy == 1)
            {
                gpio_hi_rel_lo_cont(0);
                uart_tx_str("continuity: ");
                uart_tx_uint_ascii(adcValue);

                floatVal = (float)adcValue;
                floatVal = (floatVal/1024 * 3.66) * 1000;        // 3.66 should be floatVBAT;
                adcValue = (uint16_t)floatVal;
                uart_tx_str(",  mV: ");
                uart_tx_uint_ascii(adcValue);
                uart_tx_crlf();
                break;
            }
        }
#endif
        break;

    case 'D':
    case 'd':
        gpio_lo_rel(1);                 // P2.2 Turn on low side FET
        __delay_cycles(100000);
        print_solar();
        gpio_lo_rel(0);
#ifdef NOCODE
        gpio_lo_rel(1);                 // P2.2 Turn on low side FET
        __delay_cycles(100000);
        adc_read_solar();
        for(indx=0; indx<1000; indx++)      //while(1)
        {
            if(adcRdy == 1)
            {
                gpio_lo_rel(0);
                uart_tx_str("solar: ");
                uart_tx_uint_ascii(adcValue);
                uart_tx_crlf();
                break;
            }
        }
        gpio_lo_rel(0);
#endif
        break;
    case 'e':
        uart_tx_str("energy harv_en=0, solar disabled\r\n");
        gpio_harv_en_lo(0);  // turn off solar switch
        break;
    case 'E':
        uart_tx_str("energy harv_en=1, solar enabled (disable in dark)\r\n");
        gpio_harv_en_lo(1);  // turn on solar switch
        break;
    case 'F':
    case 'f':
        adc_read_temperature();
        for(indx=0; indx<1000; indx++)
        {
            if(adcRdy == 1)
            {

                uart_tx_str("temperature: ");
                uart_tx_uint_ascii(adcValue);
                uart_tx_crlf();
                break;
            }
        }

        break;
    case 'G':
    case 'g':
        deployFlg = 1;
        uart_tx_str("Release Armed and Ready to FIRE!!!!\r\n");
        break;
    case 'h':
    case 'H':
        //uart_tx_str("Hello World\r\n");
        display_help();
        gpio_vout_en_output(1);     // turn on comm_energy RF regulator (after magnet swipe an user key), watchdog shuts it off
        break;
#ifdef TESTCODE
    case 'i':
    case 'I':
        uart_tx_str("i2c rtc dev, call rtc_init\r\n");
        rtc_print_all_regs();
        rtc_init();
        rtc_print_all_regs();
        break;
#endif
    case 'l':
    case 'L':
        //uart_tx_str("L - Read alarm\r\n");
        gpio_vout_en_output(1);     // turn on comm_energy RF regulator (after magnet swipe an user key), watchdog shuts it off
        uart_tx_str("aLarm:    ");
        rtc_print_alarm();
        if(deployFlg == 1)
        {
            uart_tx_str("  Release armed to fire!\r\n");
        }
        else
        {
            uart_tx_str("  Release test mode, will not fire\r\n");
        }
        break;
    case 'm':
    case 'M':
        gpio_vout_en_output(1);     // turn on comm_energy RF regulator (after magnet swipe an user key), watchdog shuts it off
        display_menu(1);
        break;
    case 'n':
    case 'N':
        gpio_vout_en_output(1);     // turn on comm_energy RF regulator (after magnet swipe an user key), watchdog shuts it off
        uart_tx_str("Now:   ");
        rtc_print_time();
        uart_tx_crlf();
//        rtc_print_alarm();
        break;

    case 's':
    case 'S':
        //uart_tx_str("set time\r\nSS:MM:HH W DD.MM.YY\r\n");
        uart_tx_str("ranges: YY(00-99), MM(01-12), DD(01-31), W(1-7), HH(00-23), MM(00-59), SS(00-59)  \r\n");
        uart_tx_str("set time\r\nYY.MM.DD W HH:MM:SS\r\n");
        rx.cmd = CMD_SETTIME;           // call rtc_set_time
        rx.state = ST_GETBUF;
        rx_statevar_init();
        break;

    case 'r':
    case 'R':
        uart_tx_str("Enter 123 to fire the release! (ESC to abort)\r\n");
        rx.cmd = CMD_RELEASE;           // call release_fire
        rx.state = ST_GETBUF;
        rx_statevar_init();
        break;

    case 't':
    case 'T':
        uart_tx_str("Release Test\r\n");
        deployFlg = 0;
        uart_tx_str("Release disarmed for continuity test (100mA), Wait (melt) seconds is ");
        uart_tx_uint_ascii(releaseTimeSec);
        uart_tx_str("\r\n\r\n");

        //release_test(10);
        break;
#ifdef TESTCODE
    case 'U':
        uart_tx_str("vout enabled\r\n");
        gpio_vout_en_output(1);
        break;
    case 'u':
        uart_tx_str("vout disabled\r\n");
        gpio_vout_en_output(0);
        break;
#endif

    case 'v':
    case 'V':
        print_firmware_version();
        break;
    case 'w':
    case 'W':
        // get the release time in seconds
        uart_tx_str("Enter 1 to 999 for Release Wait (melt) time in seconds\r\n");
        rx.cmd = CMD_RELEASETIME;
        rx.state = ST_GETBUF;
        rx_statevar_init();
        break;

    case 'z':
    case 'Z':
        sleep_now();
        break;

    default:
        //if(rxByte < 128)
        //    uart_tx_byte(rxByte);

        if(rxByte >= ' ' && rxByte <= 'z')      // suppress garbage characters
        {
            uart_tx_byte(rxByte);
            uart_tx_crlf();
            display_menu(1);
        }
        break;
    }

    return(rx.state);

}


void print_firmware_version(void)
{
    uart_tx_str(PRINT_VERSION);
    uart_tx_str(PRINT_AUTHOR);

}

void print_solar(void)
{
    uint16_t indx;


    adc_read(INCH_3);               // INCH_3 is P1.3 VBATMON

     // 600 is 3630 mV
     for(indx=0; indx<1000; indx++)         // loop of a 1000 is not necessary, but better than while(1)
     {
         if(adcRdy == 1)
         {

             uart_tx_str("solar: ");
             uart_tx_uint_ascii(adcValue);

             floatVal = (5.875 * (float)adcValue);          // using scaling from battery measure

             adcValue = (uint16_t)floatVal;
             uart_tx_str(",  mV: ");
             uart_tx_uint_ascii(adcValue);

             uart_tx_crlf();
             break;
         }
     }

}

void print_battery(void)
{
    uint16_t indx;

    //adc_read_battery();
    adc_read(INCH_3);               // INCH_3 is P1.3 VBATMON

     // 600 is 3630 mV
     for(indx=0; indx<1000; indx++)         // loop of a 1000 is not necessary, but better than while(1)
     {
         if(adcRdy == 1)
         {
             //gpio_lo_rel(0);
             uart_tx_str("battery: ");
             uart_tx_uint_ascii(adcValue);

             //floatVal = (4.075 * adcValue) + 703;       // 836x + y = 4110,   716x + y = 3621  -> x=2.033, y=2410
             floatVal = (5.875 * (float)adcValue);

             //floatVal = (float)adcValue;
             //floatVal = floatVal * 5.98;   //6.05;        // 3.66 should be floatVBAT;  600 counts is equal to a battery of 3.630v  factor is 6.05
             adcValue = (uint16_t)floatVal;
             uart_tx_str(",  mV: ");
             uart_tx_uint_ascii(adcValue);

             uart_tx_crlf();
             break;
         }
     }
     //uart_tx_str(",  adc indx ");
     //uart_tx_uint_ascii(indx);
     //uart_tx_crlf();

}

// called when release is fired
void print_continuity(void)
{
    uint16_t indx;

    //read continuity
    adc_read(INCH_5);               // INCH_5 is P1.5 VCONTMON

    //gpio_hi_rel_lo_cont(1);

     // 600 is 3630 mV
     for(indx=0; indx<1000; indx++)
     {
         if(adcRdy == 1)
         {
             uart_tx_str("continuity: ");
             uart_tx_uint_ascii(adcValue);



             floatVal = (float)adcValue;
             floatVal = floatVal * 4.55;     // 550 is 2.5 ohms   - 4.55
             adcValue = (uint16_t)floatVal;
             uart_tx_str(",  milli-Ohm: ");
             uart_tx_uint_ascii(adcValue);

             uart_tx_crlf();
             break;
         }
     }
     //uart_tx_str(",  adc indx ");
     //uart_tx_uint_ascii(indx);
     //uart_tx_crlf();

}


void display_help(void)
{
    uart_tx_str("\r\n\r\n----------------- Prepare Mission Settings ----------------------------\r\n");
    uart_tx_str("Overview of steps:\r\n");
    uart_tx_str(" 1) 'T' set Test mode ('T')\r\n");
    uart_tx_str(" 2) 'A' set the release Alarm date/time for deployment\r\n    - 'L' to read the alarm\r\n");
    uart_tx_str(" 3) 'S' Set the time/date a minute or two before the release time/date\r\n    - 'N' to read now time\r\n");
    uart_tx_str(" 4) Watch and wait for the alarm to be tested, look for an info message\r\n    - the release will be electrically tested and will not fire,\r\n");
    uart_tx_str(" 5) 'S' Set the time/date to the correct date/time\r\n    - 'N' to check now time\r\n");
    uart_tx_str(" 6) 'G' set the Go deployment mode\r\n    - the release is armed to fire at the alarm time/date!\r\n");
    uart_tx_str(" 7) 'Z' sleep the processor to save power (go to sleep little baby...)\r\n");
}

void display_menu(int level)
{
    if(level == 0)
    {
        uart_tx_str("Menu\r\n");
        uart_tx_str("A - Alarm set\r\n");
        uart_tx_str("H - hello\r\n");
        uart_tx_str("I - i2c rtc dev\r\n");
        uart_tx_str("L - Read alarm\r\n");
        uart_tx_str("N - Now get rtc\r\n");
        uart_tx_str("S - Set Time\r\n");
        uart_tx_str("R - Release Fire!\r\n");
        uart_tx_str("T - Release Test\r\n");
    }

    if(level == 1)
    {
        uart_tx_str("Menu\r\n");
        uart_tx_str("H - Help with mission prep settings\r\n");
        uart_tx_str("A - Alarm set\r\n");
        uart_tx_str("L - Read aLarm\r\n");
        uart_tx_str("N - Now read time\r\n");
        uart_tx_str("S - Set Time\r\n");
        uart_tx_str("T - Test Mode - release will be tested, but wont burn\r\n");
        uart_tx_str("G - Go Deploy mode - release will fire\r\n\r\n");
        uart_tx_str("B - Battery (3600 to 4200 mv)\r\n");
        uart_tx_str("C - Continuity check (590 to 600)\r\n");
        uart_tx_str("D - Solar\r\n");
        uart_tx_str("W - Wait seconds for release melt (");
        uart_tx_uint_ascii(releaseTimeSec);
        uart_tx_str(")\r\n");
        uart_tx_str("F - Temperature\r\n");
        uart_tx_str("R - then 123<enter> for Release Fire!\r\n");
        uart_tx_str("Z - Sleep now\r\n\r\n");
    }

}

/************************************************************************/
/* Function    :                                                        */
/* Purpose     :                                                        */
/* Input       :                                                        */
/* Outputs     :                                                        */
/************************************************************************/
void rx_statevar_init(void)
{
    rx.indx = 0;
    rx.cnt = 0;
}

uint16_t state_getbuf(uint8_t rxByte)
{
    switch(rxByte)
    {
    case 0x0a:
    case 0x0d:
        rx.buf[rx.indx] = 0x00;
        uart_tx_byte(0x0d);
        uart_tx_byte(0x0a);
        uart_tx_str(&rx.buf[0]);
        uart_tx_byte(0x0d);
        uart_tx_byte(0x0a);
        rx.state = ST_COMMANDKEY;

        if(rx.cmd == CMD_RELEASE)
        {
            if(rx.buf[0] == '1' && rx.buf[1] == '2' && rx.buf[2] == '3')
            {
                uart_tx_str("\r\nRelease Fire!!!\r\n");
                release_fire(RELEASE_TIME_SECS);
                uart_tx_str("\r\nRelease Done\r\n");
            }
        }

        if(rx.cmd == CMD_SETTIME)
        {
            txdata[6] = (rx.buf[0]*16) + (rx.buf[1]&0x0f);      // Year 00-99
            txdata[5] = (rx.buf[3]*16) + (rx.buf[4]&0x0f);      // Month 01-12
            txdata[4] = (rx.buf[6]*16) + (rx.buf[7]&0x0f);      // Day 01-31
            txdata[3] = (rx.buf[9]&0x0f);                       // DOW 1-7
            txdata[2] = (rx.buf[11]*16) + (rx.buf[12]&0x0f);    // Hour 00-23
            txdata[1] = (rx.buf[14]*16) + (rx.buf[15]&0x0f);    // Minute 00-59
            txdata[0] = (rx.buf[17]*16) + (rx.buf[18]&0x0f);    // Second 00-59

            rtc_set_time(txdata);

            uart_tx_str("\r\nNow:   ");
            rtc_print_time();
            uart_tx_crlf();

        }

        if(rx.cmd == CMD_SETALARM)
        {

            txdata[0] = (rx.buf[0]*16) + (rx.buf[1]&0x0f);      // Month 01-12
            txdata[1] = (rx.buf[3]*16) + (rx.buf[4]&0x0f);      // Day 01-31
            txdata[2] = (rx.buf[6]*16) + (rx.buf[7]&0x0f);      // Hour 00-23
            txdata[3] = (rx.buf[9]*16) + (rx.buf[10]&0x0f);    // Minute 00-59
            txdata[4] = (rx.buf[12]*16) + (rx.buf[13]&0x0f);    // Second 00-59
            rx.buf[14] = 0x00;

            uart_tx_str("\r\nAlarm Set\r\n");
            rtc_set_alarm(txdata);

            rtc_print_alarm();

            if(deployFlg == 1)
            {
                uart_tx_str("  Release armed to fire!  Press T to disarm and test\r\n");
            }
            else
            {
                uart_tx_str("  Release in test mode, will not fire\r\n");
            }

            //uart_tx_str("\r\nNow\r\n");
            //rtc_print_time();
        }

        if(rx.cmd == CMD_MISSIONPREP)
        {

            txdata[0] = (rx.buf[0]*16) + (rx.buf[1]&0x0f);      // Month 01-12
            txdata[1] = (rx.buf[3]*16) + (rx.buf[4]&0x0f);      // Day 01-31
            txdata[2] = (rx.buf[6]*16) + (rx.buf[7]&0x0f);      // Hour 00-23
            txdata[3] = (rx.buf[9]*16) + (rx.buf[10]&0x0f);    // Minute 00-59
            txdata[4] = (rx.buf[12]*16) + (rx.buf[13]&0x0f);    // Second 00-59
            rx.buf[14] = 0x00;      // DEBUG

            rtc_set_alarm(txdata);

            uart_tx_str("\r\nAlarm Set\r\n");

            rtc_print_alarm();

            rtc_print_time();

        }



        if(rx.cmd == CMD_RELEASETIME)
        {
            // 012
            // 345  rx.indx=3
            if(rx.indx == 0)
            {
                // leave releaseTimeSec alone
            }
            else if(rx.indx == 1)
            {
                releaseTimeSec = (uint16_t)(rx.buf[0]&0x0f);

            }
            else if(rx.indx == 2)
            {
                releaseTimeSec = (uint16_t)((rx.buf[0]&0x0f)*10 + (rx.buf[1]&0x0f));
            }
            else
            {
                releaseTimeSec = (uint16_t)((rx.buf[0]&0x0f)*100 + (rx.buf[1]&0x0f)*10 + (rx.buf[2]&0x0f));
            }

            if((releaseTimeSec == 0) || (releaseTimeSec > 999))
            {
                releaseTimeSec = RELEASE_TIME_SECS;
            }
            uart_tx_str("release time (secs) = ");
            uart_tx_uint_ascii(releaseTimeSec);
            uart_tx_str("\r\n\r\n");


        }

        break;

    // case backspace:
    //
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
    case ' ':
    case '/':
    case ':':
    case '-':
    case '.':
        rx.buf[rx.indx++] = rxByte;
        if(rx.indx >= MAX_SIZEOF_RXBUF)
            rx.indx = MAX_SIZEOF_RXBUF-1;
        uart_tx_byte(rxByte);
        break;
    default:
        // abort time entry
        uart_tx_str("entry cancelled, re-enter command\r\n");
        rx.state = ST_COMMANDKEY;

        break;
    }

    return(rx.state);
}


/************************************************************************/
/* Function    :                                                        */
/* Purpose     :                                                        */
/* Input       :                                                        */
/* Outputs     :                                                        */
/************************************************************************/
void release_fire(uint16_t  secs)
{
    uint16_t indx;

    // P2.0 = HI_REL_LO_CONT, P2.1=ALARM_LO, P2.2=LO_REL, P2.3=VOUT_ACT
    P2OUT |= LO_REL;                // P2.2 Turn on low side FET
    P2OUT |= HI_REL_LO_CONT;      // P2.0 turn on high side switch and low side 30 ohm



    for(indx=0; indx<secs; indx++)
    {
        // reset watchdog
        reset_sleep_timer();  // atomic op

        // THOM TODO look for key to abort release

        //__delay_cycles(10000000);
        print_battery();
        __delay_cycles(1000000);
    }

    P2OUT &= ~BIT2;
    P2OUT &= ~BIT0;

}

/************************************************************************/
/* Function    :                                                        */
/* Purpose     :                                                        */
/* Input       :                                                        */
/* Outputs     :                                                        */
/************************************************************************/
void release_test(uint16_t  secs)
{
    uint16_t indx;

    led0_on();

    //P2OUT |= HI_REL_LO_CONT;      // turn on high side switch and low side 30 ohm
    gpio_hi_rel_lo_cont(1);
    __delay_cycles(100000);
    for(indx=0; indx<secs; indx++)
    {
        //__delay_cycles(10000000);
        // TODO:  measure and report continuity
        //print_battery();

        reset_sleep_timer();  // atomic op

        print_continuity();   // 550 is 2.5 ohms
        __delay_cycles(1000000);
    }
    gpio_hi_rel_lo_cont(0);
    //P2OUT &= ~HI_REL_LO_CONT;
    led0_off();

}


void mission_prep(void)
{
    // GET 'Y' or exit


}

/************************************************************************/
/* Function    : gpio_vout_en_input()                                       */
/* Purpose     : vout_en as input, magnet swipe by user turns on RF and then micro takes over vout_en, watchdog shuts it off */
/* Input       :                                                        */
/* Outputs     :                                                        */
/************************************************************************/
void gpio_vout_en_input(void)
{
    // THOM TODO - make it a pulldown input
    P2DIR &= ~VOUT_EN;

    //P2REN |= VOUT_EN;       // pull up / pull down due to last state of P2OUT  THOM new 2 May 2017
    P2REN &= ~VOUT_EN;         // pull down   new 5 Apr 2018

    voutEnDir = INPUT_DIR;          // INPUT = 0

}

/************************************************************************/
/* Function    : gpio_vout_en_output()                                       */
/* Purpose     : vout_en = 1 after magnet swipe keeps regulator on, =0 shuts off rf  */
/* Input       :                                                        */
/* Outputs     :                                                        */
/************************************************************************/
void gpio_vout_en_output(int state)
{
    P2DIR |= VOUT_EN;
    voutEnDir = OUTPUT_DIR;     // OUTPUT=1
    if(state == 0)
    {
        P2OUT &= ~VOUT_EN;
    }
    else
    {
        P2OUT |= VOUT_EN;
    }

}

int gpio_vout_act(void)
{
    if((P2IN &= VOUT_ACT) == VOUT_ACT)
    {
        return(1);
    }
    else
    {
        return(0);
    }
}

#ifdef NOCODE
/************************************************************************/
/* Function    : gpio_vout_en()                                       */
/* Purpose     : active hi, turn on vout for comm                        */
/* Input       :                                                        */
/* Outputs     :                                                        */
/************************************************************************/
void gpio_vout_en(int state)
{
    if(voutEnDir == 1)  // if output
    {
        // P2.4 is VOUT_EN active hit
        if(state == 0)
        {
            P2OUT &= ~VOUT_EN;          // BIT4
        }

        if(state == 1)
        {
            P2OUT |= VOUT_EN;
        }
    }

}
#endif

/************************************************************************/
/* Function    : gpio_harv_en_lo()                                       */
/* Purpose     : active lo, turn on energy harvester, hi turn on solar direct  */
/* Input       :                                                        */
/* Outputs     :                                                        */
/************************************************************************/
void gpio_harv_en_lo(int state)
{
    // P1.0 is HARV_EN_LO energy harvest enable active lo
    if(state == 0)
    {
        P1OUT &= ~BIT0;
    }

    if(state == 1)
    {
        P1OUT |= BIT0;
    }

}



void gpio_lo_rel(int state)
{
    // P2.2 is LO_REL
    if(state == 0)
    {
        P2OUT &= ~LO_REL;          // BIT2
    }

    if(state == 1)
    {
        P2OUT |= LO_REL;
    }
}

void gpio_hi_rel_lo_cont(int state)
{
    // P2.0 is HI_REL_LO_CONT
    if(state == 0)
    {
        P2OUT &= ~HI_REL_LO_CONT;          // BIT0
    }

    if(state == 1)
    {
        P2OUT |= HI_REL_LO_CONT;
    }
}



/************************************************************************/
/* Function    :                                                        */
/* Purpose     :                                                        */
/* Input       :                                                        */
/* Outputs     :                                                        */
/************************************************************************/
void led1_toggle(void)
{
    P2OUT ^= LED1;
}
void led1_on(void)
{
    P2OUT |= LED1;
}
void led1_off(void)
{
    P2OUT &= ~LED1;
}
void led0_toggle(void)
{
    P2OUT ^= LED0;
}
void led0_on(void)
{
    P2OUT |= LED0;
}
void led0_off(void)
{
    P2OUT &= ~LED0;
}

/************************************************************************/
/* Function    :                                                        */
/* Purpose     :                                                        */
/* Input       :                                                        */
/* Outputs     :                                                        */
/************************************************************************/
#pragma vector = PORT2_VECTOR
__interrupt void isrPORT2(void)
{
    _NOP();
    // P2.1 is configured as ALARM_LO input from RTC
    if ((P2IFG & ALARM_LO) == ALARM_LO)        // RTC Alarm has gone H->L
    {
        _BIC_SR(LPM3_EXIT); // wake up from low power mode
        //_BIC_SR(LPM4_EXIT); // wake up from low power mode

        // ALARM has fired (
        P2IFG &= ~ALARM_LO;        // Clear interrupt flag
        rtcAlarmFlg = 1;            // set sema4 to indicate rtcAlarm has fired


//        led0_on();
//        uart_tx_byte('A');
    }

    if ((P2IFG & VOUT_ACT) == VOUT_ACT)        // RTC Alarm has gone H->L
    {
        _BIC_SR(LPM3_EXIT); // wake up from low power mode
        //_BIC_SR(LPM4_EXIT); // wake up from low power mode

        // ALARM has fired (
        P2IFG &= ~VOUT_ACT;        // Clear interrupt flag
        magnetSwipe = 1;

    }

}

#ifdef NOCODE
#pragma vector=PORT1_VECTOR
__interrupt void Port_1(void)
{
    switch(__even_in_range(P1IV,16))
    {
    case 0: break; // No Interrupt
    case 2: break; // P1.0
    case 4: break; // P1.1
    case 6: break; // P1.2
    case 8: // P1.3
    flag = 1;
    break;
    case 10: break; // P1.4
    case 12: break; // P1.5
    case 14: break; // P1.6
    case 16: break; // P1.7
    }
}
#endif




void reset_sleep_timer(void)
{
    __disable_interrupt();
    wdtCounter = 0L;
    __enable_interrupt();
}


void stop_sleep_timer(void)
{
    __disable_interrupt();
    wdtGoSleep = 0;
    wdtCounter = 0L;
    __enable_interrupt();
}

void start_sleep_timer(void)
{
    __disable_interrupt();
    wdtGoSleep = 1;
    wdtCounter = 0L;
    __enable_interrupt();
}
/************************************************************************/
/* Function    :                                                        */
/* Purpose     :                                                        */
/* Input       :                                                        */
/* Outputs     :                                                        */
/************************************************************************/
// Watchdog Timer interrupt service routine
#pragma vector=WDT_VECTOR
__interrupt void watchdog_timer(void)
{
    if(wdtCounter >= WDT_GOSLEEP_TIME)      // 32ms * 2500 = 80 seconds, 250 = 8 seconds
    {
        wdtCounter = 0L;
        wdtActivateSleep = 1;       // Sema4 to main loop to active sleep
#ifdef NOCODE
        led1_off();
        gpio_vout_en_output(0);
        gpio_vout_en_input();
        _BIS_SR(LPM4_bits + GIE); // Enter LPM3 w/interrupt
#endif
    }
    else
    {
	// variable should be wdtSleepTimerEnable
        if(wdtGoSleep == 1)         // this variable gets set and 8 seconds later the processor sleeps
        {
            wdtCounter++;
        }
    }
}


/************************************************************************/
/* Function    :                                                        */
/* Purpose     :                                                        */
/* Input       :                                                        */
/* Outputs     :                                                        */
/************************************************************************/
