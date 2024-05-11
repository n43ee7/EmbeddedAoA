
#include "include/uart_handler.h"
#include "include/uart0.h"

void getsUart0(USER_DATA *data) {
    int i=0;
    while(i<80) {
        data->buffer[i] = '\0';
        i++;
    }
    int count=0;
    while(1) {
        char c = getcUart0();
        if(c == 8 || (c == 127 && count>0)) {
            count--;
        }
        else if(c == 13) {
            data->buffer[count] = c;
            return;
        }
        else if(c >= 32) {
            data->buffer[count] = c;
            count++;
            if(count==MAX_CHARS)
            {
                data->buffer[count] = '\0';
                return;
            }
        }
    }
}

void parseFields(USER_DATA* data) {
    char f;
    char c;
    int len = 0;
    len = strlen(data->buffer);
    int i=0,j=0,alpha=0,num=0;
    data->fieldCount = 0;

    while(i<len-1) {
        alpha = 0;
        num = 0;
        c = data->buffer[i];

        if(i==0) {
            //Check lowercase
            j=97;
            while(j<123) {
                if(c==j) {
                    //alpha = 1;
                    data->fieldType[0] = 'a';
                    data->fieldPosition[0] = 0;
                    data->fieldCount++;
                }
                j++;
            }

            //Check Uppercase
            j = 65;
            while(j<91) {
                if(c==j) {
                    //alpha = 1;
                    data->fieldType[0] = 'a';
                    data->fieldPosition[0] = 0;
                    data->fieldCount++;
                }
                j++;
            }

            //Check for Numbers
            j = 48;
            while(j<58) {
                if(c==j) {
                    //num = 1;
                    data->fieldType[0] = 'n';
                    data->fieldType[1] = '\0';
                    data->fieldPosition[0] = 0;
                    data->fieldCount++;
                }
                j++;
            }
        }

        j=97;
        while(j<123) {
            if(c==j) {
                alpha = 1;
            }
            j++;
        }

        //Check Uppercase
        j = 65;
        while(j<91) {
            if(c==j) {
                alpha = 1;
            }
            j++;
        }

        //Check for Numbers
        j = 48;
        while(j<58) {
            if(c==j) {
                num = 1;
            }
            j++;
        }

        if(num==0 && alpha==0) {
            f = data->buffer[i+1];
            data->fieldPosition[data->fieldCount] = i+1;

            //Check lowercase
            j=97;
            while(j<123) {
                if(f==j) {
                    //alpha = 1;
                    data->fieldType[data->fieldCount] = 'a';
                    data->fieldPosition[data->fieldCount] = i+1;
                    data->fieldCount++;
                }
                j++;
            }

            //Check Uppercase
            j = 65;
            while(j<91) {
                if(f==j) {
                    //alpha = 1;
                    data->fieldType[data->fieldCount] = 'a';
                    data->fieldPosition[data->fieldCount] = i+1;
                    data->fieldCount++;
                }
                j++;
            }

            //Check for Numbers
            j = 48;
            while(j<58) {
                if(f==j) {
                    //num = 1;
                    data->fieldType[data->fieldCount] = 'n';
                    data->fieldPosition[data->fieldCount] = i+1;
                    data->fieldCount++;
                }
                j++;
            }
            data->buffer[i] = '\0';
        }

        if(data->fieldCount == MAX_FIELDS) {
            return;
        }
        i++;
    }
}

char * getFieldString(USER_DATA*data, uint8_t fieldNumber) {
    return &data->buffer[data->fieldPosition[fieldNumber]];
}

int32_t getFieldInteger(USER_DATA*data, uint8_t fieldNumber) {
    if(data->fieldType[fieldNumber] == 'n');
    {
        return atoi(&data->buffer[data->fieldPosition[fieldNumber]]);
    }
}

int comp(char *string1, char * string2) {
    int length = strlen(string2);
    int i=0;
        for(i=0;i<length;i++) {
            if(string1[i] != string2[i]) {
                return 1;
            }
        }
        return 0;
}

bool isCommand(USER_DATA* data, const char strCommand[], uint8_t minArguments) {
    int length = strlen(strCommand);
    int i=0, check = 0;
    for(i=0;i<length;i++) {
        if(data->buffer[i] != strCommand[i]) {
            check = 1;
        }
    }

    if(comp(&data->buffer[0], strCommand) == 0 && data->fieldCount-1 >= minArguments) {
        return 1;
    }

    else {
        return 0;
    }
}
