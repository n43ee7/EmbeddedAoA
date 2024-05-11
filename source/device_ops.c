//-----------------------------------------------------------------------------
// Lab 5 | Device Operation Library Mega Functions SOURCE FILE
// Nabeel Nayyar
//
//-----------------------------------------------------------------------------
// Hardware Target
//-----------------------------------------------------------------------------
//
// Target Platform: EK-TM4C123GXL
// Target uC:       TM4C123GH6PM
// System Clock:    40 MHz
//
// Hardware configuration:
//  > Analog Sample Sequencer 2
//
//-----------------------------------------------------------------------------

#include "include/device_ops.h"

// Enable Input Mic Interrupts
void ADCSS0EN(void) {
    disableNvicInterrupt(INT_ADC0SS2);
    ADC0_ISC_R = ADC_ISC_IN2;
    enableNvicInterrupt(INT_ADC0SS2);
}

// HW Reboot Function
void HWReboot(void) {
    waitMicrosecond(1000);
    NVIC_APINT_R = NVIC_APINT_VECTKEY | NVIC_APINT_SYSRESETREQ;
}

// System Initialization Routines
void initHw(void) {
    // Initialize system clock to 40 MHz
    initSystemClockTo40Mhz();

    // Enable Clock gating control on peripherals
    SYSCTL_RCGCACMP_R |= SYSCTL_RCGCACMP_R0;    // Comparator
    SYSCTL_RCGCDMA_R |= SYSCTL_RCGCDMA_R0;      // DMA

    //Enable Ports
    enablePort(PORTD);
    enablePort(PORTC);

    // Set Ports as Ain on PD0, PD1, PD2 | Enable AIN Interfaces
    selectPinAnalogInput(AIN7);
    selectPinAnalogInput(AIN6);
    selectPinAnalogInput(AIN5);
    selectPinAnalogInput(AIN4);

    // Configure AIN3 as an analog input
    // select alternative functions for AN7 (PD0, PD1, PD2, PD3)
    GPIO_PORTD_AFSEL_R |= AIN7_MASK_PD0 | AIN7_MASK_PD1 | AIN7_MASK_PD2 | AIN7_MASK_PD3;
    // turn off digital operation on pin PD0, PD1, PD2, PD3
    GPIO_PORTD_DEN_R &= ~AIN7_MASK_PD0 & ~AIN7_MASK_PD1 & ~AIN7_MASK_PD2 & ~AIN7_MASK_PD3;
    // turn on analog operation on pin PD0, PD1, PD2, PD3
    GPIO_PORTD_AMSEL_R |= AIN7_MASK_PD0 | AIN7_MASK_PD1 | AIN7_MASK_PD2 | AIN7_MASK_PD3;

    // Enable ADC on Sequence Sampler 2
    initAdc0Ss2();

    // NOTE: TALL MIC is on AIN5 PD2
    // Set AINx for Sequence Sampler 2 Hardware Sampling for AINx MUXx
    setAdc0Ss2Mux(7);        // AIN7 - PD0
    setAdc0Ss2Mux(6);        // AIN6 - PD1
    setAdc0Ss2Mux(5);        // AIN5 - PD2 (TALL MIC)
    setAdc0Ss2Mux(4);        // AIN4 - PD3

    // Enable ADCSS2 Interrupt
    enableNvicInterrupt(INT_ADC0SS2);

    // Enable UART0 on Device with BUAD Rate
    initUart0();
    setUart0BaudRate(BUAD_RATE, FCYC);

    // Enable Comparator Co-ve
    initCOMPARATOR();

    // Enable DMA system
    initDMA();
}

//  Initialize the analog comparator0 (CO-ve) for internal voltage reference for the C0+
void initCOMPARATOR(void) {
    // Analog Comparator configuration
    GPIO_PORTC_AFSEL_R  &= ~CON_MASK;      // Alternative functions for AINx (PC7)
    GPIO_PORTC_DEN_R    &= ~CON_MASK;      // OFF digital operation on pin PC7 [DEN = 0] PIN ANALOG CONTROL
    GPIO_PORTC_AMSEL_R  |= CON_MASK;       // ON analog operation on pin PC7 [analog mode]

    // Positive C0- /or/ positive
    COMP_ACREFCTL_R |= COMP_ACREFCTL_EN;   // Enable analog comparator [Oxf @ virefmin = 2]
    COMP_ACREFCTL_R |= COMP_ACREFCTL_RNG;
    COMP_ACREFCTL_R |= 0x5;                // Comp @ Value [2.078]

    // Meta-stability Delay
    _delay_cycles(10);

    COMP_ACCTL0_R |= COMP_ACRIS_R;
    COMP_ACCTL0_R |= COMP_ACCTL0_ASRCP_REF;
    COMP_ACCTL0_R |= COMP_ACCTL0_ISEN_RISE;

    // Analog comparator interrupt enable
    COMP_ACINTEN_R = COMP_ACINTEN_IN0;
    NVIC_EN0_R = 1 << (INT_COMP0-16);
}

// Function to Configure DMA channel 8 for ping-pong receive (Ch14 ADC SS0 Enc 0 [B]
void initDMA(void) {
    // Set channel 8 to High priority
    UDMA_PRIOSET_R |= (1 << 8);
    // Select primary channel control structure for channel 8
    UDMA_ALTCLR_R |= (1 << 8);
    // Allow the uDMA controller to respond to single and burst requests
    UDMA_USEBURSTCLR_R |= (1 << 8);
    // Allow the uDMA controller to recognize requests for channel 8
    UDMA_REQMASKCLR_R |= (1 << 8);

}
