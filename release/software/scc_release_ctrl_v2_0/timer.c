/*
 * timer.c
 *
 *  Created on: Apr 3, 2017
 *      Author: tm
 */

#include <msp430.h>


void timer_init(void)
{

}

//------------------------------------------------------------------------------
// micro seconds delays
//
void delay_usec(int interval)
{
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
