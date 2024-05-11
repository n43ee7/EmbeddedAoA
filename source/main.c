// CSE 4342 | Embedded Systems 2 | Project
//
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
//  Analog-Digital Interfaces @ SS1
//      AIN1:   PD0 (Short Microphone)
//      AIN2:   PD1 (Short Microphone)
//      AIN3:   PD2 (Tall Microphone)
//      AIN4:   PD3 (Short Microphone)
//  UART Interface:
//      U0TX (PA1) and U0RX (PA0) are connected to the 2nd controller
//      The USB on the 2nd controller enumerates to an ICDI interface and a virtual COM port
//      Configured to 115,200 baud, 8N1
//

// Device includes, defines, and assembler directives
#include "include/device_ops.h"

// Global variables
// ISR Flow Control variables
bool AOAC_MODE  = false;
bool TDOA_MODE  = false;
bool FAIL_MODE  = false;
bool COMP_TRIG  = false;

// ISR Flow Update variables
uint32_t fin1 = 0;          // Final Sliding Average of Microphone 1
uint32_t fin2 = 0;          // Final Sliding Average of Microphone 2
uint32_t fin3 = 0;          // Final Sliding Average of Microphone 3
uint32_t TDoA = 0;          // Time Difference of Arrival value
int      AoA  = 0;          // Angle of Arrival value
uint32_t TIME_CONST = 50;   // Signal Time arrival Constant value
uint32_t BCKF_CONST = 90;      // Analog Value BCKF_CONST DEF 30 / STABLE @ 100
uint32_t HLDO_CONST = 1000000; // Holdoff in Microseconds DEF 1000000

// Interrupt Service Routine declarations
void ADCSamplingISR (void);     // ADC SS2 Sampling ISR
void ADCOMPResponseISR (void);  // Comparator Trigger ISR

// ISR Functions Load values
uint16_t raw1   = 0, raw2   = 0, raw3   = 0, raw4   = 0;    // ADCSS2
uint16_t C_raw1 = 0, C_raw2 = 0, C_raw3 = 0, C_raw4 = 0;    // COMPISR

int main(void) {
    USER_DATA data;
    initHw();       // Enable System Hardware
    ADCSS0EN();     // Enable ADC Sequence Sampler Level 0 Interrupt

    char str1[MAX_CHARS] = {};
    while(true) {
        // UART0 Handler
        putsUart0(">");
        getsUart0(&data);
        parseFields(&data);
        bool valid = false;

        // Reset Command
        if(isCommand(&data, "reset", 0)) {
            putsUart0("[!] Reset Activated\n");
            HWReboot();
            valid = true;
        }

        // Average Command
        if(isCommand(&data, "average", 0)) {
            snprintf(str1, sizeof(str1), "Analog Average Mic 1: %d\nAnalog Average Mic 2: %d\nAnalog Average Mic 3: %d\n", fin1, fin2, fin3);
            putsUart0(str1);
            snprintf(str1, sizeof(str1), "DAC Average Mic 1: %.2f\nDAC Average Mic 2: %.2f\nDAC Average Mic 3: %.2f\n", fin1*0.0008, fin2*0.0008, fin3*0.0008);
            putsUart0(str1);
            snprintf(str1, sizeof(str1), "SPL Average Mic 1: %.2f\nSPL Average Mic 2: %.2f\nSPL Average Mic 3: %.2f\n", 20*log((fin1*0.0008)/0.00631)+10, 20*log((fin2*0.0008)/0.00631)+10, 20*log((fin3*0.0008)/0.00631)+10);
            putsUart0(str1);
            valid = true;
        }

        // time constant command in microseconds
        if(isCommand(&data, "tc", 0)) {
            disableNvicInterrupt(INT_ADC0SS2);
            char *str = getFieldString(&data, 1);
            if(atoi(&data.buffer[data.fieldPosition[1]])>0) {
                uint32_t NumGiven = atoi(&data.buffer[data.fieldPosition[1]]);
                snprintf(str1, sizeof(str1), " TC Set to %lu \n", NumGiven);
                putsUart0(str1);
                int Divs = NumGiven % 3;
                TIME_CONST = (NumGiven - Divs)/3;
                valid = true;
            }
            else {
                snprintf(str1, sizeof(str1), " TC is %d \n", TIME_CONST*3);
                putsUart0(str1);
                valid = true;
            }
            enableNvicInterrupt(INT_ADC0SS2);
        }

        // backoff command in microseconds
        if(isCommand(&data, "backoff", 0)) {
            disableNvicInterrupt(INT_ADC0SS2);
            char *str = getFieldString(&data, 1);
            if(atoi(&data.buffer[data.fieldPosition[1]])>0) {
                BCKF_CONST = atoi(&data.buffer[data.fieldPosition[1]]);
                snprintf(str1, sizeof(str1), "[>] Backoff Set to %lu \n", BCKF_CONST);
                putsUart0(str1);
                valid = true;
            } else {
                snprintf(str1, sizeof(str1), "[>] Backoff is %d \n", BCKF_CONST);
                putsUart0(str1);
                valid = true;
            }
            enableNvicInterrupt(INT_ADC0SS2);
        }

        // holdoff command in microseconds
        if(isCommand(&data, "holdoff", 0)) {
            disableNvicInterrupt(INT_ADC0SS2);
            char *str = getFieldString(&data, 1);
            if(atoi(&data.buffer[data.fieldPosition[1]])>0) {
                HLDO_CONST = atoi(&data.buffer[data.fieldPosition[1]]);
                snprintf(str1, sizeof(str1), "[>] Holdoff Set to %lu microseconds \n", HLDO_CONST);
                putsUart0(str1);
                valid = true;
            } else {
                snprintf(str1, sizeof(str1), "[>] Holdoff is %d microseconds\n", HLDO_CONST);
                putsUart0(str1);
                valid = true;
            }
            enableNvicInterrupt(INT_ADC0SS2);
        }

        // Angle of Arrival ISR command
        if(isCommand(&data, "aoa", 0)) {
            char *str = getFieldString(&data, 1);
            if(!comp(&data.buffer[data.fieldPosition[1]], "always")) {
                putsUart0("[+] AOA DATA ON\n");
                AOAC_MODE = true;
            } else if (!comp(&data.buffer[data.fieldPosition[1]], "stop")) {
                putsUart0("[>] AOA DATA OFF\n");
                AOAC_MODE = false;
            } else {
                snprintf(str1, sizeof(str1), "[+] Last AoA: %d deg\n", AoA);
                putsUart0(str1);
            }
            valid = true;
        }

        // T-DoA ISR Command
        if(isCommand(&data, "tdoa", 0)) {
            char *str = getFieldString(&data, 1);
            if(!comp(&data.buffer[data.fieldPosition[1]], "ON")) {
                putsUart0("TDOA DATA ON\n");
                TDOA_MODE = true;
                valid = true;
            } if(!comp(&data.buffer[data.fieldPosition[1]], "OFF")) {
                putsUart0("TDOA DATA OFF\n");
                TDOA_MODE = false;
                valid = true;
            }
        }

        // Fail Command
        if(isCommand(&data, "fail", 0)) {
            char *str = getFieldString(&data, 1);
            if(!comp(&data.buffer[data.fieldPosition[1]], "ON")) {
                putsUart0("FAIL DATA ON\n\n");
                FAIL_MODE = true;
                valid = true;
            } if(!comp(&data.buffer[data.fieldPosition[1]], "OFF")) {
                putsUart0("FAIL DATA OFF\n\n");
                FAIL_MODE = false;
                valid = true;
            }
        }

        // Raw DAC Values command
        if (isCommand(&data, "raw", 0)) {
            snprintf(str1, sizeof(str1), "[>] Immediate RAW Values: \n Mic 1 = %d\n Mic 2 = %d\n Mic 3 = %d\n Mic 4 = %d\n", raw1, raw2, raw3, raw4);
            putsUart0(str1);
            valid = true;
        }

        // Comparator Level Command
        if (isCommand(&data, "level", 0)) {
            uint8_t COMP_LVL;
            COMP_LVL = atoi(&data.buffer[data.fieldPosition[1]]);
            if (COMP_LVL <= 0xF) {
                snprintf(str1, sizeof(str1), "[>] Comparator Level now set to: %02X \n", COMP_LVL);
                COMP_ACREFCTL_R |= COMP_LVL;
                putsUart0(str1);
            } else {
                snprintf(str1, sizeof(str1), "[!] Comparator Level not in range. [0 - 15] [Default: 5]\n");
                putsUart0(str1);
            }
            valid = true;
        }

        // Help Command
        if (isCommand(&data, "help", 0)) {
            snprintf(str1, sizeof(str1), "------------------------------------- [>] Help menu -------------------------------------\n");
            putsUart0(str1);
            snprintf(str1, sizeof(str1), "$ help         | Shows this menu again.\n");
            putsUart0(str1);
            snprintf(str1, sizeof(str1), "$ reset        | Resets the controller.\n");
            putsUart0(str1);
            snprintf(str1, sizeof(str1), "$ raw          | Outputs all microphones' active raw DAC values.\n");
            putsUart0(str1);
            snprintf(str1, sizeof(str1), "$ level <0-15> | Sets Comparator trigger level rage [0x0 - 0xF]\n");
            putsUart0(str1);
            snprintf(str1, sizeof(str1), "$ fail <1>     | Shows failed events in TDoA calculation. <1> ON/OFF.\n");
            putsUart0(str1);
            snprintf(str1, sizeof(str1), "$ average      | Shows DAC units and SPL Average for all Microphones.\n");
            putsUart0(str1);
            snprintf(str1, sizeof(str1), "$ backoff <1>  | <1> Sets backoff threshold for microphone events\n");
            putsUart0(str1);
            snprintf(str1, sizeof(str1), "$ holdoff <1>  | <1> Sets minimum trigger time interval for subsequent events\n");
            putsUart0(str1);
            snprintf(str1, sizeof(str1), "$ aoa <1>      | Starts calculating Angle of Arrival of events <1> 'always' or 'stop' \n");
            putsUart0(str1);
            snprintf(str1, sizeof(str1), "$ tdoa <1>     | Starts calculating time of arrival of events <1> 'ON' or 'OFF'\n");
            putsUart0(str1);
            snprintf(str1, sizeof(str1), "-----------------------------------------------------------------------------------------\n");
            putsUart0(str1);
            valid = true;
        }

        // Try-Catch for invalid command entry
        if(!valid) {
            putsUart0("[!] Invalid command. Enter 'help' for commands supported\n");
        }
    }
}

// ------------------------------------------------------ ISRs ------------------------------------------------------
void ADCSamplingISR (void) {
    char str[MAX_CHARS];
    uint16_t x1[SAMPLE_LENGTH], x2[SAMPLE_LENGTH], x3[SAMPLE_LENGTH];
    static uint32_t ISR_STATE_CHK = 0;
    int i = 0;
    static uint16_t index1 = 0, index2 = 0, index3 = 0;
    static uint16_t FIR_SUM1 = 0, FIR_SUM2 = 0, FIR_SUM3 = 0; // total fits in 16b since 12b adc output x 16 samples
    static bool EVENT_RESPONSE = false;
    static int superCount = 0;
    static int SavedRaw1 = 0, SavedRaw2 = 0, SavedRaw3 = 0, R2 = 0, R3 = 0;
    static int count = 0;
    static bool raw1Flag = false;

    // Collect Analog values from the DAC SS2
    raw1 = readAdc0Ss2();
    raw2 = readAdc0Ss2();
    raw4 = readAdc0Ss2();
    raw3 = readAdc0Ss2();

    //HOLDOFF "FUNCTION"
    if(EVENT_RESPONSE) {
        superCount++;
        ISR_STATE_CHK = 1;
        if(superCount == (HLDO_CONST / SAMPLE_DIV)) {
            EVENT_RESPONSE = false;
            superCount = 0;
            ISR_STATE_CHK = 0;
        }
    }

    // Implementing a sliding average filter (RAW 1-3)
    index1 = (index1 + 1) % TIME_CONST;
    x1[index1] = raw1;
    FIR_SUM1 = 0;

    index2 = (index2 + 1) % TIME_CONST;
    x2[index2] = raw2;
    FIR_SUM2 = 0;

    index3 = (index3 + 1) % TIME_CONST;
    x3[index3] = raw3;
    FIR_SUM3 = 0;

    for (i = 0; i < TIME_CONST; i++) {
        FIR_SUM1 += x1[i];
        FIR_SUM2 += x2[i];
        FIR_SUM3 += x3[i];
    }

    fin1 = (FIR_SUM1/TIME_CONST);
    fin2 = (FIR_SUM2/TIME_CONST);
    fin3 = (FIR_SUM3/TIME_CONST);

    // Computing Cycles taking to trigger response
    if(raw1 > fin1+2 && ISR_STATE_CHK == 0) {
        raw1Flag = true;
        SavedRaw1 = raw1;
    }
    if(raw1Flag == true) {
        count++;
        ISR_STATE_CHK = 1;
        static int SavedCount2 = 0, SavedCount3 = 0;
        if(raw2 > (fin2 - BCKF_CONST) && R2 == 0) {
            SavedRaw2 = raw2;
            SavedCount2 = count; // Num of ISR Cycles to get raw2
            R2 = 1;
        }
        if(raw3 > (fin3 +50 - BCKF_CONST) && R3 == 0) {
            SavedRaw3 = raw3;
            SavedCount3 = count; // Num of ISR Cycles to get raw3
            R3 = 1;
        }

        if(SavedRaw2 > 0 && SavedRaw3 > 0) {
            if(TDOA_MODE || AOAC_MODE) {
                snprintf(str, sizeof(str), "[+] TRIGGER EVENT: \nRAW 1: %d\nRAW 2: %d\nRAW 3: %d\n", SavedRaw1, SavedRaw2, SavedRaw3);
                putsUart0(str);
            }

            EVENT_RESPONSE = true;

            // TDoA Calculation
            TDoA = (SavedCount2 * 3) - (SavedCount3 * 3);
            if(TDOA_MODE) {
                snprintf(str, sizeof(str), "TDoA: %d \n", TDoA);
                putsUart0(str);
            }

            if(AOAC_MODE) {
                AoA = 0 - (SavedCount2 * 3) / TDoA;
                snprintf(str, sizeof(str), "Angle Of Arrival: %d \n", AoA);
                putsUart0(str);
            }

            ISR_STATE_CHK = 0;
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
                if(FAIL_MODE) {
                    snprintf(str, sizeof(str), "[>] FAILED: \nRAW 1: %d\nRAW 2: %d\nRAW 3: %d", SavedRaw1, SavedRaw2, SavedRaw3);
                    putsUart0(str);
                }
            }

            if(SavedRaw3 == 0) {
                if(FAIL_MODE) {
                    snprintf(str, sizeof(str), "[>] FAILED: \nRAW 1: %d\nRAW 2: %d\nRAW 3: %d", SavedRaw1, SavedRaw2, SavedRaw3);
                    putsUart0(str);
                }
            }
            ISR_STATE_CHK = 0;
            count = 0;
            raw1Flag = false;
            R2 = 0;
            R3 = 0;
            SavedRaw1 = 0;
            SavedRaw2 = 0;
            SavedRaw3 = 0;
        }
    }
    // Release the Interrupt status
    ADC0_ISC_R = ADC_ISC_IN2;
}

// Comparator Response Analog Fetch ISR
void ADCOMPResponseISR (void) {

    // Read Analog Samples on Trigger event
    C_raw1 = readAdc0Ss2();
    C_raw2 = readAdc0Ss2();
    C_raw3 = readAdc0Ss2();
    C_raw4 = readAdc0Ss2();

    // Set Trigger state to True
    COMP_TRIG = true;

    // Clear Interrupt Status
    COMP_ACMIS_R = COMP_ACMIS_IN0;
}
