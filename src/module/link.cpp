#include "link.hpp"
#include "../linkStatus.hpp"
#include "../callbacks/commands.hpp"
#include <zephyr/irq.h>

namespace 
{
    constexpr uint32_t modeEvent = BIT(0);
    constexpr uint32_t enableHandshaleEvent = BIT(1);
    constexpr uint32_t masterConnectEvent = BIT(2);
}

void LinkModule::execute()
{
    m_packetLayer.reset();
    m_packetLayer.disableHandshake();
    sendLinkStatus(LinkStatus::HandshakeWaiting);

    while(m_packetLayer.getReceivedHandshake() != LINK_SLAVE_HANDSHAKE) { }
    k_event_wait(&m_connectEvent, modeEvent, true, K_FOREVER);
    m_packetLayer.setMode(m_packetLayerMode);

    establishConncection();
    m_packetLayer.setTransiveHandler(usbLinkCommand());

    bool keepAlive = true;

    bool partnerReadyCloseLink = false;
    bool readyCloseLink = false;
    
    while (true)
    {
        auto usbCommand = usbLink_loadTransivePacket();

        if (partnerReadyCloseLink && readyCloseLink)
        {   
            NVIC_EnableIRQ(USB_IRQn);

            if (keepAlive)
            {
                sendLinkStatus(LinkStatus::LinkReconnecting);
                m_packetLayer.disableHandshake();
                k_sleep(K_MSEC(40));
                m_packetLayer.reset(); // disable sync
                m_packetLayer.setTransiveHandler(usbLinkCommand());
                k_sleep(K_MSEC(200));
                m_packetLayer.setMode(m_packetLayerMode); // enable sync again
                establishConncection();
                partnerReadyCloseLink = false;
                readyCloseLink = false;
            }
            else 
            {
                sendLinkStatus(LinkStatus::LinkClosed);
                break;
            }
            
        }

        auto linkCommand = m_packetLayer.getCommand();

        NVIC_EnableIRQ(USB_IRQn);

        UsbLayer::getInstance().sendData(std::span(reinterpret_cast<const uint8_t*>(linkCommand.data()), 16));

        if ((linkCommand[0] == LINKCMD_SEND_HELD_KEYS) && (linkCommand[1] == LINK_KEY_CODE_EXIT_ROOM))
        {
            keepAlive = false;
        } 

        if (usbCommand.has_value() && ((usbCommand.value()[0] == LINKCMD_SEND_HELD_KEYS) && (usbCommand.value()[1] == LINK_KEY_CODE_EXIT_ROOM)))
        {
            keepAlive = false;
        }

        if (linkCommand[0] == LINKCMD_READY_CLOSE_LINK)
        {
            partnerReadyCloseLink = true;
        }

        if (usbCommand.has_value() && usbCommand.value()[0] == LINKCMD_READY_CLOSE_LINK)
        {
            readyCloseLink = true;
        }

        k_sleep(K_MSEC(5));
        NVIC_DisableIRQ(USB_IRQn);
    }
}

void LinkModule::receiveCommand(std::span<const uint8_t> command)
{
    switch (static_cast<LinkModeCommand>(command[0]))
    {
        case LinkModeCommand::SetModeMaster:
            m_packetLayerMode = PacketLayer::Mode::master;
            k_event_post(&m_connectEvent, modeEvent);
            break;
        case LinkModeCommand::SetModeSlave:
            m_packetLayerMode = PacketLayer::Mode::slave;
            k_event_post(&m_connectEvent, modeEvent);
            break;
        case LinkModeCommand::StartHandshake: return m_packetLayer.enableHandshake();
        case LinkModeCommand::ConnectLink:
        {
           if (m_packetLayer.getMode() == PacketLayer::Mode::master)
           {
                return m_packetLayer.connect();
           }
        } 
        default: break;
    }
}

void LinkModule::establishConncection()
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

