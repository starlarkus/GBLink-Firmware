#pragma once

#include <cstdint>

class Hardware {
public:
    static Hardware& getInstance();

    Hardware(const Hardware&) = delete;
    Hardware& operator=(const Hardware&) = delete;
    Hardware(Hardware&&) = delete;
    Hardware& operator=(Hardware&&) = delete;

    void setVoltage3V3();
    void setVoltage5V();
    void setLED(uint8_t r, uint8_t g, uint8_t b, bool on);
    void rebootToBootloader();
    void reboot();

private:
    Hardware();
    void ws2812Init();
    void ws2812Set(uint32_t grb);
};
