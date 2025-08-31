#include <zephyr/kernel.h>
#include "../layers/packetLayer.hpp"

#include "../sections/sections.hpp"

class EmuModule
{
public:
    EmuModule(PacketLayer& packetLayer) : m_packetLayer(packetLayer)
    {
        m_packetLayer.disableHandshake();
    }
    
    void execute();

    void receiveCommand(std::span<const uint8_t> command) {}

    bool canHandle(uint8_t command) { return (command & 0xF0) == 0x20; }

    void cancel() 
    { 
        m_cancel = true;
        m_packetLayer.cancel();
    }

private:
    void connect();
    bool m_cancel = false;
    PacketLayer& m_packetLayer;
};