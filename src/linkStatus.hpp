#include <zephyr/kernel.h>
#include "./layers/usbLayer.hpp"

#pragma once

enum class LinkStatus : uint16_t
{
    GameboyConnected = 0xFF00,
    GameboyDisconnected = 0xFF01,

    AwaitMode = 0xFF02,
    HandshakeReceived = 0xFF03,
    HandshakeFinished = 0xFF04,

    LinkConnected = 0xFF05,
    LinkReconnecting = 0xFF06,
    LinkClosed = 0xFF07,

    DeviceReady = 0xFF08,
    EmuTradeSessionFinished = 0xFF09,

    StatusDebug = 0xFFFF
};

inline bool sendLinkStatus(LinkStatus status)
{
    return UsbLayer::getInstance().sendStatus(std::span<const uint8_t, 2>(reinterpret_cast<const uint8_t*>(&status), 2));
}