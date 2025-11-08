#include "emu.hpp"
#include "../linkStatus.hpp"

void EmuModule::execute()
{
    m_cancel = false;
    NextSection nextSection = NextSection::setup;
    while (true)
    {
        switch (nextSection)
        {
            case NextSection::setup:
            {
                m_packetLayer.setMode(PacketLayer::Mode::master);
                connect();
                TradeSetup tradeSetup(m_packetLayer, LINKTYPE_TRADE_SETUP, m_cancel);
                nextSection = tradeSetup.process(); // -> exit / -> connection / -> cancel
                break;
            }
            case NextSection::connection:
            {
                m_packetLayer.setMode(PacketLayer::Mode::master);
                connect();
                TradeConnection tradeConnection(m_packetLayer, m_cancel);
                nextSection = tradeConnection.process(); // -> tradeLounge? / -> disconnect / -> cancel
                break;
            }
            case NextSection::disconnect:
            {
                m_packetLayer.setMode(PacketLayer::Mode::slave);
                connectSlave();
                TradeDisconnect tradeDisconnection(m_packetLayer, m_cancel);
                nextSection = tradeDisconnection.process(); // -> connection / -> cancel
                break;
            }
            case NextSection::lounge:
            {
                m_packetLayer.setMode(PacketLayer::Mode::master);
                connect();
                TradeLounge tradeLounge(m_packetLayer, m_cancel);
                nextSection = tradeLounge.process(); // -> exit / -> connection / -> cancel
                break;
            }
            case NextSection::cancel: [[fallthrough]];
            case NextSection::exit:
            {
                m_cancel = false;
                NVIC_EnableIRQ(USB_IRQn);
                return;
            }
        }
    }
}

void EmuModule::connect()
{
    while(m_packetLayer.getReceivedHandshake() != LINK_SLAVE_HANDSHAKE) 
    {
        if (m_cancel) return;
    }
    m_packetLayer.setMode(PacketLayer::Mode::master);
    m_packetLayer.enableHandshake();
    k_sleep(K_MSEC(500));
    m_packetLayer.connect();
}

void EmuModule::connectSlave()
{
    while(m_packetLayer.getReceivedHandshake() != LINK_SLAVE_HANDSHAKE) 
    {
        if (m_cancel) return;
    }
    m_packetLayer.setMode(PacketLayer::Mode::slave);
    m_packetLayer.enableHandshake();
}