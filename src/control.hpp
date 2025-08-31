#include "./layers/usbLayer.hpp"
#include "./layers/packetLayer.hpp"
#include "./module/link.hpp"
#include "./module/emu.hpp"
#include "linkStatus.hpp"
#include "./callbacks/commands.hpp"

class Control
{
    enum class ControlCommand
    {
        SetMode = 0x00,
        Cancel = 0x01
    };

    enum class Mode 
    {
        tradeEmu = 0x00,
        onlineLink = 0x01
    };

    static constexpr uint8_t callSetModeId = 0x01;

public:
    Control(PacketLayer& packetLayer) : 
        m_packetLayer(packetLayer), 
        m_linkModule(m_packetLayer),
        m_emuModule(m_packetLayer)
    {
        UsbLayer::getInstance().setReceiveCommandHandler(receiveCommandHandler, this);

        k_sem_init(&m_waitForModeSemaphore, 0, 1);
        k_timer_init(&m_connectionTimer, &connectionCheck, nullptr);
        k_timer_user_data_set(&m_connectionTimer, this);
    }

    void executeMode()
    {
        //k_timer_start(&m_connectionTimer, K_MSEC(1000), K_MSEC(1000));
        k_sem_take(&m_waitForModeSemaphore, K_FOREVER);
        //k_timer_stop(&m_connectionTimer);
        m_packetLayer.reset();
        switch (m_mode)
        {
            case Mode::tradeEmu:
                party::partyInit();
                UsbLayer::getInstance().setReceiveDataHandler(party::usbReceivePkmFile, nullptr);
                m_emuModule.execute();
                break;
            case Mode::onlineLink:
                UsbLayer::getInstance().setReceiveDataHandler(usbLink_receiveHandler, nullptr);
                m_linkModule.execute();
                break;
        }
        m_mode = {};
    }

private:
    PacketLayer& m_packetLayer;
    struct k_sem m_waitForModeSemaphore;
    struct k_timer m_connectionTimer;
    Mode m_mode;

    LinkModule m_linkModule;
    EmuModule m_emuModule;

    bool canHandle(uint8_t command) { return (command & 0xF0) == 0x00; }

    void receiveCommand(std::span<const uint8_t> data)
    {
        if (m_linkModule.canHandle(data[0])) return m_linkModule.receiveCommand(data);
        if (m_emuModule.canHandle(data[0])) return m_emuModule.receiveCommand(data);
        if (!canHandle(data[0])) return;

        switch (static_cast<ControlCommand>(data[0]))
        {
            case ControlCommand::SetMode: return callSetMode(static_cast<Mode>(data[1]));
            case ControlCommand::Cancel: return callCancel();
            default: return;
        }
    }

    void isGameboyConnected()
    {
        const LinkStatus status = m_packetLayer.isGameboyConnected() ? LinkStatus::GameboyConnected : LinkStatus::GameboyDisconnected;
        sendLinkStatus(status);
    }

    //-////////////////////////////////////////////////////////////////////////////////////////////////////////-//
    // CALLS
    //-////////////////////////////////////////////////////////////////////////////////////////////////////////-//

    void callSetMode(Mode mode)
    {
        k_sem_give(&m_waitForModeSemaphore);
        m_mode = mode;
    }

    void callCancel()
    {
        switch (m_mode)
        {
            case Mode::tradeEmu:
                m_emuModule.cancel();
                break;
            case Mode::onlineLink:
                //m_linkModule.cancel();
                m_packetLayer.cancel();
                break;
        }
    }

    //-////////////////////////////////////////////////////////////////////////////////////////////////////////-//

    static void receiveCommandHandler(std::span<const uint8_t> receivedData, void* userData)
    {
        Control* self = static_cast<Control*>(userData);
        self->receiveCommand(receivedData);
    }

    static void connectionCheck(struct k_timer *timer)
    {
        void* userData = k_timer_user_data_get(timer);
        Control* self = static_cast<Control*>(userData);
        self->isGameboyConnected();
    }
};