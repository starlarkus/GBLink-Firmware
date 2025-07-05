#include <zephyr/kernel.h>

#include "./layers/packetLayer.hpp"
#include "./sections/tradeSetup.hpp"
#include "./sections/tradeConnection.hpp"
#include "./sections/tradeDisconnected.hpp"

#include "link_defines.h"

int main(void)
{
    PacketLayer g_packetLayer = PacketLayer();
    {
        TradeSetup tradeSetup(g_packetLayer, 0x2233);
        tradeSetup.process();
    }
    {
        TradeConnection tradeConnection(g_packetLayer);
        tradeConnection.process();
    }

    while (true)
    {
        auto command = g_packetLayer.getCommand();

        switch(command[0])
        {
            
            case LINKCMD_SEND_LINK_TYPE:
                switch(command[1])
                {
                    case LINKTYPE_TRADE_SETUP:
                    {
                        {
                            //TradeSetup tradeSetup(g_packetLayer);
                            //tradeSetup.process();
                        }
                        {
                            TradeConnection tradeConnection(g_packetLayer);
                            tradeConnection.process();
                        }
                        break;
                    }

                    case LINKTYPE_TRADE_DISCONNECTED:
                    {
                        {
                            TradeDisconnect tradeDisconnect(g_packetLayer);
                            tradeDisconnect.process();
                        }
                        {
                            TradeConnection tradeConnection(g_packetLayer);
                            tradeConnection.process();
                        }
                        break;
                    }
                    
                    case LINKTYPE_TRADE:
                        break;
                }
                break;
        }
    }

    return 0;
}