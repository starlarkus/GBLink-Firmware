#include "tradeLounge.hpp"
extern "C"
{
    #include "../payloads/trainerCard.h"
    #include "../payloads/linkPlayer.h"
}
#include "../callbacks/commands.hpp"
#include <algorithm>

NextSection TradeLounge::process()
{
    m_packetLayer.setTransiveHandler(sendLinkTypeCommand(LINKTYPE_TRADE));
    NextSection nextSection = NextSection::connection;

    while (!m_cancel)
    {
        auto command = m_packetLayer.getCommand();
        //NVIC_EnableIRQ(USB_IRQn);

        switch(command[0])
        {
            case LINKCMD_INIT_BLOCK:
            {
                auto transive = blockCommand();
                const struct LinkPlayerBlock* linkPlayerBlock = linkPLayer(LINKTYPE_TRADE);
                blockCommandSetup(linkPlayerBlock, sizeof(*linkPlayerBlock), sizeof(*linkPlayerBlock));
                m_packetLayer.setTransiveHandler(transive);
                break;
            }
            
            case LINKCMD_SEND_HELD_KEYS:
            {
                if (command[1] == LINK_KEY_CODE_EXIT_ROOM)
                {
                    moveCommandInit(LINK_KEY_CODE_EXIT_ROOM);
                    m_packetLayer.setTransiveHandler(moveCommand());
                    nextSection = NextSection::exit;
                } 
                if (command[1] == LINK_KEY_CODE_READY)
                {   
                    moveCommandInit(LINK_KEY_CODE_READY);
                    m_packetLayer.setTransiveHandler(moveCommand());
                    nextSection = NextSection::connection;
                }
                break;
            }
            
            case LINKCMD_READY_CLOSE_LINK:
            {
                m_packetLayer.setTransiveHandler(readyCloseLinkCommand());
                k_sleep(K_MSEC(40));
                m_packetLayer.reset(); //master
                k_sleep(K_MSEC(200));
                return nextSection;
            }
            
            default: break;
        }

        k_sleep(K_MSEC(5));
        //NVIC_DisableIRQ(USB_IRQn);
    }
    return NextSection::cancel; // user canceled from web interface
}