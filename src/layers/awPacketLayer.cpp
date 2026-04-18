
#include "awPacketLayer.hpp"

AwPacketLayer::AwPacketLayer(Role role)
    : m_role(role)
{
    m_slotMask = (role == Role::slave) ? 0x02 : 0x01;

    k_sem_init(&m_blockTransferSem, 0, 1);
    k_sem_init(&m_connectionLoopSem, 0, 1);

    link_setTransmitCallback(&transmitCallback, this);
    link_setReceiveCallback(&receiveCallback, this);
    link_setTransiveDoneCallback(&transiveDoneCallback, this);
}

AwPacketLayer::~AwPacketLayer()
{
    link_setTransmitCallback(nullptr, nullptr);
    link_setReceiveCallback(nullptr, nullptr);
    link_setTransiveDoneCallback(nullptr, nullptr);
}

bool AwPacketLayer::awaitBlockTransfer()
{
    return k_sem_take(&m_blockTransferSem, K_FOREVER) == 0;
}

bool AwPacketLayer::awaitConnectionLoop()
{
    return k_sem_take(&m_connectionLoopSem, K_FOREVER) == 0;
}

void AwPacketLayer::cancel()
{
    k_sem_give(&m_blockTransferSem);
    k_sem_give(&m_connectionLoopSem);
}

//-////////////////////////////////////////////////////////////////////////////////////////////////////////-//
// Connection Loop — local responses (no USB relay needed)
//-////////////////////////////////////////////////////////////////////////////////////////////////////////-//

uint16_t AwPacketLayer::getConnectionLoopResponse()
{
    switch (m_awState)
    {
        case 0x00:
            // Peer detection: tell GBA we exist in our slot
            // GBA checks SIOMULTI[slot] != 0xFFFF
            return 0x6200 | m_slotMask;

        case 0x01:
        case 0x02:
            // ID exchange (state 0x01) and data validation (state 0x02):
            // Both require 0x72 marker with our slot bit.
            // State 0x02 compares against the value stored during state 0x01,
            // so we must send the SAME value in both states.
            return 0x7200 | m_slotMask;

        default:
        {
            // States 0x04-0xC2: sync pattern
            // "Others" handler validates high byte == (0x62 - state/2)
            // and low byte == (1 << slot)
            uint8_t highByte = 0x62 - (m_awState / 2);
            return (static_cast<uint16_t>(highByte) << 8) | m_slotMask;
        }
    }
}

//-////////////////////////////////////////////////////////////////////////////////////////////////////////-//
// Connection Loop — receive handler (tracks GBA's state)
//-////////////////////////////////////////////////////////////////////////////////////////////////////////-//

void AwPacketLayer::handleConnectionLoopReceive(uint16_t rx)
{
    uint8_t highByte = (rx >> 8) & 0xFF;

    switch (m_awState)
    {
        case 0x00:
        {
            // GBA sends 0x62XX during peer detection.
            // We need to wait for the retry counter to expire before advancing,
            // mimicking AW's +0x4A countdown from 0x0F to 0.
            if (m_retryCounter > 0)
            {
                m_retryCounter--;
                return;
            }

            // Retry counter expired — advance to state 0x01
            m_awState = 0x01;
            m_accumulator = 0;
            m_retryCounter = 0x0F;
            break;
        }

        case 0x01:
        {
            // GBA sends 0x72XX (its own identification).
            // We check that the GBA identified with its expected slot.
            // For a 2-player game: the GBA's slot mask is the "other" slot.
            // GBA is slot 0 when firmware is slave, slot 1 when firmware is master.
            uint8_t gbaSlotMask = (m_role == Role::slave) ? 0x01 : 0x02;

            if (highByte == 0x72)
            {
                uint8_t lowByte = rx & 0xFF;
                if (lowByte == gbaSlotMask)
                {
                    m_accumulator |= gbaSlotMask;
                    m_storedPeerValue = rx;
                }
            }

            // In 2-player, one valid 0x72XX from the GBA is sufficient to advance.
            if (m_accumulator != 0)
            {
                m_awState = 0x02;
            }
            break;
        }

        default:
        {
            // States 0x02-0xC2: advance state by 2 each exchange
            m_awState += 2;

            if (m_awState >= 0xC4)
            {
                // Wrap back to state 0x00 — start new connection loop cycle
                m_awState = 0x00;
                m_retryCounter = 0x0F;
                m_accumulator = 0;
            }

            // Detect transition to block transfer:
            // If the GBA sends 0x63XX, it has entered state 0xD0 (block transfer).
            // This happens when the game calls sub_08063454 to send actual data.
            if (highByte == 0x63)
            {
                m_awState = 0xD0;
                m_phase = Phase::blockTransfer;
                k_sem_give(&m_blockTransferSem);
            }
            break;
        }
    }
}

//-////////////////////////////////////////////////////////////////////////////////////////////////////////-//
// Block Transfer — relay via USB message queues
//-////////////////////////////////////////////////////////////////////////////////////////////////////////-//

uint16_t AwPacketLayer::getBlockTransferResponse()
{
    uint16_t value;

    // Try to get partner's data from USB TX queue
    if (k_msgq_get(&g_awTxQueue, &value, K_NO_WAIT) == 0)
    {
        return value;
    }

    // No partner data available yet — send our stored peer value
    // (the 0x72XX identification from state 0x01) as a safe fallback.
    // During block transfer states 0xD0/0xD1, the GBA validates received
    // values against this stored identification value.
    return m_storedPeerValue;
}

void AwPacketLayer::handleBlockTransferReceive(uint16_t rx)
{
    // Enqueue GBA's value for relay to partner via USB
    k_msgq_put(&g_awRxQueue, &rx, K_NO_WAIT);

    // Detect return to connection loop:
    // After the E0-E9 verification handshake completes (state 0xE9),
    // the main handler returns immediately. The game will eventually
    // reset state and re-enter the connection loop.
    //
    // We detect the end of block transfer when the GBA starts sending
    // connection loop values again (0x62XX high byte = peer detection).
    uint8_t highByte = (rx >> 8) & 0xFF;

    if (highByte == 0x62 && m_phase == Phase::blockTransfer)
    {
        m_awState = 0x00;
        m_retryCounter = 0x0F;
        m_accumulator = 0;
        m_phase = Phase::connectionLoop;
        k_sem_give(&m_connectionLoopSem);
    }
}
