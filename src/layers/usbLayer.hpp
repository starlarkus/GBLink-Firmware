
#include "zephyr/drivers/usb/usb_dc.h"
#include "zephyr/sys/reboot.h"
#include <zephyr/sys/byteorder.h>
#include <zephyr/usb/usb_device.h>
#include <usb_descriptor.h>
#include <span>
#include <array>
#include <algorithm>
#include <zephyr/kernel.h>

#pragma once

class UsbLayer
{
    static constexpr uint8_t commandOutEndpoint = 1;
    static constexpr uint8_t statusInEndpoint = 129;
    static constexpr uint8_t dataOutEndpoint = 2;
    static constexpr uint8_t dataInEndpoint = 130;

    static constexpr uint16_t m_endpointSize = 64;


public:
    using UsbReceiveHandler = void(*)(std::span<const uint8_t>, void*);

    static UsbLayer& getInstance() 
    {
        static UsbLayer instance = UsbLayer();
        return instance;
    }

    UsbLayer(const UsbLayer&) = delete;
    UsbLayer& operator=(const UsbLayer&) = delete;
    UsbLayer(UsbLayer&&) = delete;
    UsbLayer& operator=(UsbLayer&&) = delete;

    bool sendStatus(std::span<const uint8_t> data)
    {
        if (!m_endpointsEnabled) return false;
        if (!k_sem_take(&m_waitForFreeEndpoint, K_MSEC(200))) return false;
        std::ranges::copy(data, m_sendData.begin());
        return usb_transfer(statusInEndpoint, m_sendData.data(), data.size(), USB_TRANS_WRITE, m_usbWriteCallback, this) == 0;
    }

    bool sendStatus(std::initializer_list<uint8_t> data)
    {
        if (!m_endpointsEnabled) return false;
        if (!k_sem_take(&m_waitForFreeEndpoint, K_MSEC(200))) return false;
        std::ranges::copy(data, m_sendData.begin());
        return usb_transfer(statusInEndpoint, m_sendData.data(), data.size(), USB_TRANS_WRITE, m_usbWriteCallback, this) == 0;
    }

    bool sendData(std::span<const uint8_t> data) 
    {
        if (!m_endpointsEnabled) return false;
        std::ranges::copy(data, m_sendData.begin());
        return usb_transfer(dataInEndpoint, m_sendData.data(), data.size(), USB_TRANS_WRITE, m_usbWriteCallback, this) == 0;
    }

    void setReceiveCommandHandler(const UsbReceiveHandler& handler, void* userData)
    {
        m_receiveCommandHandler.userData = userData;
        m_receiveCommandHandler.handler = handler;
    }

    void setReceiveDataHandler(const UsbReceiveHandler& handler, void* userData)
    {
        m_receiveDataCommandHandler.handler = handler;
        m_receiveDataCommandHandler.userData = userData;
    }

    static constexpr uint16_t endpointSize() { return m_endpointSize; }

    void setStatus(enum usb_dc_status_code status)
    {
        switch (status) 
        {
        case USB_DC_CONFIGURED:
            m_endpointsEnabled = true;
            break;
        case USB_DC_ERROR: 
            [[fallthrough]];
        case USB_DC_DISCONNECTED: 
            [[fallthrough]];
        case USB_DC_RESET: 
            [[fallthrough]];
        case USB_DC_UNKNOWN:
            m_endpointsEnabled = false;
            break;

        case USB_DC_CONNECTED:
        case USB_DC_SUSPEND:
        case USB_DC_RESUME:
            break;
        case USB_DC_CLEAR_HALT:
            sys_reboot(SYS_REBOOT_WARM);
        default:
            break;
        }
    }

private:
    bool m_endpointsEnabled = false;
    struct k_sem m_waitForFreeEndpoint;

    struct ReceiveDelegate
    {
        void* userData;
        UsbReceiveHandler handler;
        uint8_t endpoint;
        UsbLayer* layer;
        std::array<uint8_t, m_endpointSize> endpointBuffer = {};

        void operator()(std::span<const uint8_t> data)
        {
            if (handler != nullptr)
            {
                handler(data, userData);
            }
        }
    };

    int perpareNextReceive(ReceiveDelegate& delegate)
    {
        return usb_transfer(delegate.endpoint, delegate.endpointBuffer.data(), delegate.endpointBuffer.size(), USB_TRANS_READ, m_usbReadCallback, &delegate);
    }

    void receive(size_t size, ReceiveDelegate* delegate)
    {
        (*delegate)(std::span(delegate->endpointBuffer.begin(), size));
        perpareNextReceive(*delegate);
    }; 

    ReceiveDelegate m_receiveCommandHandler = {
        .userData = nullptr,
        .handler = nullptr,
        .endpoint = commandOutEndpoint,
        .layer = this
    };

    ReceiveDelegate m_receiveDataCommandHandler = {
        .userData = nullptr,
        .handler = nullptr,
        .endpoint = dataOutEndpoint,
        .layer = this
    };
    
    std::array<uint8_t, m_endpointSize> m_sendData = {};

    UsbLayer() 
    {
        perpareNextReceive(m_receiveCommandHandler);
        perpareNextReceive(m_receiveDataCommandHandler);
        k_sem_init(&m_waitForFreeEndpoint, 1, 1);
    }

    static void m_usbReadCallback(uint8_t ep, int size, void* userData)
    {
        ReceiveDelegate* self = static_cast<ReceiveDelegate*>(userData);
        self->layer->receive(size, self);
    }

    static void m_usbWriteCallback(uint8_t ep, int size, void* userData)
    {
        UsbLayer* self = static_cast<UsbLayer*>(userData);
        k_sem_give(&self->m_waitForFreeEndpoint);
    }
};
