#include <span>

#include "../layers/packetLayer.hpp"
#include "../payloads/pokemon.hpp"
#include "../payloads/mail.hpp"

extern "C"
{
    #include "../payloads/linkPlayer.h"
}


#pragma once



class TradeConnection
{
    struct CommandEntry
    {
        using SetupCallback = void(*)(void);

        TransiveStruct transive;
        SetupCallback setupCb;
    };

public:
    TradeConnection(PacketLayer& layer) : m_packetLayer(layer)
    {
        m_packetLayer.setMode(PacketLayer::Mode::master);
        k_timer_init(&m_commandRequestTimer, requestBlockCommand, NULL);
        k_timer_user_data_set(&m_commandRequestTimer, this);
    }

    ~TradeConnection()
    {
        while(!m_packetLayer.idle()) {};
    }

    void process();

private:

    void handleInitialDataExchange();

    void handleTradeNegotiations();
    
    enum class TradeConnectionState : uint8_t
    {
        LinkPlayer = 0x00,
        PartyPart0 = 0x01,
        PartyPart1 = 0x02,
        PartyPart2 = 0x03,
        Mail = 0x04,
        Ribbons = 0x05,
        LinkCMD = 0x06
    };

    TradeConnectionState m_blockState = TradeConnectionState::LinkPlayer;
    PacketLayer& m_packetLayer;
    struct k_sem m_commandSemaphore;
    std::array<uint16_t, 8> m_currentCommand;

    struct k_timer m_commandRequestTimer;

    size_t m_emptyCounter = 0;
    bool m_requestBlock = false;

    static void requestBlockCommand(struct k_timer *timer)
    {
        void* userData = k_timer_user_data_get(timer);
        TradeConnection* self = static_cast<TradeConnection*>(userData);
        self->m_requestBlock = true;
    }
};