#include <zephyr/kernel.h>
#include "../sections//usbSection.hpp"

class LinkModule
{
private:
    enum class LinkModeCommand : uint8_t
    {
        SetModeMaster = 0x10,
        SetModeSlave = 0x11,
        StartHandshake = 0x12,
        ConnectLink = 0x13
    };

public:
    LinkModule()
    {
        k_sem_init(&m_waitForLinkModeCommand, 0, 1);
    }

    void execute();

    bool canHandle(uint8_t command) { return (command & 0xF0) == 0x10; }

    void receiveCommand(std::span<const uint8_t> command);

private:

    struct k_sem m_waitForLinkModeCommand;

    UsbSection* m_currentSection = nullptr;
    PacketLayer::Mode m_packetLayerMode = PacketLayer::Mode::slave;

    //-////////////////////////////////////////////////////////////////////////////////////////////////////////-//
    // CALLS
    //-////////////////////////////////////////////////////////////////////////////////////////////////////////-//
};

