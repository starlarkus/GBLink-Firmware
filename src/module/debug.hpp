#include <zephyr/kernel.h>
#include "../layers/packetLayer.hpp"


class DebugModule
{
public:
    DebugModule(PacketLayer& packetLayer) : m_packetLayer(packetLayer)
    {
        m_packetLayer.disableHandshake();
    }
    
    void execute();

    void receiveCommand(std::span<const uint8_t> command) {}

    bool canHandle(uint8_t command) { return (command & 0xF0) == 0x30; }

    void cancel() 
    { 
        m_cancel = true;
        m_packetLayer.cancel();
    }

private:
    bool m_cancel = false;
    PacketLayer& m_packetLayer;
};