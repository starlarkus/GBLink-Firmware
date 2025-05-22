#include "readyExitStandbyCommand.h"
#include "../link_defines.h"

static uint8_t index = 0;

uint16_t readyExitStandbyCommand_cb()
{
    switch(index)
    {
        case 0:
            index++;
            return LINKCMD_READY_EXIT_STANDBY;
        case 7:
            index = 0;
            return 0x00;
        default:
            index++;
         return 0x00;
    }
}