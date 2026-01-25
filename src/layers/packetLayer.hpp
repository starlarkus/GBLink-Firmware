
#include <cstdint>
extern "C"
{
    #include "linkLayer.h"
}

#include "../callbacks/commands.hpp"
#include "../link_defines.h"

#ifdef CONFIG_STM32F0
#include "masterClock.hpp"
#endif

#include <span>
#include <zephyr/drivers/gpio.h>

#pragma once

class PacketLayer
{
public:
    enum class Mode
    {
        master,
        slave
    };

    struct TransiveResult
    {
        std::span<const uint16_t> received;
        std::span<const uint16_t> transmitted;
    };

private:

    enum class TransiveState
    {
        crc,
        command,
        handshake
    };

    static constexpr uint32_t timingHandshake = 30097;
    static constexpr uint32_t timingCommandBytes = 1378;
    static constexpr uint32_t timingBetweenCommands = 12953;

public:
    PacketLayer()
    {
        link_setTransmitCallback(&transmitCallback, this);
        link_setReceiveCallback(&receiveCallback, this);
        link_setTransiveDoneCallback(&transiveDoneCallback, this);
        
        k_sem_init(&m_commandTransiveSemaphore, 0, 1);
        k_sem_init(&m_handshakeSemaphore, 0, 1);
        #ifdef CONFIG_STM32F0
        k_timer_init(&m_timeoutTimer, &packetTimeout, nullptr);
        k_timer_user_data_set(&m_timeoutTimer, this);
        #endif
    }

    void setTransiveHandler(struct TransiveStruct handler)
    {
        m_handler = handler;
        m_idle = false;
        if (handler.init != nullptr) handler.init(handler.userData);
    }
    
    TransiveResult awaitTransiveResults() 
    { 
        k_sem_take(&m_commandTransiveSemaphore, K_FOREVER);
        return TransiveResult
        {
            std::span(m_receivedCommand),
            std::span(m_transmittedCommand)
        };
    }

    uint16_t getReceivedHandshake()
    {
        k_sem_take(&m_handshakeSemaphore, K_FOREVER);
        return m_receivedHandshake;
    }

    uint16_t getTransmittedHandshake()
    {
        m_waiting = 1;
        k_sem_take(&m_handshakeSemaphore, K_FOREVER);
        m_waiting = 0;
        return m_transmitHandShake;
    }

    void cancel() {  if (m_waiting == 1) k_sem_give(&m_handshakeSemaphore); }

    bool idle() { return m_idle; }

    void enableHandshake() { m_transmitHandShake = LINK_SLAVE_HANDSHAKE; }

    void disableHandshake() { m_transmitHandShake = LINK_HANDSHAKE_DISABLE; }

    void connect() { m_transmitHandShake = LINK_MASTER_HANDSHAKE; }

    bool isHandshakeEnabled() { return m_transmitHandShake == LINK_SLAVE_HANDSHAKE; }

    bool hasSendHandShake() { return m_receivedHandshake == LINK_SLAVE_HANDSHAKE; }

    void reset();

    void setMode(Mode mode) 
    {   
        switch (mode)
        {
            case Mode::master:
                link_changeMode(MASTER);
                #ifdef CONFIG_STM32F0
                m_masterClock.enableSync();
                #endif
                break;
            
            case Mode::slave:
                link_changeMode(SLAVE);
                #ifdef CONFIG_STM32F0
                m_masterClock.disableSync();
                #endif
                break;
        }

        m_mode = mode;
    }

    Mode getMode() { return m_mode; }

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

    struct NextTransmit onTransmit()
    {
        switch(m_state)
        {
            case TransiveState::crc: return {transmitCrc(), m_timingUs};
            case TransiveState::command: return {transmitCommand(), m_timingUs};
            case TransiveState::handshake: return {transmitHandshake(), m_timingUs};
            default: return {0x00, m_timingUs};
        };
    }

    //-////////////////////////////////////////////////////////////////////////////////////////////////////////-//

    void receiveHandshake(uint16_t rxBytes)
    {
        m_receivedHandshake = rxBytes;
    }

    uint16_t transmitHandshake()
    {
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
        uint16_t txBytes = (m_handler.transive != nullptr) ? m_handler.transive() : 0x00;
        m_crc += txBytes;
        m_transmittedCommand[m_transmitCommandIndex] = txBytes;
        m_transmitCommandIndex++;
        return txBytes;
    }

    //-////////////////////////////////////////////////////////////////////////////////////////////////////////-//

    void onTransiveDone();

    //-////////////////////////////////////////////////////////////////////////////////////////////////////////-//

private:
    atomic_t m_waiting = 0;

    bool m_idle = true;
    Mode m_mode = Mode::slave;

    uint16_t m_handshakeCount = 0;
    uint16_t m_receivedHandshake = LINK_HANDSHAKE_DISABLE;
    uint16_t m_transmitHandShake = LINK_HANDSHAKE_DISABLE;
    uint16_t m_crc = LINK_SLAVE_HANDSHAKE; //first crc is always handshake

    uint32_t m_timingUs = 0;

    struct k_sem m_commandTransiveSemaphore;
    struct k_sem m_handshakeSemaphore;

    int m_commandIndex = 0;
    std::array<uint16_t, 8> m_receivedCommand = {};
    int m_transmitCommandIndex = 0;
    std::array<uint16_t, 8> m_transmittedCommand = {};

    TransiveStruct m_handler = emptyCommand();
    TransiveState m_state = TransiveState::handshake;

    struct k_timer m_timeoutTimer;
    #ifdef CONFIG_STM32F0
    MasterClock m_masterClock;
    #endif
    //-////////////////////////////////////////////////////////////////////////////////////////////////////////-//
    // Callbacks
    //-////////////////////////////////////////////////////////////////////////////////////////////////////////-//

    static void receiveCallback(uint16_t rxBytes, void* userData)
    {
        PacketLayer* self = static_cast<PacketLayer*>(userData);
        return self->onReceive(rxBytes);
    }

    static struct NextTransmit transmitCallback(void* userData)
    {
        PacketLayer* self = static_cast<PacketLayer*>(userData);
        return self->onTransmit();
    }

    static void transiveDoneCallback(void* userData)
    {
        PacketLayer* self = static_cast<PacketLayer*>(userData);
        self->onTransiveDone();
    }

    #ifdef CONFIG_STM32F0
    static void packetTimeout(struct k_timer *timer)
    {
        void* userData = k_timer_user_data_get(timer);
        PacketLayer* self = static_cast<PacketLayer*>(userData);
        self->reset();
    }
    #endif
};