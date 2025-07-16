
#include "usbLinkBridge.hpp"

void UsbLinkBridge::process()
{
    while (true)
    {
        auto command = m_packetLayer.getCommand();

        if (command[0] == LINKCMD_READY_CLOSE_LINK)
        {
            m_packetLayer.setTransiveHandler(readyCloseLinkCommand());
            return;
        }
        
        // if 
        // m_packetLayer.setTransiveHandler(readyCloseLinkCommand());
                
    }
}

void UsbLinkBridge::receiveCommand(std::span<const uint8_t> command)
{
    switch(static_cast<UsbCommand>(command[0]))
    {
        case UsbCommand::SetMaster:
            m_packetLayer.setMode(PacketLayer::Mode::master);
            break;
        
        case UsbCommand::SetSlave:
            m_packetLayer.setMode(PacketLayer::Mode::slave);
            break;

        default:
            break;
    }
}