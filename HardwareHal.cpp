#include "HardwareHal.h"

#include <fstream>
#include <string>
#include <unistd.h>

namespace {

constexpr int kBusPowerGpio = 131;
constexpr uint8_t kServoPowerPin = 0;
constexpr uint8_t kRgbDataPin = 13;
constexpr uint8_t kRgbLedCount = 12;

bool writeSysfsValue(const char* path, const char* value)
{
    std::ofstream file(path);
    if (!file.is_open()) {
        return false;
    }

    file << value;
    return file.good();
}

}  // namespace

HardwareHal::HardwareHal(const char* i2c_device)
    : io_expander_(i2c_device), si12t_(nullptr)
{
}

HardwareHal::~HardwareHal()
{
    if (si12t_ != nullptr) {
        si12t_delete(si12t_);
    }
}

bool HardwareHal::initialize()
{
    return enableBusPower() && initializeExpander();
}

bool HardwareHal::enableBusPower()
{
    // Exporting an already exported GPIO fails with EBUSY, so the result is ignored.
    const std::string gpio_number = std::to_string(kBusPowerGpio);
    writeSysfsValue("/sys/class/gpio/export", gpio_number.c_str());

    return writeSysfsValue("/sys/class/gpio/PI3/direction", "out")
        && writeSysfsValue("/sys/class/gpio/PI3/value", "1");
}

bool HardwareHal::initializeExpander()
{
    if (!io_expander_.begin()) {
        return false;
    }

    io_expander_.setDirection(kServoPowerPin, true);
    io_expander_.setPullMode(kServoPowerPin, true);
    setServoPowerEnabled(true);
    usleep(200 * 1000);

    io_expander_.setDirection(kRgbDataPin, true);
    io_expander_.setPullMode(kRgbDataPin, true);
    io_expander_.setDriveMode(kRgbDataPin, false);
    io_expander_.setLedCount(kRgbLedCount);
    usleep(200 * 1000);

    writeRgbColor(0, 0, 0);
    usleep(50 * 1000);
    writeRgbColor(0, 0, 0);
    usleep(100 * 1000);

    si12t_config_t si12t_cfg = {"/dev/i2c-2", SI12T_GND_ADDRESS};
    if (si12t_init(&si12t_cfg, &si12t_) != 0) {
        si12t_ = nullptr;
        return false;
    }
    if (si12t_setup(si12t_, SI12T_TYPE_LOW, SI12T_SENSITIVITY_LEVEL_3) != 0) {
        si12t_delete(si12t_);
        si12t_ = nullptr;
        return false;
    }
    return true;
}

void HardwareHal::setServoPowerEnabled(bool enabled)
{
    io_expander_.digitalWrite(kServoPowerPin, enabled);
}

void HardwareHal::writeRgbColor(uint8_t r, uint8_t g, uint8_t b)
{
    for (uint8_t index = 0; index < kRgbLedCount; ++index) {
        io_expander_.setLedColor(index, r, g, b);
    }
    io_expander_.refreshLeds();
}

void HardwareHal::setRgbLeftLed(uint8_t index, uint8_t r, uint8_t g, uint8_t b)
{
    if(index >= 6) {
        return;
    }
    io_expander_.setLedColor(index, r, g, b);
    io_expander_.refreshLeds();
}

void HardwareHal::setRgbRightLed(uint8_t index, uint8_t r, uint8_t g, uint8_t b)
{
    if(index >= 6) {
        return;
    }
    io_expander_.setLedColor(11 - index, r, g, b);
    io_expander_.refreshLeds();
}

void HardwareHal::setRgbBothLed(uint8_t index, uint8_t r, uint8_t g, uint8_t b)
{
    setRgbLeftLed(index, r, g, b);
    setRgbRightLed(index, r, g, b);
}

bool HardwareHal::readTouchIntensities(uint8_t intensities[3])
{
    if (si12t_ == nullptr || intensities == nullptr) {
        return false;
    }

    uint8_t touch_result = 0;
    if (si12t_read_touch_result(si12t_, &touch_result) != 0) {
        return false;
    }

    si12t_parse_touch_result_to(touch_result, intensities);
    return true;
}

