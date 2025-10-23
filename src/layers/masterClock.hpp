
#pragma once

extern "C"
{
    #include "linkLayer.h"
}

#include <zephyr/drivers/counter.h>

class MasterClock
{
public:

    MasterClock()
    {
        counter_set_top_value(m_transmissionCounter, &m_transmissionConfig);
    }

    void enableSync()
    {
        m_syncEnabled = true;
        counter_set_top_value(m_syncCounter, &m_periodicConfig);
        counter_start(m_syncCounter);
    }

    void disableSync()
    {
        m_syncEnabled = false;
        m_transmissionEnabled = false;
        m_transmissionCount = 0;
        counter_stop(m_syncCounter);
        counter_stop(m_transmissionCounter);
    }

    void startTransmissionSync()
    {
        m_transmissionCount = 0;
        m_transmissionEnabled = true;
    }

private:

    //-////////////////////////////////////////////////////////////////////////////////////////////////////////-//

    void onPeriodicCounter()
    {
        if (!m_syncEnabled) return;

        link_startTransive();

        if (m_transmissionEnabled)
        {
            counter_start(m_transmissionCounter);
            m_transmissionCount = 0;
        }

    }

    void onTransmissionCounter()
    {
        if (m_transmissionCount >= 8){
            counter_stop(m_transmissionCounter);
            return;
        } 
        link_startTransive();
        m_transmissionCount++;
    }

    //-////////////////////////////////////////////////////////////////////////////////////////////////////////-//

    struct counter_top_cfg m_periodicConfig = 
    {
        .ticks = 50100,
        .callback = periodicCounterInterrupt,
        .user_data = this,
        .flags = 0
    };

    struct counter_top_cfg m_transmissionConfig = 
    {
        .ticks = 58000,
        .callback = transmissionCounterInterrupt,
        .user_data = this,
        .flags = 0
    };

    const struct device* m_syncCounter = DEVICE_DT_GET(DT_CHILD(DT_NODELABEL(timers16), counter));
    const struct device* m_transmissionCounter = DEVICE_DT_GET(DT_CHILD(DT_NODELABEL(timers17), counter));

    bool m_syncEnabled = false;
    bool m_transmissionEnabled = false;
    uint8_t m_transmissionCount = 0;

    //-////////////////////////////////////////////////////////////////////////////////////////////////////////-//

    static void periodicCounterInterrupt(const struct device *counterDev, void *userData)
    {
        MasterClock* self = static_cast<MasterClock*>(userData);
        self->onPeriodicCounter();
    }

    static void transmissionCounterInterrupt(const struct device *counterDev, void *userData)
    {
        MasterClock* self = static_cast<MasterClock*>(userData);
        self->onTransmissionCounter();
    }
};