#include "usbLinkCommand.hpp"
#include "../link_defines.h"

static uint8_t g_index = 0;

uint16_t packet[8] = {};
bool g_packetAvailable = false;
K_MSGQ_DEFINE(g_packetQueue, 16, 20, 1);

void usbLink_receiveHandler(std::span<const uint8_t> data, void*)
{
    if (data.size() > 16) return;
    k_msgq_put(&g_packetQueue, data.data(), K_NO_WAIT);
}

void usbLink_loadTransivePacket()
{
    if (k_msgq_num_used_get(&g_packetQueue) >= 1)
    {
        g_packetAvailable = (k_msgq_get(&g_packetQueue, packet, K_NO_WAIT) == 0);
    }
}

uint16_t usbLinkTransive()
{
    if (!g_packetAvailable) return 0x00;
    uint16_t ret = packet[g_index];
    g_index++;
    if (g_index == 8)
    {
        g_packetAvailable = false;
        g_index = 0;
    }
    return ret;
}

TransiveStruct usbLinkCommand()
{
    static TransiveStruct transive
    {
        .init = nullptr,
        .transive = usbLinkTransive,
        .transiveDone = [](){ return CommandState::resume; }
    };

    return transive;
}