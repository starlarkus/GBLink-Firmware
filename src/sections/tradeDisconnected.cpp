#include "tradeDisconnected.hpp"

extern "C"
{
    #include "../payloads/linkPlayer.h"
}

#include "../callbacks/commands.hpp"

#include <algorithm>


void TradeDisconnect::exchangeTrainerData()
{
    while(true)
    {
        auto command = m_packetLayer.getCommand();
        switch(command[0])
        {
            case LINKCMD_INIT_BLOCK:
            {
                auto transive = blockCommand();

                switch(m_blockState)
                {
                    case BlockCommandState::None:
                    {
                        const struct LinkPlayerBlock* linkPlayerBlock = linkPLayer(LINKTYPE_TRADE_DISCONNECTED);
                        blockCommandSetup(linkPlayerBlock, sizeof(*linkPlayerBlock), sizeof(*linkPlayerBlock));
                        m_packetLayer.setTransiveHandler(blockCommand());
                        m_blockState = BlockCommandState::LinkPlayer;
                        break;
                    }
                    default: break;
                }
                break;
            }

            default:
            {
                if (m_blockState == BlockCommandState::LinkPlayer && m_packetLayer.idle())
                {
                    return;
                }
            }
        }
    }
}

void TradeDisconnect::handleDisconnect()
{
    while(true)
    {
        auto command = m_packetLayer.getCommand();

        switch (command[0])
        {
            case LINKCMD_READY_EXIT_STANDBY:
                m_packetLayer.setTransiveHandler(readyExitStandbyCommand());
                break;
            
            case LINKCMD_READY_CLOSE_LINK:
                m_packetLayer.setTransiveHandler(readyCloseLinkCommand());
                break;

            case LINKCMD_CONT_BLOCK:
            {
                switch (command[1])
                {
                    case LINKCMD_READY_FINISH_TRADE:
                    {
                        std::array<uint16_t, 2> command = {LINKCMD_READY_FINISH_TRADE};
                        blockCommandSetup(command.data(), command.size(), 20);
                        m_packetLayer.setTransiveHandler(blockCommand());
                        break;
                    }
                        
                    case LINKCMD_CONFIRM_FINISH_TRADE:
                    {
                        std::array<uint16_t, 2> command = {LINKCMD_CONFIRM_FINISH_TRADE};
                        blockCommandSetup(command.data(), command.size(), 20);
                        m_packetLayer.setTransiveHandler(blockCommand());
                        break;
                    }
                    
                    default: break;
                }
            }
        }
    }
}


void TradeDisconnect::process()
{
    exchangeTrainerData();
    handleDisconnect();   
}