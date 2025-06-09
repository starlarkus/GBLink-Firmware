#include "tradeConnection.hpp"

#include "../payloads/pokemon.hpp"
#include "../payloads/mail.hpp"
#include "../payloads/linkPlayer.hpp"

#include "../callbacks/commands.hpp"

#include <algorithm>

void TradeConnection::process()
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
                    case TradeConnectionState::LinkPlayer:
                    {
                        const struct LinkPlayerBlock* linkPlayerBlock = linkPLayer();
                        blockCommandSetup(linkPlayerBlock, sizeof(*linkPlayerBlock), sizeof(*linkPlayerBlock));
                        m_blockState = TradeConnectionState::PartyPart0;
                        break;
                    }

                    case TradeConnectionState::PartyPart0:
                    {
                        const auto party = std::as_bytes(getParty().subspan<0, 2>());
                        blockCommandSetup(party.data(), party.size(), 200);
                        m_blockState = TradeConnectionState::PartyPart1;
                        break;
                    }

                    case TradeConnectionState::PartyPart1:
                    {
                        const auto party = std::as_bytes(getParty().subspan<2, 2>());
                        blockCommandSetup(party.data(), party.size(), 200);
                        m_blockState = TradeConnectionState::PartyPart2;
                        break;
                    }

                    case TradeConnectionState::PartyPart2:
                    {
                        const auto party = std::as_bytes(getParty().subspan<4, 2>());
                        blockCommandSetup(party.data(), party.size(), 200);
                        m_blockState = TradeConnectionState::Mail;
                        break;
                    }
                    
                    case TradeConnectionState::Mail:
                    {
                        const auto mail = getEmptyMailPayload();
                        blockCommandSetup(mail.data(), mail.size(), 220);
                        m_blockState = TradeConnectionState::Ribbons;
                        break;
                    }

                    case TradeConnectionState::Ribbons:
                    {
                        blockCommandSetup(nullptr, 0, 40);
                        break;
                    }

                    default: break;
                }
                m_packetLayer.setTransiveHandler(transive);
                break;
            }
            
            case LINKCMD_READY_CLOSE_LINK:
                m_packetLayer.reset();
                return;
            
            default: break;
        }
    }
}