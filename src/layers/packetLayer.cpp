#include "packetLayer.hpp"

void PacketLayer::onTransiveDone()
{
    switch (m_state)
    {
        case TransiveState::handshake:
            k_sem_give(&m_handshakeSemaphore);
            if (m_receivedHandshake == LINK_MASTER_HANDSHAKE 
                || m_transmitHandShake == LINK_MASTER_HANDSHAKE)
            {
                m_state = TransiveState::crc;
                if (m_mode == Mode::master) m_masterClock.startTransmissionSync();
            }
            break;

        case TransiveState::crc:
            k_timer_start(&m_timeoutTimer, K_MSEC(14), K_NO_WAIT);
            m_state = TransiveState::command;
            m_crc = 0x00;
            break;

        case TransiveState::command:
            
            if (m_commandIndex != 8) return;

            m_commandIndex = 0;
            m_state = TransiveState::crc;
            k_timer_stop(&m_timeoutTimer);
            k_sem_give(&m_commandRxCompleteSemaphore);
            if (m_handler.transiveDone != nullptr && m_handler.transiveDone() == CommandState::done)
            {
                m_idle = true;
                m_handler = emptyCommand();
            }
            break;
        
        default: break;
    }
}

void PacketLayer::reset()
{
    m_state = TransiveState::handshake;
    m_idle = true;
    m_commandIndex = 0;
    m_receivedHandshake = LINK_HANDSHAKE_DISABLE;
    m_transmitHandShake = LINK_HANDSHAKE_DISABLE;
    m_handshakeCount = 0;
    m_crc = LINK_SLAVE_HANDSHAKE;
    m_handler = emptyCommand();
    m_receivedCommand = {};
    m_masterClock.disableSync();
}

bool PacketLayer::isGameboyConnected()
{
    disableHandshake();
    m_state = TransiveState::handshake;
    setMode(PacketLayer::Mode::master);
    k_busy_wait(25000);
    bool connectionCheck = (
        (m_receivedHandshake == 0xFFFC) ||
        (m_receivedHandshake == 0xFFFE) ||
        (m_receivedHandshake == LINK_SLAVE_HANDSHAKE)
    );
    reset();
    link_reset();
    setMode(PacketLayer::Mode::slave);
    return connectionCheck;
}