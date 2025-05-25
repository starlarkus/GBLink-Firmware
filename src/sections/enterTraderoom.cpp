#include "enterTraderoom.hpp"

ConnectionStatus EnterTraderoom::process(std::span<const uint8_t> command)
{
    switch(command[CMD_INDEX])
    {
        case LINKCMD_READY_CLOSE_LINK:
            return ConnectionStatus::closed;
        
        case LINKCMD_SEND_LINK_TYPE:
            if (command[1] == LINKTYPE_TRADE_SETUP)
            {
                const struct LinkPlayerBlock* linkPlayerBlock = linkPLayer();
                blockCommandSetup(linkPlayerBlock, sizeof(*linkPlayerBlock), sizeof(*linkPlayerBlock));
                g_transmitHandler = blockCommand_cb;
            }
            break;
    
        case LINKCMD_SEND_BLOCK_REQ:
            const struct TrainerCard* trainerCard = trainerCardPlaceholder();
            blockCommandSetup(trainerCard, sizeof(*trainerCard), 0x64);
            g_transmitHandler = blockCommand_cb;
            break;
        
        case LINKCMD_READY_EXIT_STANDBY:
            g_transmitHandler = readyExitStandbyCommand_cb;
            break;
        
        case LINKCMD_SEND_HELD_KEYS:
            g_transmitHandler = moveCommand_cb;
            break;
    }
    return ConnectionStatus::open;
}