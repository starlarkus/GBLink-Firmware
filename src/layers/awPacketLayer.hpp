
#include <cstdint>
#include <array>
extern "C"
{
    #include "linkLayer.h"
}

#include <zephyr/kernel.h>

#pragma once

// Forward-declare the message queues (defined in awSection.cpp)
extern struct k_msgq g_awTxQueue;  // USB → GBA (partner data for block transfer)
extern struct k_msgq g_awRxQueue;  // GBA → USB (local data for block transfer)

class AwPacketLayer
{
public:
    enum class Role
    {
        slave,   // Firmware is PIO slave → GBA becomes parent (slot 0), firmware = slot 1
        master   // Firmware is PIO master → GBA becomes child (slot 1), firmware = slot 0
    };

    enum class Phase
    {
        connectionLoop,
        blockTransfer
    };

    AwPacketLayer(Role role);
    ~AwPacketLayer();

    Phase getPhase() const { return m_phase; }

    // Block until the phase transitions to BlockTransfer.
    // Returns false if cancelled.
    bool awaitBlockTransfer();

    // Block until block transfer completes and we return to connection loop.
    // Returns false if cancelled.
    bool awaitConnectionLoop();

    void cancel();

private:

    // AW protocol state tracking
    uint8_t m_awState = 0;         // 0x00, 0x01, 0x02, 0x04...0xC2, 0xD0, 0xD1, 0xE0-0xE9
    Phase m_phase = Phase::connectionLoop;
    Role m_role;
    uint8_t m_slotMask;            // 0x02 if slave (slot 1), 0x01 if master (slot 0)
    uint8_t m_retryCounter = 0x0F; // Matches AW's +0x4A init value
    uint8_t m_accumulator = 0;     // Matches AW's +0x49 — tracks which slots have responded
    uint16_t m_storedPeerValue = 0; // The value received from partner during state 0x01

    // Semaphores for phase transition signaling
    struct k_sem m_blockTransferSem;
    struct k_sem m_connectionLoopSem;

    // MASTER mode timing (PIO cycles, ~540ns each)
    // ~30000 cycles ≈ 16.2ms ≈ 1 VBlank frame at 60fps
    static constexpr uint32_t timingConnectionLoop = 30000;
    // Longer timing during block transfer to allow USB data to arrive
    static constexpr uint32_t timingBlockTransfer = 15370;

    //-////////////////////////////////////////////////////////////////////////////////////////////////////////-//
    // Transmit: called from PIO ISR to get next value to send to GBA
    //-////////////////////////////////////////////////////////////////////////////////////////////////////////-//

    uint16_t getConnectionLoopResponse();
    uint16_t getBlockTransferResponse();

    //-////////////////////////////////////////////////////////////////////////////////////////////////////////-//
    // Receive: called from PIO ISR with value received from GBA
    //-////////////////////////////////////////////////////////////////////////////////////////////////////////-//

    void handleConnectionLoopReceive(uint16_t rx);
    void handleBlockTransferReceive(uint16_t rx);

    //-////////////////////////////////////////////////////////////////////////////////////////////////////////-//
    // PIO Callbacks (static trampolines)
    //-////////////////////////////////////////////////////////////////////////////////////////////////////////-//

    static struct NextTransmit transmitCallback(void* userData)
    {
        AwPacketLayer* self = static_cast<AwPacketLayer*>(userData);

        uint16_t value;
        uint32_t timing;

        if (self->m_phase == Phase::connectionLoop)
        {
            value = self->getConnectionLoopResponse();
            timing = timingConnectionLoop;
        }
        else
        {
            value = self->getBlockTransferResponse();
            timing = timingBlockTransfer;
        }

        return { value, timing };
    }

    static void receiveCallback(uint16_t rx, void* userData)
    {
        AwPacketLayer* self = static_cast<AwPacketLayer*>(userData);

        if (self->m_phase == Phase::connectionLoop)
        {
            self->handleConnectionLoopReceive(rx);
        }
        else
        {
            self->handleBlockTransferReceive(rx);
        }
    }

    static void transiveDoneCallback(uint16_t rx, uint16_t tx, void* userData)
    {
        (void)rx;
        (void)tx;
        (void)userData;
    }
};
