#include <span>

#include "../layers/packetLayer.hpp"
#include "../layers/usbLayer.hpp"

#pragma once

class TradeSetup
{

    enum class BlockCommandState : uint8_t
    {
        LinkPlayer = 0x00,
        TrainerCard = 0x01,
        RequestTrainerCard = 0x02
    };

public:
    TradeSetup(PacketLayer& layer, uint16_t linkType) : m_packetLayer(layer), m_linkType(linkType)
    {
        m_packetLayer.setMode(PacketLayer::Mode::master);
    }

    ~TradeSetup()
    {
        while(!m_packetLayer.idle()) {};
    }

    void process();

private:

    BlockCommandState m_blockState = BlockCommandState::LinkPlayer;
    PacketLayer& m_packetLayer;
    struct k_sem m_commandSemaphore;
    uint16_t m_linkType;
    std::array<uint16_t, 8> m_currentCommand;

    size_t m_movementDataIndex = 0;
    std::array<uint16_t, 6> m_movementData = {LINK_KEY_CODE_DPAD_UP, LINK_KEY_CODE_DPAD_UP, LINK_KEY_CODE_DPAD_RIGHT, LINK_KEY_CODE_DPAD_UP, LINK_KEY_CODE_DPAD_LEFT, LINK_KEY_CODE_READY};
};