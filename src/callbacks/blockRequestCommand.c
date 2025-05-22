#include "blockRequestCommand.h"


uint8_t g_index = 0;

static uint16_t g_blockCommandContent[8] = {0xCCCC, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

uint16_t blockRequest_cb()
{
    uint16_t ret = g_blockCommandContent[g_index];
    

    g_index++;
    if (g_index == 8)
    {
        g_index = 0;
    } 

    return ret;
}