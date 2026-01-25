#include "link.hpp"
#include "../linkStatus.hpp"
#include "../callbacks/commands.hpp"
#include "syscalls/kernel.h"
#include "zephyr/kernel.h"
#include <zephyr/irq.h>


void LinkModule::execute()
{
    m_packetLayer.reset();
    m_packetLayer.disableHandshake();
    sendLinkStatus(LinkStatus::HandshakeWaiting);

    while(m_packetLayer.getReceivedHandshake() != LINK_SLAVE_HANDSHAKE) { }
    k_sem_take(&m_waitForLinkModeCommand, K_FOREVER);
    
    m_packetLayer.setMode(m_packetLayerMode);

    establishConncection();
    m_packetLayer.setTransiveHandler(usbLinkCommand());

    bool keepAlive = true;

    bool partnerReadyCloseLink = false;
    bool readyCloseLink = false;
    
    while (true)
    {

        PacketLayer::TransiveResult result = m_packetLayer.awaitTransiveResults();

        if (partnerReadyCloseLink && readyCloseLink)
        {   
            if (keepAlive)
            {
                sendLinkStatus(LinkStatus::LinkReconnecting);
                m_packetLayer.disableHandshake();
                m_packetLayer.reset(); // disable sync
                m_packetLayer.setTransiveHandler(usbLinkCommand());
                k_sleep(K_MSEC(400));
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

        k_sleep(K_MSEC(5));
    }
}

void LinkModule::receiveCommand(std::span<const uint8_t> command)
{
    switch (static_cast<LinkModeCommand>(command[0]))
    {
        case LinkModeCommand::SetModeMaster:
            m_packetLayerMode = PacketLayer::Mode::master;
            k_sem_give(&m_waitForLinkModeCommand);
            break;
        case LinkModeCommand::SetModeSlave:
            m_packetLayerMode = PacketLayer::Mode::slave;
            k_sem_give(&m_waitForLinkModeCommand);            
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

