#pragma once

#include <cstdint>

#include "PY32IOExpander.hpp"
#include "Si12T.h"

class HardwareHal
{
public:
    explicit HardwareHal(const char* i2c_device = "/dev/i2c-2");
    ~HardwareHal();

    HardwareHal(const HardwareHal&) = delete;
    HardwareHal& operator=(const HardwareHal&) = delete;

    bool initialize();
    bool readTouchIntensities(uint8_t intensities[3]);

    void setServoPowerEnabled(bool enabled);
    void setRgbLeftLed(uint8_t index, uint8_t r, uint8_t g, uint8_t b);
    void setRgbRightLed(uint8_t index, uint8_t r, uint8_t g, uint8_t b);
    void setRgbBothLed(uint8_t index, uint8_t r, uint8_t g, uint8_t b);

    enum class HeadPetGesture { None, Press, Release, SwipeForward, SwipeBackward };

private:
    bool enableBusPower();
    bool initializeExpander();
    void writeRgbColor(uint8_t r, uint8_t g, uint8_t b);

    m5::PY32IOExpander io_expander_;
    si12t_handle_t si12t_;
};