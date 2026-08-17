#include "PY32IOExpander.hpp"

#include <cstdio>
#include <cstdlib>

int main(int argc, char *argv[])
{
    const char *i2c_device = argc > 1 ? argv[1] : "/dev/i2c-1";
    m5::PY32IOExpander io_expander(i2c_device);

    if (!io_expander.begin()) {
        std::fprintf(stderr, "Failed to initialize PY32IOExpander on %s\n", i2c_device);
        return 1;
    }

    std::printf("PY32IOExpander found on %s: UID=0x%04X, version=0x%02X\n",
                i2c_device,
                io_expander.readDeviceUID(),
                io_expander.readVersion());

    if (argc > 2) {
        char *end = nullptr;
        long pin_value = std::strtol(argv[2], &end, 10);
        if (*argv[2] == '\0' || *end != '\0' || pin_value < 0 || pin_value > 15) {
            std::fprintf(stderr, "GPIO pin must be in the range 0-15\n");
            return 2;
        }

        uint8_t pin = static_cast<uint8_t>(pin_value);
        io_expander.setDirection(pin, false);
        io_expander.enablePull(pin, true);
        std::printf("GPIO %u input: %u\n", pin, io_expander.digitalRead(pin) ? 1 : 0);
    }

    return 0;
}