//-----------------------------------------------------------------------------
// Project | Device Operation Library Mega Functions SOURCE FILE
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

#ifndef INCLUDE_DEVICE_OPS_H_
#define INCLUDE_DEVICE_OPS_H_

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

// System Constants
#define SAMPLE_LENGTH 300
#define SAMPLE_DIV    100
// Bitwise definition for PC7 Co -ve analog comparator
#define CON (*((volatile uint32_t *)(0x42000000 + (0x400063FC-0x40000000)*32 + 7*4)))
#define CON_MASK        128          // PC7

// Device Operations Routines
void ADCSS0EN(void);
void initCOMPARATOR(void);
void initDMA(void);
void initHw(void);
void HWReboot(void);

#endif /* INCLUDE_DEVICE_OPS_H_ */
