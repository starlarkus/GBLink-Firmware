#include <span>

#include "../layers/packetLayer.hpp"
#include "../layers/usbLayer.hpp"

#include "nextSectionState.hpp"

#pragma once

class TradeLounge
{

    enum class BlockCommandState : uint8_t
    {
        LinkPlayer = 0x00,
        TrainerCard = 0x01,
        RequestTrainerCard = 0x02
    };

public:
    TradeLounge(PacketLayer& layer, bool& cancel) : m_packetLayer(layer), m_cancel(cancel)
    {
        //m_packetLayer.setMode(PacketLayer::Mode::master);
    }

    ~TradeLounge()
    {
        while(!m_packetLayer.idle()) {};
    }

    NextSection process();

private:

    BlockCommandState m_blockState = BlockCommandState::LinkPlayer;
    PacketLayer& m_packetLayer;
    struct k_sem m_commandSemaphore;
    std::array<uint16_t, 8> m_currentCommand;

    bool& m_cancel;
    size_t m_movementDataIndex = 0;
    std::array<uint16_t, 6> m_movementData = {LINK_KEY_CODE_DPAD_UP, LINK_KEY_CODE_DPAD_UP, LINK_KEY_CODE_DPAD_RIGHT, LINK_KEY_CODE_DPAD_UP, LINK_KEY_CODE_DPAD_LEFT, LINK_KEY_CODE_READY};
};