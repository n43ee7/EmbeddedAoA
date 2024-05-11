//-----------------------------------------------------------------------------
// Hardware Target
//-----------------------------------------------------------------------------

// Target Platform: EK-TM4C123GXL with LCD/Temperature Sensor
// Target uC:       TM4C123GH6PM
// System Clock:    40 MHz
// Stack:           4096 bytes (needed for snprintf)

// Hardware configuration:
// LM60 Temperature Sensor:
//   AIN3/PE0 is driven by the sensor
//   (V = 424mV + 6.25mV / degC with +/-2degC uncalibrated error)
// UART Interface:
//   U0TX (PA1) and U0RX (PA0) are connected to the 2nd controller
//   The USB on the 2nd controller enumerates to an ICDI interface and a virtual COM port
//   Configured to 115,200 baud, 8N1

//-----------------------------------------------------------------------------
// Device includes, defines, and assembler directives
//-----------------------------------------------------------------------------

#include <stdio.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include "include/tm4c123gh6pm.h"
#include "include/clock.h"
#include "include/wait.h"
#include "include/uart0.h"
#include "include/adc0.h"
#include "include/gpio.h"
#include "include/nvic.h"
#include "include/uart_handler.h"


// Port D Operators and masks
#define AIN7_MASK_PD0   0
#define AIN7_MASK_PD1   1
#define AIN7_MASK_PD2   2
#define AIN7_MASK_PD3   3
#define AIN7            PORTD, 0     // Analog Interface 1 (PD0)
#define AIN6            PORTD, 1     // Analog Interface 2 (PD1)
#define AIN5            PORTD, 2     // Analog Interface 3 (PD2)
#define AIN4            PORTD, 3     // Analog Interface 4 (PD3)
#define AIN7_MUX        0            // Analog Interface 1 SS2 MUX
#define AIN6_MUX        1            // Analog Interface 2 SS2 MUX
#define AIN5_MUX        2            // Analog Interface 3 SS2 MUX
#define AIN4_MUX        4            // Analog Interface 4 SS2 MUX


// Global variables
// ISR Flow Control variables
bool AngleOn    = false;
bool tdOn       = false;
bool failOn     = false;

// ISR Flow Update variables
uint32_t fin2 = 0;
uint32_t fin3 = 0;
uint32_t TimeConstant = 50;
uint32_t TDoA = 0;
uint32_t AoA = 0;
uint32_t backoff = 90; //Analog Value Backoff DEF 30 / STABLE @ 100
uint32_t holdoff = 1000000; // Holdoff in Microseconds DEF 1000000
uint32_t HBPM = 0;

// Local Routine declarations
void initHw();
void MicISR();
void MicON();

// ISR Function Load values
uint16_t raw1 = 0, raw2 = 0, raw3 = 0, raw4 = 0;

int main(void) {
    USER_DATA data;
    initHw();
    initUart0();

    MicON();
    setUart0BaudRate(115200, 40e6);

    char str1[80] = {};
    while(1) {
        // UART0 Handler
        getsUart0(&data);
        parseFields(&data);
        bool valid = false;

        // Reset Command
        if(isCommand(&data, "reset", 0)) {
            putsUart0("[!] Reset Activated\n");
            waitMicrosecond(1000);
            NVIC_APINT_R = NVIC_APINT_VECTKEY | NVIC_APINT_SYSRESETREQ;
            valid = true;
        }

        // Average Command
        if(isCommand(&data, "average", 0)) {
            snprintf(str1, sizeof(str1), "Analog Average Mic 1 = %d\nAnalog Average 2 = %d\nAnalog Average 3 = %d\n", HBPM, fin2, fin3);
            putsUart0(str1);
            snprintf(str1, sizeof(str1), "DAC Average Mic 1 = %.2f\nDAC Average 2 = %.2f\nDAC Average 3 = %.2f\n", HBPM*0.0008, fin2*0.0008, fin3*0.0008);
            putsUart0(str1);
            snprintf(str1, sizeof(str1), "SPL Average Mic 1 = %.2f\nSPL Average 2 = %.2f\nSPL Average 3 = %.2f\n", 20*log((HBPM*0.0008)/0.00631)+10, 20*log((fin2*0.0008)/0.00631)+10, 20*log((fin3*0.0008)/0.00631)+10);
            putsUart0(str1);
            valid = true;
        }

        // time constant command in microseconds
        if(isCommand(&data, "tc", 0)) {
            disableNvicInterrupt(INT_ADC0SS2);
            char * str = getFieldString(&data, 1);
            if(atoi(&data.buffer[data.fieldPosition[1]])>0) {
                uint32_t NumGiven = atoi(&data.buffer[data.fieldPosition[1]]);
                snprintf(str1, sizeof(str1), " TC Set to %lu \n", NumGiven);
                putsUart0(str1);
                int Divs = NumGiven % 3;
                TimeConstant = (NumGiven - Divs)/3;
                valid = true;
            }
            else {
                snprintf(str1, sizeof(str1), " TC is %d \n", TimeConstant*3);
                putsUart0(str1);
                valid = true;
            }
            enableNvicInterrupt(INT_ADC0SS2);
        }

        // time constant command in microseconds
        if(isCommand(&data, "backoff", 0)) {
            disableNvicInterrupt(INT_ADC0SS2);
            char *str = getFieldString(&data, 1);
            if(atoi(&data.buffer[data.fieldPosition[1]])>0) {
                backoff = atoi(&data.buffer[data.fieldPosition[1]]);
                snprintf(str1, sizeof(str1), "[>] Backoff Set to %lu \n", backoff);
                putsUart0(str1);
                valid = true;
            } else {
                snprintf(str1, sizeof(str1), "[>] Backoff is %d \n", backoff);
                putsUart0(str1);
                valid = true;
            }
            enableNvicInterrupt(INT_ADC0SS2);
        }

        // time constant command in microseconds
        if(isCommand(&data, "holdoff", 0)) {
            disableNvicInterrupt(INT_ADC0SS2);
            char *str = getFieldString(&data, 1);
            if(atoi(&data.buffer[data.fieldPosition[1]])>0) {
                holdoff = atoi(&data.buffer[data.fieldPosition[1]]);
                snprintf(str1, sizeof(str1), " Holdoff Set to %lu microseconds \n", holdoff);
                putsUart0(str1);
                valid = true;
            } else {
                snprintf(str1, sizeof(str1), " Holdoff is %d microseconds\n", holdoff);
                putsUart0(str1);
                valid = true;
            }
            enableNvicInterrupt(INT_ADC0SS2);
        }

        // Angle of Arrival ISR command
        if(isCommand(&data, "aoa", 0)) {
            char * str = getFieldString(&data, 1);
            if(!comp(&data.buffer[data.fieldPosition[1]], "always")) {
                putsUart0("AOA DATA ON\n");
                AngleOn = true;
                valid = true;
            } if (!comp(&data.buffer[data.fieldPosition[1]], "stop")) {
                putsUart0("AOA DAA OFF\n");
                AngleOn = false;
                valid = true;
            }
        }

        // T-DoA ISR Command
        if(isCommand(&data, "tdoa", 0)) {
            char * str = getFieldString(&data, 1);
            if(!comp(&data.buffer[data.fieldPosition[1]], "ON")) {
                putsUart0("TDOA DATA ON\n");
                tdOn = true;
                valid = true;
            } if(!comp(&data.buffer[data.fieldPosition[1]], "OFF")) {
                putsUart0("TDOA DATA OFF\n");
                tdOn = false;
                valid = true;
            }
        }

        // Fail Command
        if(isCommand(&data, "fail", 0)) {
            char * str = getFieldString(&data, 1);
            if(!comp(&data.buffer[data.fieldPosition[1]], "ON")) {
                putsUart0("FAIL DATA ON\n\n");
                failOn = true;
                valid = true;
            } if(!comp(&data.buffer[data.fieldPosition[1]], "OFF")) {
                putsUart0("FAIL DATA OFF\n\n");
                failOn = false;
                valid = true;
            }
        }

        // Raw DAC Values command
        if (isCommand(&data, "raw", 0)) {
            snprintf(str1, sizeof(str1), "[!] RAW \n Mic 1 = %d\n Mic 2 = %d\n Mic 3 = %d\n Mic 4 = %d\n", raw1, raw2, raw3, raw4);
            putsUart0(str1);
            valid = true;
        }

        // Try-Catch for invalid command entry
        if(!valid) {
            putsUart0("Invalid command\n");
        }

    }
}

// ---------------------- Subroutines ----------------------

void MicON() {
    disableNvicInterrupt(INT_ADC0SS2);
    ADC0_ISC_R = ADC_ISC_IN2;
    enableNvicInterrupt(INT_ADC0SS2);
}

void initHw() {
    // Initialize system clock to 40 MHz
    initSystemClockTo40Mhz();

    //Enable Ports
    enablePort(PORTD);

    //Set Ports as Ain on PD0, PD1, PD2
    // Enable AIN Interfaces for Analog Input PD0-PD2
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

    // Init ADC SS2
    initAdc0Ss2();
    // NOTE: TALL MIC is on AIN5 PD2
    // Set AINx for Sequence Sampler 2 Hardware Sampling for AINx MUXx
    setAdc0Ss2Mux(7);        // AIN7 - PD0
    setAdc0Ss2Mux(6);        // AIN6 - PD1
    setAdc0Ss2Mux(5);        // AIN5 - PD2 (TALL MIC)
    setAdc0Ss2Mux(4);        // AIN4 - PD3

    enableNvicInterrupt(INT_ADC0SS2);
}

void MicISR() {
            char str[80];
            uint16_t x[300], x2[300], x3[300]; // check
            static uint32_t checker = 0;
            float firTemp;
            static uint8_t counter = 0;
            int i = 0;
            static uint16_t index = 0, index2 = 0, index3 = 0;
            static uint16_t sum = 0, sum2 = 0, sum3 = 0; // total fits in 16b since 12b adc output x 16 samples
            static bool EventON = false;
            static int superCount = 0;

            raw1 = readAdc0Ss2();
            raw2 = readAdc0Ss2();
            raw3 = readAdc0Ss2();
            raw4 = readAdc0Ss2();

            //HOLDOFF "FUNCTION"
            if(EventON) {
                superCount++;
                checker = 1;
                if(superCount == (holdoff/100)) {
                    EventON = false;
                    superCount = 0;
                    checker = 0;
                }
            }

            //SLIDING AVERAGE FILTER
            //raw1
            index = (index + 1) % TimeConstant;
            x[index] = raw1;
            sum = 0;
            for (i = 0; i < TimeConstant; i++) {
                sum += x[i];
            }
            firTemp = (sum/TimeConstant);
            HBPM = firTemp;

            //raw2
            index2 = (index2 + 1) % TimeConstant;
            x2[index2] = raw2;
            sum2 = 0;
            for (i = 0; i < TimeConstant; i++) {
                sum2 += x2[i];
            }
            int div2 = (sum2/TimeConstant);
            fin2 = div2;

            //raw3
            index3 = (index3 + 1) % TimeConstant;
            x3[index3] = raw3;
            sum3 = 0;
            for (i = 0; i < TimeConstant; i++) {
                sum3 += x3[i];
            }
            int div3 = (sum3/TimeConstant);
            fin3 = div3;

            static int SavedRaw1 = 0, SavedRaw2 = 0, SavedRaw3 = 0, R2=0, R3=0;
            static int count=0;
            static bool raw1Flag = false;
            if(raw1 > HBPM+2 && checker == 0) {
                raw1Flag = true;
                SavedRaw1 = raw1;
            }
            if(raw1Flag == true) {
                count++;
                checker = 1;
                static int SavedCount2 = 0, SavedCount3 = 0;
                if(raw2 > (fin2 - backoff) && R2 == 0) {
                    SavedRaw2 = raw2;
                    SavedCount2 = count; // Num of ISR Cycles to get raw2
                    R2 = 1;
                }
                if(raw3 > (fin3 +50 - backoff) && R3 == 0) {
                    SavedRaw3 = raw3;
                    SavedCount3 = count; // Num of ISR Cycles to get raw3
                    R3 = 1;
                }

                if(SavedRaw2 > 0 && SavedRaw3 > 0) {
                    if(tdOn || AngleOn) {
                        snprintf(str, sizeof(str), "EVENT FOUND: \nRAW 1: %d\nRAW 2: %d\nRAW 3: %d\n", SavedRaw1, SavedRaw2, SavedRaw3);
                        putsUart0(str);

                    }

                    EventON = true;

                    //TDoA Calculation
                    TDoA = (SavedCount2 * 3) - (SavedCount3 * 3);
                    if(tdOn) {
                        snprintf(str, sizeof(str), "TDoA: %d \n", TDoA);
                        putsUart0(str);
                    }

                    if(AngleOn) {
                        int AoA = 0 -(SavedCount2 * 3) / TDoA;
                        snprintf(str, sizeof(str), "Angle Of Arrival: %d \n", AoA);
                        putsUart0(str);
                    }

                    checker = 0;
                    count = 0;
                    raw1Flag = false;
                    R2 = 0;
                    R3 = 0;
                    SavedRaw1 = 0;
                    SavedRaw2 = 0;
                    SavedRaw3 = 0;
                }
                if(count == 77) {
                    if(SavedRaw2 == 0) {
                        if(failOn) {
                            snprintf(str, sizeof(str), "FAILED: \nRAW 1: %d\nRAW 2: %d\nRAW 3: %d", SavedRaw1, SavedRaw2, SavedRaw3);
                            putsUart0(str);
                        }
                    }

                    if(SavedRaw3 == 0) {
                        if(failOn) {
                            snprintf(str, sizeof(str), "FAILED: \nRAW 1: %d\nRAW 2: %d\nRAW 3: %d", SavedRaw1, SavedRaw2, SavedRaw3);
                            putsUart0(str);
                        }
                    }

                    checker = 0;
                    count = 0;
                    raw1Flag = false;
                    R2 = 0;
                    R3 = 0;
                    SavedRaw1 = 0;
                    SavedRaw2 = 0;
                    SavedRaw3 = 0;
                }
            }

            //ADC0_ISC_R &= ~ADC_ISC_IN2;
            counter++;
            ADC0_ISC_R = ADC_ISC_IN2;
}
