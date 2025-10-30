#include <zephyr/kernel.h>

#include "control.hpp"

#include "./layers/packetLayer.hpp"

#include "link_defines.h"

int main(void)
{
    PacketLayer g_packetLayer = PacketLayer();
    g_packetLayer.setMode(PacketLayer::Mode::slave);
    Control g_control = Control(g_packetLayer);
    while (true)
    {
        g_control.executeMode();
    }

    return 0;
}