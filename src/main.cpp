#include <zephyr/kernel.h>

#include "./layers/packetLayer.hpp"
#include "./sections/tradeSetup.hpp"
#include "./sections/tradeConnection.hpp"
#include "./sections/tradeDisconnected.hpp"

#include "link_defines.h"

#include "payloads/pokemon.hpp"

int main(void)
{
    PacketLayer g_packetLayer = PacketLayer();
    TradeSetup g_tradeSetup(g_packetLayer);
    TradeConnection g_tradeConnection(g_packetLayer);
    TradeDisconnect g_tradeDisconnect(g_packetLayer);

    while (true)
    {
        auto command = g_packetLayer.getCommand();

        switch(command[0])
        {
            
            case LINKCMD_SEND_LINK_TYPE:
                switch(command[1])
                {
                    case LINKTYPE_TRADE_SETUP:
                        g_tradeSetup.process();
                        g_packetLayer.changeLinkLayerDirection();
                        g_tradeConnection.process(); // connection section is master mode
                        g_packetLayer.changeLinkLayerDirection();

                    case LINKTYPE_TRADE_DISCONNECTED:
                        g_tradeDisconnect.process();
                        g_packetLayer.changeLinkLayerDirection();
                        g_tradeConnection.process(); // connection section is master mode
                        g_packetLayer.changeLinkLayerDirection();
                        break;
                    
                    case LINKTYPE_TRADE:
                        break;
                }
                break;
        }
    }

    return 0;
}