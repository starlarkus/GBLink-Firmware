
#include "usbSection.hpp"

#include "../linkStatus.hpp"

void UsbSection::establishConncection()
{
    while(m_packetLayer.getReceivedHandshake() != LINK_SLAVE_HANDSHAKE) { }
    sendLinkStatus(LinkStatus::HandshakeReceived);

    while(!m_packetLayer.isHandshakeEnabled()) { }

    switch (m_packetLayerMode)
    {
        case PacketLayer::Mode::master:
            while(m_packetLayer.getTransmittedHandshake() != LINK_MASTER_HANDSHAKE) { }
            sendLinkStatus(LinkStatus::LinkConnected);
            return;

        case PacketLayer::Mode::slave:
            while(m_packetLayer.getReceivedHandshake() != LINK_MASTER_HANDSHAKE) { }
            sendLinkStatus(LinkStatus::LinkConnected);
            return;
    }
}

bool UsbSection::process()
{
    bool keepAlive = true;
    bool partnerReadyCloseLink = false;
    bool readyCloseLink = false;
    establishConncection();

    while(true)
    {
        PacketLayer::TransiveResult result = m_packetLayer.awaitTransiveResults();
        
        UsbLayer::getInstance().sendData(std::span(reinterpret_cast<const uint8_t*>(result.received.data()), 16));

        if ((result.received[0] == LINKCMD_SEND_HELD_KEYS) && (result.received[1] == LINK_KEY_CODE_EXIT_ROOM))
        {
            keepAlive = false;
        } 

        if (result.transmitted[0] == LINKCMD_SEND_HELD_KEYS && result.transmitted[1] == LINK_KEY_CODE_EXIT_ROOM)
        {
            keepAlive = false;
        }

        if (result.received[0] == LINKCMD_READY_CLOSE_LINK)
        {
            partnerReadyCloseLink = true;
        }

        if (result.transmitted[0] == LINKCMD_READY_CLOSE_LINK)
        {
            readyCloseLink = true;
        }

        if (partnerReadyCloseLink && readyCloseLink)
        {   
            return keepAlive;
        }
    }
}