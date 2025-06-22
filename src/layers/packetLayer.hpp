
extern "C"
{
    #include "linkLayer.h"
}

#include "../callbacks/commands.hpp"

#include "../link_defines.h"

#include <span>

#include <zephyr/drivers/gpio.h>

#pragma once

class PacketLayer
{
private:

    enum class TransiveState
    {
        crc,
        command,
        handshake
    };

public:
    PacketLayer()
    {
        link_setTransmitCallback(&transmitCallback, this);
        link_setReceiveCallback(&receiveCallback, this);
        link_setTransiveDoneCallback(&transiveDoneCallback, this);
        
        k_sem_init(&m_commandRxCompleteSemaphore, 0, 1);
        k_timer_init(&m_timeoutTimer, &packetTimeout, nullptr);
        k_timer_user_data_set(&m_timeoutTimer, this);
    }

    void setTransiveHandler(struct TransiveStruct handler)
    {
        if (!m_idle) return; 
        m_handler = handler;
        m_idle = false;
        if (handler.init != nullptr) handler.init(handler.userData);
    }
    
    std::span<const uint16_t, 8> getCommand() 
    { 
        k_sem_take(&m_commandRxCompleteSemaphore, K_FOREVER);
        return std::span(m_receivedCommand); 
    }

    void reset() 
    { 
        m_state = TransiveState::handshake;
        m_idle = true;
        m_commandIndex = 0;
        m_receivedHandshake = 0x00;
        m_transmitHandShake = LINK_SLAVE_HANDSHAKE;
        m_handshakeCount = 0;
        m_crc = LINK_SLAVE_HANDSHAKE;
        m_handler = emptyCommand();
    }

    bool idle() { return m_idle; }

    void changeLinkLayerDirection() 
    {   
        enum LinkMode mode = SLAVE;
        if (link_getMode() == mode) mode = MASTER;
        link_changeMode(mode); 
    }

private:

    //-////////////////////////////////////////////////////////////////////////////////////////////////////////-//

    void onReceive(uint16_t rxBytes)
    {
        switch(m_state)
        {
            case TransiveState::crc: return receiveCrc(rxBytes);
            case TransiveState::command: return receiveCommand(rxBytes);
            case TransiveState::handshake: return receiveHandshake(rxBytes);
            default: return;
        };
    }

    uint16_t onTransmit()
    {
        switch(m_state)
        {
            case TransiveState::crc: return transmitCrc();
            case TransiveState::command: return transmitCommand();
            case TransiveState::handshake: return transmitHandshake();
            default: return 0x00;
        };
    }

    //-////////////////////////////////////////////////////////////////////////////////////////////////////////-//

    void receiveHandshake(uint16_t rxBytes)
    {
        m_receivedHandshake = rxBytes;
    }

    uint16_t transmitHandshake()
    {
        if (link_getMode() == MASTER)
        {
            m_handshakeCount++;
        }
        if (m_handshakeCount >= 200)
        {
            m_handshakeCount = 0;
            m_transmitHandShake = LINK_MASTER_HANDSHAKE;
        }
        return m_transmitHandShake;
    }

    //-////////////////////////////////////////////////////////////////////////////////////////////////////////-//

    void receiveCrc(uint16_t rxBytes)
    {
        // don't care
    }

    uint16_t transmitCrc()
    {
        return m_crc;
    }

    //-////////////////////////////////////////////////////////////////////////////////////////////////////////-//

    void receiveCommand(uint16_t rxBytes)
    {
        m_receivedCommand[m_commandIndex] = rxBytes;
        m_crc += rxBytes;
        m_commandIndex++;
    }

    uint16_t transmitCommand()
    {
        uint16_t m_transmittedBytes = (m_handler.transive != nullptr) ? m_handler.transive() : 0x00;
        m_crc += m_transmittedBytes;
        return m_transmittedBytes;
    }

    //-////////////////////////////////////////////////////////////////////////////////////////////////////////-//

    void onTransiveDone()
    {
        switch (m_state)
        {
            case TransiveState::handshake:
                if (m_receivedHandshake == LINK_MASTER_HANDSHAKE 
                    || m_transmitHandShake == LINK_MASTER_HANDSHAKE)
                {
                    m_state = TransiveState::crc;
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

private:
    bool m_idle = true;

    uint16_t m_handshakeCount = 0;
    uint16_t m_receivedHandshake = LINK_SLAVE_HANDSHAKE;
    uint16_t m_transmitHandShake = LINK_SLAVE_HANDSHAKE;
    uint16_t m_crc = LINK_SLAVE_HANDSHAKE; //first crc is always handshake

    struct k_sem m_commandRxCompleteSemaphore;

    int m_commandIndex = 0;
    std::array<uint16_t, 8> m_receivedCommand = {};

    TransiveStruct m_handler = emptyCommand();
    TransiveState m_state = TransiveState::handshake;

    struct k_timer m_timeoutTimer;

    //-////////////////////////////////////////////////////////////////////////////////////////////////////////-//
    // Callbacks
    //-////////////////////////////////////////////////////////////////////////////////////////////////////////-//

    static void receiveCallback(uint16_t rxBytes, void* userData)
    {
        PacketLayer* self = static_cast<PacketLayer*>(userData);
        return self->onReceive(rxBytes);
    }

    static uint16_t transmitCallback(void* userData)
    {
        PacketLayer* self = static_cast<PacketLayer*>(userData);
        return self->onTransmit();
    }

    static void transiveDoneCallback(void* userData)
    {
        PacketLayer* self = static_cast<PacketLayer*>(userData);
        self->onTransiveDone();
    }

    static void packetTimeout(struct k_timer *timer)
    {
        void* userData = k_timer_user_data_get(timer);
        PacketLayer* self = static_cast<PacketLayer*>(userData);
        self->reset();
    }
};