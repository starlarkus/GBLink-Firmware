
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
        link_setTransiveCallback(&transiveCallback, this);
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

    void reset() { m_state = TransiveState::handshake; }

    bool idle() { return m_idle; }

private:
    uint16_t onTransive(uint16_t rxBytes)
    {
        switch(m_state)
        {
            case TransiveState::crc: return crc(rxBytes);
            case TransiveState::command: return command(rxBytes);
            case TransiveState::handshake: return handshake(rxBytes);
            default: return 0x00;
        };
    }

    uint16_t handshake(uint16_t rx_bytes)
    {
        if (rx_bytes == LINK_MASTER_HANDSHAKE) 
        {
            m_state = TransiveState::crc;
        }
        return LINK_SLAVE_HANDSHAKE;
    }

    uint16_t crc(uint16_t rx_bytes)
    {
        m_state = TransiveState::command;
        m_crcSendFinished = true;
        return rx_bytes;
    }

    uint16_t command(uint16_t rxBytes)
    {
        m_receivedCommand[m_commandIndex] = rxBytes;
        m_commandIndex++;
        if (m_commandIndex == 8)
        {
            m_commandIndex = 0;
            m_state = TransiveState::crc;
            m_commandSendFinished = true;
        }

        return (m_handler.transive != nullptr) ? m_handler.transive() : 0x00;
    }

    void onTransiveDone()
    {
        if (m_crcSendFinished)
        {
            m_crcSendFinished = false;
            k_timer_start(&m_timeoutTimer, K_MSEC(14), K_NO_WAIT);
        }
        if (!m_commandSendFinished) return;
        m_commandSendFinished = false;
        if (m_handler.transiveDone != nullptr)
        {
            if (m_handler.transiveDone() == CommandState::done)
            {
                m_idle = true;
                m_handler = emptyCommand();
                gpio_pin_toggle(DEVICE_DT_GET(DT_NODELABEL(gpioa)), 1);
                gpio_pin_toggle(DEVICE_DT_GET(DT_NODELABEL(gpioa)), 1);
            } 
        }
        k_timer_stop(&m_timeoutTimer);
        k_sem_give(&m_commandRxCompleteSemaphore);
    }

private:
    bool m_idle = true;
    bool m_commandSendFinished = false;
    bool m_crcSendFinished = false;

    struct k_sem m_commandRxCompleteSemaphore;

    int m_commandIndex = 0;
    std::array<uint16_t, 8> m_receivedCommand = {};

    uint8_t m_transiveCounter = 0;
    TransiveStruct m_handler = emptyCommand();
    TransiveState m_state = TransiveState::handshake;

    struct k_timer m_timeoutTimer;

    static uint16_t transiveCallback(uint16_t rxBytes, void* userData)
    {
        PacketLayer* self = static_cast<PacketLayer*>(userData);
        return self->onTransive(rxBytes);
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