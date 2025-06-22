#include <span>

#include "../layers/packetLayer.hpp"
#include "../payloads/pokemon.hpp"
#include "../payloads/mail.hpp"
#include "../payloads/linkPlayer.hpp"

#pragma once



class TradeConnection
{
    struct CommandEntry
    {
        using SetupCallback = void(*)(void);

        TransiveStruct transive;
        SetupCallback setupCb;
    };

public:
    TradeConnection(PacketLayer& layer) : m_packetLayer(layer)
    {}

    void process();

private:

    void handleInitialDataExchange();

    void handleTradeNegotiations();
    
    enum class TradeConnectionState : uint8_t
    {
        LinkPlayer = 0x00,
        PartyPart0 = 0x01,
        PartyPart1 = 0x02,
        PartyPart2 = 0x03,
        Mail = 0x04,
        Ribbons = 0x05,
        LinkCMD = 0x06
    };

    TradeConnectionState m_blockState = TradeConnectionState::LinkPlayer;
    PacketLayer& m_packetLayer;
    struct k_sem m_commandSemaphore;
    std::array<uint16_t, 8> m_currentCommand;

    // size_t m_commandIndex = 0;
    // std::array<CommandEntry, 7> commandSequence =
    // {{
    //     {
    //         .transive = sendLinkTypeCommand(LINKTYPE_TRADE_CONNECTING),
    //         .setupCb = nullptr
    //     },
    //     {
    //         .transive = blockCommand(),
    //         .setupCb = []() 
    //         {
    //             const struct LinkPlayerBlock* linkPlayerBlock = linkPLayer(LINKTYPE_TRADE_CONNECTING);
    //             blockCommandSetup(linkPlayerBlock, sizeof(*linkPlayerBlock), sizeof(*linkPlayerBlock));
    //         }
    //     },
    //     {
    //         .transive = blockCommand(),
    //         .setupCb = []() 
    //         {
    //             const auto party = std::as_bytes(getParty().subspan<0, 2>());
    //             blockCommandSetup(party.data(), party.size(), 200);
    //         }
    //     },
    //     {
    //         .transive = blockCommand(),
    //         .setupCb = []() 
    //         {
    //             const auto party = std::as_bytes(getParty().subspan<2, 2>());
    //             blockCommandSetup(party.data(), party.size(), 200);
    //         }
    //     },
    //     {
    //         .transive = blockCommand(),
    //         .setupCb = []() 
    //         {
    //             const auto party = std::as_bytes(getParty().subspan<4, 2>());
    //             blockCommandSetup(party.data(), party.size(), 200);
    //         }
    //     },
    //     {
    //         .transive = blockCommand(),
    //         .setupCb = []() 
    //         {
    //             const auto mail = getEmptyMailPayload();
    //             blockCommandSetup(mail.data(), mail.size(), 220);
    //         }
    //     },
    //     {
    //         .transive = blockCommand(),
    //         .setupCb = []() 
    //         {
    //             blockCommandSetup(nullptr, 0, 40); 
    //         }
    //     }
    // }};

    size_t m_emptyCounter = 0;
};