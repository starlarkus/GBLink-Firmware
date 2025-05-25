#include "linkLayer.h"
#include "../callbacks/emptyCommand.h"

struct TransiveHandler
{
    using InitCallback = void(*)(std::span<const uint8_t>);
    using TransiveCallback = uint16_t(*)();
    using TraniveDoneCallback = bool(*)();

    std::span<const uint8_t> userdata;

    InitCallback init;
    TransiveCallback transive;
    TraniveDoneCallback transiveDone;
};

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
    }

    void setTransiveHandler(const struct TransiveHandler* handler)
    {
        if (handler == nullptr) 
        {
            m_handler = &m_defaultHandler;
            return;
        }

        m_handler = handler;
        if (handler->init != null) handler->init(handler->userData);
    }

    std::span<const uint8_t> getCommand { return std::span(m_receivedCommand); }

    void reset() { m_state = TransiveState::handshake; }

private:
    uint16_t onTransive(uint16_t rxBytes)
    {
        switch(m_state)
        {
            case TransiveState::crc: return crc(rxBytes);
            case TransiveState::command: return command(rxBytes);
            case TransiveState::handshake: return handshake(rxBytes);
        };
    }

    uint16_t handshake(uint16_t rx_bytes)
    {
        if (rx_bytes == LINK_MASTER_HANDSHAKE) 
        {
            g_state = TransiveState::crc;
        }
        return LINK_SLAVE_HANDSHAKE;
    }

    uint16_t crc(uint16_t rx_bytes)
    {
        g_state = TransiveState::command;
        return rx_bytes;
    }

    uint16_t command(uint16_t rxBytes)
    {
        m_receivedCommand[m_commandIndex] = rxBytes;
        m_commandIndex++;
        if (m_commandIndex == 8) m_commandIndex = 0;

        if (m_handler->transive != nullptr)  return m_handler->transive();
        g_state = TransiveState::crc
    }

    void onTransiveDone()
    {
        if (m_handler->transiveDone != nullptr)
        {
            bool continue = m_handler->transiveDone();
        } 
    }

private:

    std::array<uint16_t, 8> m_receivedCommand = {};

    TransiveHandler m_defaultHandler = 
    {
        .transive = emptyCommand_cb;
    };

    uint8_t m_transiveCounter = 0;
    TransiveHandler* m_handler = &m_defaultHandler;
    TransiveState m_state = TransiveState::handshake;

    static uint16_t transiveCallback(uint16_t rxBytes, void* userData)
    {
        PacketLayer* self = static_cast<PacketLayer*>(user_data);
        return self->onTransive(rx_bytes);
    }

    static void transiveDoneCallback(void* userData)
    {
        PacketLayer* self = static_cast<PacketLayer*>(user_data);
        self->onTransiveDone();
    }
};