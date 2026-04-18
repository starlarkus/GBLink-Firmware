
#include "awSection.hpp"

// TX queue: USB → GBA (filled by USB handler, drained by PIO ISR)
K_MSGQ_DEFINE(g_awTxQueue, 2, 256, 2);

// RX queue: GBA → USB (filled by PIO ISR, drained by process() loop)
K_MSGQ_DEFINE(g_awRxQueue, 2, 256, 2);

void awRelay_receiveHandler(std::span<const uint8_t> data, void*)
{
    // 64-byte USB packet = 32 x uint16_t values
    const uint16_t* words = reinterpret_cast<const uint16_t*>(data.data());
    for (size_t i = 0; i < 32 && i * 2 < data.size(); i++)
    {
        if (words[i] != 0x0000)
        {
            k_msgq_put(&g_awTxQueue, &words[i], K_NO_WAIT);
        }
    }
}

AwSection::AwSection()
{
    k_msgq_purge(&g_awTxQueue);
    k_msgq_purge(&g_awRxQueue);

    link_setTransmitCallback(&transmitCallback, this);
    link_setReceiveCallback(&receiveCallback, this);
    link_setTransiveDoneCallback(&transiveDoneCallback, this);
}

AwSection::~AwSection()
{
    link_setTransmitCallback(nullptr, nullptr);
    link_setReceiveCallback(nullptr, nullptr);
    link_setTransiveDoneCallback(nullptr, nullptr);
}

void AwSection::process()
{
    while (!m_cancel)
    {
        uint16_t value;

        // Drain RX queue into USB accumulation buffer
        while (k_msgq_get(&g_awRxQueue, &value, K_NO_WAIT) == 0)
        {
            // Suppress consecutive 0x7FFF idle frames — same approach as
            // Pokemon's CAFE 0x11 idle suppression in UsbSection.
            // Only suppress 0x7FFF (CMD_NONE), not other values, because:
            // - The game uses 0x7FFF as a state transition signal
            // - Packet payloads may have intentional duplicate values
            // - The partner firmware fills 0x7FFF locally anyway
            if (value == 0x7FFF)
            {
                m_idleStreak++;
                if (m_idleStreak > 1)
                    continue;
            }
            else
            {
                m_idleStreak = 0;
            }

            m_rxPacket[m_rxPacketIndex++] = value;

            if (m_rxPacketIndex == 32)
            {
                flushRxBuffer();
            }
        }

        // Flush partial buffer periodically to avoid stale data
        if (m_rxPacketIndex > 0)
        {
            flushRxBuffer();
        }

        k_sleep(K_MSEC(10));
    }

    // Flush any remaining data
    if (m_rxPacketIndex > 0)
    {
        flushRxBuffer();
    }
}

void AwSection::flushRxBuffer()
{
    // Skip all-empty packets (same as Pokemon's flush check)
    const bool emptyPacket = std::all_of(
        m_rxPacket.begin(), m_rxPacket.end(),
        [](uint16_t v) { return v == 0x0000; }
    );

    if (!emptyPacket)
    {
        UsbLayer::getInstance().sendData(
            std::span(reinterpret_cast<const uint8_t*>(m_rxPacket.data()), 64)
        );
    }

    std::memset(m_rxPacket.data(), 0x00, 64);
    m_rxPacketIndex = 0;
}

struct NextTransmit AwSection::transmitCallback(void* userData)
{
    AwSection* self = static_cast<AwSection*>(userData);

    // AW multiplayer uses 0x7FFF (CMD_NONE) as the idle/default value.
    // Don't cache/replay partner values — the game uses 0x7FFF as a
    // state transition signal between protocol phases.
    uint16_t value = 0x7FFF;
    k_msgq_get(&g_awTxQueue, &value, K_NO_WAIT);

    return { value, self->defaultTimingUs };
}

void AwSection::receiveCallback(uint16_t rx, void* userData)
{
    (void)userData;
    k_msgq_put(&g_awRxQueue, &rx, K_NO_WAIT);
}

void AwSection::transiveDoneCallback(uint16_t rx, uint16_t tx, void* userData)
{
    (void)rx;
    (void)tx;
    (void)userData;
}
