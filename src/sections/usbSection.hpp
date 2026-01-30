
#include "../layers/packetLayer.hpp"

#pragma once

class UsbSection
{
public:
    UsbSection(PacketLayer::Mode mode) : m_packetLayerMode(mode)
    {
        m_packetLayer.setTransiveHandler(usbLinkCommand());
        m_packetLayer.setMode(m_packetLayerMode);
    }

    bool process();

    void startHandshake() { m_packetLayer.enableHandshake(); }

    void connectLink() 
    { 
        if (m_packetLayer.getMode() == PacketLayer::Mode::master) m_packetLayer.connectHandshake();
    }

private:

    void establishConncection();

    PacketLayer m_packetLayer;
    PacketLayer::Mode m_packetLayerMode;
};