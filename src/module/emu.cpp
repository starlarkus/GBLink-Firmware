#include "emu.hpp"

void EmuModule::execute()
{
    // m_cancel = false;
    // NextSection nextSection = NextSection::setup;
    // while (true)
    // {
    //     switch (nextSection)
    //     {
    //         case NextSection::setup:
    //         {
    //             TradeSetup tradeSetup(m_packetLayer, LINKTYPE_TRADE_SETUP, m_cancel);
    //             nextSection = tradeSetup.process(); // -> exit / -> connection / -> cancel
    //             break;
    //         }
    //         case NextSection::connection:
    //         {
    //             TradeConnection tradeConnection(m_packetLayer, m_cancel);
    //             nextSection = tradeConnection.process(); // -> tradeLounge? / -> disconnect / -> cancel
    //             break;
    //         }
    //         case NextSection::disconnect:
    //         {
    //             TradeDisconnect tradeDisconnection(m_packetLayer, m_cancel);
    //             nextSection = tradeDisconnection.process(); // -> connection / -> cancel
    //             break;
    //         }
    //         case NextSection::lounge:
    //         {
    //             TradeLounge tradeLounge(m_packetLayer, m_cancel);
    //             nextSection = tradeLounge.process(); // -> exit / -> connection / -> cancel
    //             break;
    //         }
    //         case NextSection::cancel: [[fallthrough]];
    //         case NextSection::exit:
    //         {
    //             m_cancel = false;
    //             //NVIC_EnableIRQ(USB_IRQn);
    //             return;
    //         }
    //     }
    // }
}