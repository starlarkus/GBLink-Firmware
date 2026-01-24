
#include "../layers/packetLayer.hpp"

#pragma once 

namespace section 
{
    inline void connectAsMaster(PacketLayer& packetLayer, bool& m_cancel)
    {
        packetLayer.setMode(PacketLayer::Mode::master);
        while(packetLayer.getReceivedHandshake() != LINK_SLAVE_HANDSHAKE) 
        {
            if (m_cancel) return;
        }
        packetLayer.enableHandshake();
        k_sleep(K_MSEC(500));
        packetLayer.connect();
    }

    inline void connectAsSlave(PacketLayer& packetLayer, bool& m_cancel)
    {
        packetLayer.setMode(PacketLayer::Mode::slave);
        while(packetLayer.getReceivedHandshake() != LINK_SLAVE_HANDSHAKE) 
        {
            if (m_cancel) return;
        }
        packetLayer.enableHandshake();
    }
}