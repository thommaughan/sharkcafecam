/*
 * adc.c
 *
 *  Created on: Apr 19, 2017
 *      Author: tm
 */
#include <msp430.h>
#include <stdint.h>
#include "adc.h"



uint16_t adcValue = 0;
uint16_t adcRdy = 0;

extern  void gpio_lo_rel(int state);
extern void gpio_hi_rel_lo_cont(int state);

void adc_init(void)
{
    // configure the ADC inputs
    //P1.3 (VBAT), P1.4(VSOL), P1.5(VCONT)


    ADC10CTL1 = INCH_3 + ADC10DIV_3 ;         // Channel 3, ADC10CLK/3
    // 1010 Temperature sensor
    // SHS0

    //ADC10CTL0 = SREF_0 + ADC10SHT_3 + ADC10ON + ADC10IE;  // Vcc & Vss as reference, Sample and hold for 64 Clock cycles, ADC on, ADC interrupt enable
    ADC10CTL0 = SREF_0 + ADC10SHT_1 + ADC10ON + ADC10IE;  // Vcc & Vss as reference, Sample and hold for 8 Clock cycles, ADC on, ADC interrupt enable
    // ADC10SR


    ADC10AE0 |= BIT3 | BIT4 | BIT5;           // ADC input enable P1.3

}

/**
* Reads ADC 'chan' once using AVCC as the reference.
**/
void adc_read(unsigned int chan)
{
    ADC10CTL0 &= ~ENC;              // Disable ADC
    ADC10CTL0 = ADC10SHT_3 + ADC10ON + ADC10IE; // 16 clock ticks, ADC On, enable ADC interrupt
    ADC10CTL1 = ADC10SSEL_3 + chan;             // Set 'chan', SMCLK
    ADC10CTL0 |= ENC + ADC10SC;                 // Enable and start conversion
}


/**
* Reads ADC 'chan' once using an internal reference, 'ref' determines if the
*   2.5V or 1.5V reference is used.
**/
void adc_read_intref(unsigned int chan, unsigned int ref)
{
    ADC10CTL0 &= ~ENC;                          // Disable ADC
    ADC10CTL0 = SREF_1 + ADC10SHT_3 + REFON + ADC10ON + ref + ADC10IE;  // Use reference,
                                                    //   16 clock ticks, internal reference on
                                                    //   ADC On, enable ADC interrupt, Internal  = 'ref'
    ADC10CTL1 = ADC10SSEL_3 + chan;                 // Set 'chan', SMCLK
    __delay_cycles (128);                           // Delay to allow Ref to settle
    ADC10CTL0 |= ENC + ADC10SC;                     // Enable and start conversion
}


void adc_read_temperature(void)
{
    adc_read_intref(INCH_10, 0);
}

void adc_read_vcc(void)
{
    //adc_read_intref(INCH_11, 0);
    adc_read(INCH_11);
}

void adc_read_battery(void)
{
    //adc_read(INCH_3);               // INCH_3 is P1.3 VBATMON
    gpio_lo_rel(1);
    __delay_cycles(10000);
    adc_read(INCH_3);               // INCH_3 is P1.3 VBATMON
    //adc_read_intref(INCH_3, 0);               // INCH_3 is P1.3 VBATMON

}

void adc_read_solar(void)
{
    gpio_lo_rel(1);
    __delay_cycles(10000);
    adc_read(INCH_4);               // INCH_3 is P1.4 VSOLMON
}

void adc_read_cont(void)
{
    gpio_hi_rel_lo_cont(1);
    __delay_cycles(10000);
    adc_read(INCH_5);               // INCH_3 is P1.5 VCONTMON
}


/**
* ADC interrupt routine. Pulls CPU out of sleep mode for the main loop.
**/
#pragma vector=ADC10_VECTOR
__interrupt void ADC10_ISR (void)
{
    adcValue = ADC10MEM;            // Saves measured value.
    adcRdy = 1;             // Sets flag for main loop.
    __bic_SR_register_on_exit(CPUOFF);  // Enable CPU so the main while loop continues
}

