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
    establishConncection();
    m_packetLayer.setTransiveHandler(usbLinkCommand());
    sendLinkStatus(LinkStatus::StatusDebug);
    while (true)
    {
        usbLink_loadTransivePacket();

        auto command = m_packetLayer.getCommand();
        NVIC_EnableIRQ(USB_IRQn);

        UsbLayer::getInstance().sendData(std::span(reinterpret_cast<const uint8_t*>(command.data()), 16));

        if (command[0] == LINKCMD_READY_CLOSE_LINK)
        {
            m_packetLayer.setTransiveHandler(readyCloseLinkCommand());
            break;
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
            m_packetLayer.setMode(PacketLayer::Mode::slave);
            k_event_post(&m_connectEvent, modeEvent);
            break;
        case LinkModeCommand::SetModeSlave:
            m_packetLayer.setMode(PacketLayer::Mode::master);
            k_event_post(&m_connectEvent, modeEvent);
            break;
        case LinkModeCommand::StartHandshake: return m_packetLayer.enableHandshake();
        case LinkModeCommand::ConnectLink: return m_packetLayer.connect();
        default: break;
    }
}

void LinkModule::establishConncection()
{
    sendLinkStatus(LinkStatus::HandshakeWaiting);
    k_event_wait(&m_connectEvent, modeEvent, true, K_FOREVER);

    while(m_packetLayer.getReceivedHandshake() != LINK_SLAVE_HANDSHAKE) { }
    sendLinkStatus(LinkStatus::HandshakeReceived);

    while(!m_packetLayer.isHandshakeEnabled()) { }
    sendLinkStatus(LinkStatus::StatusDebug);

    switch (m_packetLayer.getMode())
    {
        case PacketLayer::Mode::master:
            sendLinkStatus(LinkStatus::StatusDebug);
            while(m_packetLayer.getTransmittedHandshake() != LINK_MASTER_HANDSHAKE) { }
            sendLinkStatus(LinkStatus::StatusDebug);
            return;
        case PacketLayer::Mode::slave:
            while(m_packetLayer.getReceivedHandshake() != LINK_MASTER_HANDSHAKE) { }
            sendLinkStatus(LinkStatus::LinkConnected);
            return;
    }
}
