#include "moveCommand.h"


uint8_t index = 0;

uint16_t moveCommand_cb()
{
    switch(index)
    {
        case 0:
            index++;
            return 0xCAFE;
        case 1:
            index++;
            return 0x11;
        case 7:
            index = 0;
            return 0x00;
        default:
            index++;
         return 0x00;
    }
}