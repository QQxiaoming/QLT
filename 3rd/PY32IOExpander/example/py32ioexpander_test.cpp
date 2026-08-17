#include "PY32IOExpander.hpp"

#include <cstdio>
#include <cstdlib>
#include <unistd.h>

void showRgbColor(m5::PY32IOExpander &io_expander, uint8_t r, uint8_t g, uint8_t b)
{
    for (int i = 0; i < 12; i++) {
        io_expander.setLedColor(i, r, g, b);
    }
    io_expander.refreshLeds();
}

void setServoPowerEnabled(m5::PY32IOExpander &io_expander, bool enabled)
{
    io_expander.digitalWrite(0, enabled ? true : false);
}

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
        // VM EN
        io_expander.setDirection(0, true);  // Output
        io_expander.setPullMode(0, true);   // Pull-up
        setServoPowerEnabled(io_expander, true);
        usleep(200*1000);

        // RGB
        io_expander.setDirection(13, true);   // Output
        io_expander.setPullMode(13, true);    // Pull-up
        io_expander.setDriveMode(13, false);  // Push-pull
        io_expander.setLedCount(12);
        usleep(200*1000);
        showRgbColor(io_expander, 0, 0, 0);
        usleep(50*1000);
        showRgbColor(io_expander, 0, 0, 0);
        usleep(100*1000);

        int r = std::atoi(argv[2]);
        int g = std::atoi(argv[3]);
        int b = std::atoi(argv[4]);
        io_expander.setLedColor(0, r, g, b);
        io_expander.refreshLeds();
        io_expander.setLedColor(11, r, g, b);
        io_expander.refreshLeds();
    }

    return 0;
}