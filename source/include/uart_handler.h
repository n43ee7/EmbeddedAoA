/*
 * uart_handler.h
 *
 *  Created on: Apr 17, 2024
 *      Author: nabee
 */

#ifndef INCLUDE_UART_HANDLER_H_
#define INCLUDE_UART_HANDLER_H_

#include <stdio.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include "include/uart0.h"

#define MAX_CHARS 100
#define MAX_FIELDS 5
#define FCYC            40e6         // System Clock Constant
#define BUAD_RATE       115200       // UART0 Buad-Rate

typedef struct _USER_DATA {
    char buffer[MAX_CHARS+1];
    uint8_t fieldCount;
    uint8_t fieldPosition[MAX_FIELDS];
    char fieldType[MAX_FIELDS];
} USER_DATA;


void getsUart0(USER_DATA *data);
void parseFields(USER_DATA* data);
char * getFieldString(USER_DATA*data, uint8_t fieldNumber);
int32_t getFieldInteger(USER_DATA*data, uint8_t fieldNumber);
int comp(char *string1, char * string2);
bool isCommand(USER_DATA* data, const char strCommand[], uint8_t minArguments);


#endif /* INCLUDE_UART_HANDLER_H_ */
