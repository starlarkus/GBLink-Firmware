#include "tradeConnection.hpp"

#include "../payloads/pokemon.hpp"
#include "../payloads/mail.hpp"
#include "../payloads/linkPlayer.hpp"

#include "../callbacks/commands.hpp"

#include <algorithm>

void TradeConnection::handleInitialDataExchange()
{
   
    m_packetLayer.setTransiveHandler(sendLinkTypeCommand(LINKTYPE_BATTLE));

    while (true)
    {
        auto command = m_packetLayer.getCommand();

        if (m_requestBlock)
        {
            m_requestBlock = false;
            m_packetLayer.setTransiveHandler(sendBlockCommandRequestCommand(2));
            continue;
        }


        switch(command[0])
        {
            case LINKCMD_INIT_BLOCK:
            {
                auto transive = blockCommand();

                switch(m_blockState)
                {
                    case TradeConnectionState::LinkPlayer:
                    {
                        const struct LinkPlayerBlock* linkPlayerBlock = corruptedLinkPLayer(LINKTYPE_BATTLE);
                        blockCommandSetup(linkPlayerBlock, sizeof(*linkPlayerBlock), sizeof(*linkPlayerBlock));
                        m_blockState = TradeConnectionState::PartyPart0;
                        k_timer_start(&m_commandRequestTimer, K_MSEC(1000), K_NO_WAIT);
                        break;
                    }

                    // case TradeConnectionState::PartyPart0:
                    // {
                    //     const auto party = std::as_bytes(getParty().subspan<0, 2>());
                    //     blockCommandSetup(party.data(), party.size(), 200);
                    //     m_blockState = TradeConnectionState::PartyPart1;
                    //     k_timer_start(&m_commandRequestTimer, K_MSEC(1000), K_NO_WAIT);
                    //     break;
                    // }

                    // case TradeConnectionState::PartyPart1:
                    // {
                    //     const auto party = std::as_bytes(getParty().subspan<2, 2>());
                    //     blockCommandSetup(party.data(), party.size(), 200);
                    //     m_blockState = TradeConnectionState::PartyPart2;
                    //     k_timer_start(&m_commandRequestTimer, K_MSEC(1000), K_NO_WAIT);
                    //     break;
                    // }

                    // case TradeConnectionState::PartyPart2:
                    // {
                    //     const auto party = std::as_bytes(getParty().subspan<4, 2>());
                    //     blockCommandSetup(party.data(), party.size(), 200);
                    //     k_timer_start(&m_commandRequestTimer, K_MSEC(1000), K_NO_WAIT);
                    //     break;
                    // }
                    
                    // case TradeConnectionState::Mail:
                    // {
                    //     const auto mail = getEmptyMailPayload();
                    //     blockCommandSetup(mail.data(), mail.size(), 220);
                    //     m_blockState = TradeConnectionState::Ribbons;
                    //     k_timer_start(&m_commandRequestTimer, K_MSEC(1000), K_NO_WAIT);
                    //     break;
                    // }

                    // case TradeConnectionState::Ribbons:
                    // {
                    //     blockCommandSetup(nullptr, 0, 40); 
                    //     m_blockState = TradeConnectionState::LinkCMD;
                    //     break;
                    // }

                    default: break;

                }
                m_packetLayer.setTransiveHandler(transive);
                break;
            }
            
            default:
            {
                if (m_blockState == TradeConnectionState::LinkCMD && m_packetLayer.idle())
                {
                    return;
                }
            }
        }
    }
}

void TradeConnection::handleTradeNegotiations()
{
    std::array<uint16_t, 2> command = {LINKCMD_READY_TO_TRADE, 0x01}; //Hard Code second party member for now
    blockCommandSetup(command.data(), command.size(), 20);
    m_packetLayer.setTransiveHandler(blockCommand());
    
    while(true)
    {
        auto command = m_packetLayer.getCommand();

        if (command[0] == LINKCMD_READY_CLOSE_LINK)
        {
            m_packetLayer.setTransiveHandler(readyCloseLinkCommand());
            return;
        }

        if (command[0] != LINKCMD_CONT_BLOCK) continue;

        switch (command[1])
        {
            case LINKCMD_INIT_BLOCK: //Here INIT_BLOCK is used to signal that the Pokemon is valid, why they didn't use a new value, God knows why ¯\_(ツ)_/¯
            {
                std::array<uint16_t, 2> command = {LINKCMD_INIT_BLOCK, 0x00};
                blockCommandSetup(command.data(), command.size(), 20);
                m_packetLayer.setTransiveHandler(blockCommand());
                break;
            }
                
            case LINKCMD_READY_CLOSE_LINK:
                m_packetLayer.setTransiveHandler(readyCloseLinkCommand());
                return;
            
            default: break;
        }
    }
}

void TradeConnection::process()
{
   handleInitialDataExchange();
   handleTradeNegotiations();
}



//         case LINKCMD_READY_TO_TRADE:
//         case LINKCMD_READY_FINISH_TRADE:
//         case LINKCMD_READY_CANCEL_TRADE:
//         case LINKCMD_START_TRADE:
//         case LINKCMD_CONFIRM_FINISH_TRADE:
//         case LINKCMD_SET_MONS_TO_TRADE:
//         case LINKCMD_PLAYER_CANCEL_TRADE:
//         case LINKCMD_REQUEST_CANCEL:
//         case LINKCMD_BOTH_CANCEL_TRADE:
//         case LINKCMD_PARTNER_CANCEL_TRADE:
