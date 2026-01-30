#include "usbLinkCommand.hpp"
#include "../link_defines.h"
#include "zephyr/kernel.h"

static uint8_t g_index = 0;

static uint16_t g_packet[8] = {};
static bool g_packetAvailable = false;
K_MSGQ_DEFINE(g_packetQueue, 16, 200, 1);

void usbLink_receiveHandler(std::span<const uint8_t> data, void*)
{
    if (data.size() > 16) return;
    k_msgq_put(&g_packetQueue, data.data(), K_NO_WAIT);
}

static void loadTransivePacket()
{
    if (k_msgq_num_used_get(&g_packetQueue) <= 0) return;
    
    g_packetAvailable = (k_msgq_get(&g_packetQueue, g_packet, K_NO_WAIT) == 0);
}

static uint16_t usbLinkTransive()
{
    if (!g_packetAvailable) return 0x00;
    uint16_t ret = g_packet[g_index];

    // TODO Why does this happen? Only observed on Reconnect and is concistent, so no random flip
    if (g_index == 0 && g_packet[0] == 0xFF06) ret = 0x5FFF;

    
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
        .userData = std::span<const uint16_t>(),
        .init = [](std::span<const uint16_t>) {
            k_msgq_purge(&g_packetQueue);
            g_index = 0;
            g_packetAvailable = false;
        },
        .transive = usbLinkTransive,
        .transiveDone = []() 
        {
            loadTransivePacket();
            return CommandState::resume; 
        }
    };

    return transive;
}