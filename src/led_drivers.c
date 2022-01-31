/*
 *  led_drivers.c
 *
 *  Created on: Jan 27, 2022
 *  Authors:  Anssi Ronkainen & Bogdan Moroz
 */
#include <zynq_registers.h>
#include <xttcps.h>

void setupRGBLed();

void setupRGBLed()
{
	// Set prescale to 0 (plus 1) and enable prescaler
    TTC0_CLK_CNTRL = (0 << XTTCPS_CLK_CNTRL_PS_VAL_SHIFT) | XTTCPS_CLK_CNTRL_PS_EN_MASK;
	//  Reset count value
    //  Disable counter
	//  Set timer to Match mode 
	//  Set the waveform polarity
    TTC0_CNT_CNTRL = XTTCPS_CNT_CNTRL_RST_MASK | XTTCPS_CNT_CNTRL_DIS_MASK | XTTCPS_CNT_CNTRL_MATCH_MASK | XTTCPS_CNT_CNTRL_POL_WAVE_MASK;
    //  Initialize match value to 0
    TTC0_MATCH_0 = 0;
    // Start timer
    TTC0_CNT_CNTRL &= ~XTTCPS_CNT_CNTRL_DIS_MASK;
}
