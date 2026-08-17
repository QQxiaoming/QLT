/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#pragma once

#include <cstdint>
#include <string>

namespace m5 {

class PY32IOExpander {
public:
    static constexpr uint8_t DEFAULT_ADDRESS = 0x6F;

    explicit PY32IOExpander(const char* i2c_device = "/dev/i2c-1", uint8_t addr = DEFAULT_ADDRESS);
    ~PY32IOExpander();

    /**
     * @brief Initialize the device
     *
     * @return true if successful
     * @return false if failed
     */
    bool begin();

    // IOExpander methods
    // false input, true output
    void setDirection(uint8_t pin, bool direction);

    void enablePull(uint8_t pin, bool enablePull);

    // false down, true up
    void setPullMode(uint8_t pin, bool mode);

    // false push-pull, true open-drain
    void setDriveMode(uint8_t pin, bool openDrain);

    void setHighImpedance(uint8_t pin, bool enable);

    bool getWriteValue(uint8_t pin);

    void digitalWrite(uint8_t pin, bool level);

    bool digitalRead(uint8_t pin);

    void resetIrq();

    void disableIrq();

    void enableIrq();

    // Extended functionality
    uint16_t readDeviceUID();
    uint8_t readVersion();

    // ADC
    // channel: 1-4
    uint16_t analogRead(uint8_t channel);

    // PWM
    // channel: 0-3
    void setPwmDuty(uint8_t channel, uint8_t duty);
    void setPwmFrequency(uint16_t freq);

    // LED
    void setLedCount(uint8_t count);
    void setLedColor(uint8_t index, uint16_t color565);
    void setLedColor(uint8_t index, uint8_t r, uint8_t g, uint8_t b);
    void setLedColor(uint8_t index, uint32_t color);
    void setLedData(const uint8_t* data, size_t len);
    void refreshLeds();

private:
    std::string _i2c_device;
    int _i2c_fd;
    uint8_t _addr;
    bool _initialized;

    int writeRegister8(uint8_t reg, uint8_t value);
    uint8_t readRegister8(uint8_t reg);
    int writeRegister(uint8_t reg, const uint8_t* data, size_t len);
    int readRegister(uint8_t reg, uint8_t* data, size_t len);

    int bitOn(uint8_t reg, uint8_t mask);
    int bitOff(uint8_t reg, uint8_t mask);

    void _writeBit(uint8_t reg_l, uint8_t reg_h, uint8_t pin, bool value);
    bool _readBit(uint8_t reg_l, uint8_t reg_h, uint8_t pin);
};

}  // namespace m5
