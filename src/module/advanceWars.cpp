
#include "advanceWars.hpp"
#include "../linkStatus.hpp"

void AdvanceWarsModule::execute()
{
    m_cancel = false;

    // Step 1: Emit AwaitMode — server will assign master/slave
    sendLinkStatus(LinkStatus::AwaitMode);
    k_sem_take(&m_waitForLinkModeCommand, K_FOREVER);
    if (m_cancel) return;

    // Step 2: Mode stored in receiveCommand(), emit HandshakeReceived
    // Server waits for both clients to report HandshakeReceived, then sends StartHandshake
    sendLinkStatus(LinkStatus::HandshakeReceived);
    k_sem_take(&m_waitForStart, K_FOREVER);
    if (m_cancel) return;

    // Step 3: Create section, enable PIO in assigned mode, relay raw data.
    // AW multiplayer uses IRQ-based SIO with 0x7FFF as idle.
    // One adapter MASTER (GBA=child/P2), one SLAVE (GBA=parent/P1).
    {
        AwSection section;
        m_currentSection = &section;
        link_changeMode(m_linkMode);
        sendLinkStatus(LinkStatus::LinkConnected);
        section.process();
    }
    m_currentSection = nullptr;
}

void AdvanceWarsModule::receiveCommand(std::span<const uint8_t> command)
{
    switch (static_cast<LinkModeCommand>(command[0]))
    {
        case LinkModeCommand::SetModeMaster:
            m_linkMode = MASTER;
            k_sem_give(&m_waitForLinkModeCommand);
            break;

        case LinkModeCommand::SetModeSlave:
            m_linkMode = SLAVE;
            k_sem_give(&m_waitForLinkModeCommand);
            break;

        case LinkModeCommand::StartHandshake:
            k_sem_give(&m_waitForStart);
            break;

        case LinkModeCommand::ConnectLink:
            // Ignored in AW mode — connection loop handles this
            break;

        default: break;
    }
}
