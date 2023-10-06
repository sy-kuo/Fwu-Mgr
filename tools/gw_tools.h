#ifndef __GW_TOOLS_H__
#define __GW_TOOLS_H__

#include "mbed.h"

class GW_BufLen
{
public:
    GW_BufLen(char * data = NULL, uint32_t length = NULL)
    {
        uint32_t _length = length == NULL? strlen(data): length;
        buffer = (char *)calloc(_length + 1, sizeof(char));
        if(buffer != NULL)
        {
            len = _length;
            if(data != NULL)
                memcpy(buffer, data, len);
        }
        //printf(" Mem: %p \r\n", buffer);
    }

    GW_BufLen(const char * format, ...)
    {
#define DATA_MAX_LENGTH       1024
        char data[DATA_MAX_LENGTH] = {0};

        va_list args;
        va_start(args, format);
        vsprintf(data, format, args);
        va_end(args);

        len = strlen(data);
        buffer = (char *)calloc(len + 1, sizeof(char));
        if(buffer != NULL)
            memcpy(buffer, data, len);
        //printf(" Mem: %p \r\n", buffer);
    }

    ~GW_BufLen()
    {
        //printf("~Mem: %p \r\n", buffer);
        if(buffer != NULL)
            free(buffer);
        buffer = NULL;
        len = NULL;
    }

    char * buffer;
    uint32_t len;
};

#endif