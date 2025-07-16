#include "../layers/usbLayer.hpp"
#include "../layers/packetLayer.hpp"


class UsbLinkBridge
{
    enum class UsbCommand : uint8_t
    {
        SetMaster = 0x00,
        SetSlave = 0x01
    };

public:
    UsbLinkBridge(PacketLayer& packetLayer, UsbLayer& usbLayer) : 
        m_packetLayer(packetLayer),
        m_usbLayer(usbLayer)
    {
        usbLayer.setReceiveCommandHandler (
            [](std::span<const uint8_t> command, void* userData) { static_cast<UsbLinkBridge*>(userData)->receiveCommand(command); },
            this
        );
    }

    void process();

private:

    void receiveCommand(std::span<const uint8_t> command);

    PacketLayer& m_packetLayer;
    UsbLayer& m_usbLayer;
};