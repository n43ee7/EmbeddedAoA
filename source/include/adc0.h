// ADC0  Library
// Jason Losh

//-----------------------------------------------------------------------------
// Hardware Target
//-----------------------------------------------------------------------------

// Target Platform: EK-TM4C123GXL
// Target uC:       TM4C123GH6PM
// System Clock:    -

// Hardware configuration:
// ADC0 SS3

//-----------------------------------------------------------------------------
// Device includes, defines, and assembler directives
//-----------------------------------------------------------------------------

#ifndef ADC0_H_
#define ADC0_H_

//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

// SS1
void initAdc0Ss1();
void setAdc0Ss1Log2AverageCount(uint8_t log2AverageCount);
void setAdc0Ss1Mux(uint8_t input);
int16_t readAdc0Ss1();

// SS2
void initAdc0Ss2();
void setAdc0Ss2Log2AverageCount(uint8_t log2AverageCount);
void setAdc0Ss2Mux(uint32_t input);
int16_t readAdc0Ss2();

// SS3
void initAdc0Ss3();
void setAdc0Ss3Log2AverageCount(uint8_t log2AverageCount);
void setAdc0Ss3Mux(uint8_t input);
int16_t readAdc0Ss3();

#endif
