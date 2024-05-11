// ADC0 Library
// Jason Losh

//-----------------------------------------------------------------------------
// Hardware Target
//-----------------------------------------------------------------------------

// Target Platform: EK-TM4C123GXL
// Target uC:       TM4C123GH6PM
// System Clock:    -

// Hardware configuration:
// ADC0 SS2

//-----------------------------------------------------------------------------
// Device includes, defines, and assembler directives
//-----------------------------------------------------------------------------

#include <stdint.h>
#include <stdbool.h>
#include "include/tm4c123gh6pm.h"
#include "include/adc0.h"

#define ADC_CTL_DITHER          0x00000040

// Subroutines

// Initialize Hardware
void initAdc0Ss2() {
    // Enable clocks
    SYSCTL_RCGCADC_R |= SYSCTL_RCGCADC_R0;
    _delay_cycles(16);

    // Configure ADC
    ADC0_ACTSS_R &= ~ADC_ACTSS_ASEN2;                // disable sample sequencer 2 (SS2) for programming
    ADC0_CC_R = ADC_CC_CS_SYSPLL;                    // select PLL as the time base (not needed, since default value)
    ADC0_PC_R = ADC_PC_SR_1M;                        // select 1Msps rate
    ADC0_EMUX_R = ADC_EMUX_EM2_ALWAYS;            // select SS2 bit in ADCPSSI as trigger
    ADC0_SSCTL2_R = ADC_SSCTL2_END2 | ADC_SSCTL2_IE2;                 // mark Third sample as the end
    ADC0_IM_R = ADC_IM_MASK2;                           // enable interrupt after third sample on ss2
    ADC0_ACTSS_R |= ADC_ACTSS_ASEN2;                 // enable SS2 for operation
}

// Set SS3 input sample average count
void setAdc0Ss2Log2AverageCount(uint8_t log2AverageCount) {
    ADC0_ACTSS_R &= ~ADC_ACTSS_ASEN2;                // disable sample sequencer 2 (SS2) for programming
    ADC0_SAC_R = log2AverageCount;                   // sample HW averaging
    if (log2AverageCount == 0)
        ADC0_CTL_R &= ~ADC_CTL_DITHER;               // turn-off dithering if no averaging
    else
        ADC0_CTL_R |= ADC_CTL_DITHER;                // turn-on dithering if averaging
    ADC0_ACTSS_R |= ADC_ACTSS_ASEN2;                 // enable SS2 for operation
}

// Set SS3 analog input
void setAdc0Ss2Mux(uint32_t input) {
    ADC0_ACTSS_R &= ~ADC_ACTSS_ASEN2;                // disable sample sequencer 3 (SS2) for programming
    ADC0_SSMUX2_R = input;                           // Set analog input for single sample
    ADC0_ACTSS_R |= ADC_ACTSS_ASEN2;                 // enable SS2 for operation
}

// Request and read one sample from SS3 -- Need to make array?
int16_t readAdc0Ss2() {
    ADC0_PSSI_R |= ADC_PSSI_SS3;                     // set start bit
    return ADC0_SSFIFO2_R;                           // get single result from the FIFO
}
