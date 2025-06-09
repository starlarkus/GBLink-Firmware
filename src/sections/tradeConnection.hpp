#include <span>

#include "../layers/packetLayer.hpp"

#pragma once

class TradeConnection
{

public:
    TradeConnection(PacketLayer& layer) : m_packetLayer(layer)
    {}

    void process();

private:

    enum class TradeConnectionState : uint8_t
    {
        LinkPlayer = 0x00,
        PartyPart0 = 0x01,
        PartyPart1 = 0x02,
        PartyPart2 = 0x03,
        Mail = 0x04,
        Ribbons = 0x05
    };

    TradeConnectionState m_blockState = TradeConnectionState::LinkPlayer;
    PacketLayer& m_packetLayer;
    struct k_sem m_commandSemaphore;
    std::array<uint16_t, 8> m_currentCommand;
};