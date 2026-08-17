// SPDX-FileCopyrightText: 2026 karaage0703
// SPDX-License-Identifier: MIT

#include "Si12T.h"

#include <chrono>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>

namespace {
const char* gestureName(Si12T::Gesture gesture) {
    switch (gesture) {
        case Si12T::Gesture::Press: return "Press";
        case Si12T::Gesture::Release: return "Release";
        case Si12T::Gesture::SwipeForward: return "SwipeForward";
        case Si12T::Gesture::SwipeBackward: return "SwipeBackward";
        case Si12T::Gesture::None: return "None";
    }
    return "Unknown";
}

uint8_t parseAddress(const char* text) {
    char* end = nullptr;
    const unsigned long value = std::strtoul(text, &end, 0);
    if (*text == '\0' || *end != '\0' || value > 0x7f) {
        throw std::invalid_argument("I2C address must be a 7-bit value, e.g. 0x68");
    }
    return static_cast<uint8_t>(value);
}
}  // namespace

int main(int argc, char* argv[]) {
    const char* device = argc > 1 ? argv[1] : "/dev/i2c-1";
    const uint8_t address = argc > 2 ? parseAddress(argv[2]) : SI12T_GND_ADDRESS;

    try {
        Si12T touch(device, address);
        if (!touch.begin()) {
            std::cerr << "Si12T initialization failed: device=" << device
                      << ", address=0x" << std::hex << static_cast<int>(address)
                      << std::dec << std::endl;
            return EXIT_FAILURE;
        }

        std::cout << "Si12T ready on " << device << ", address=0x"
                  << std::hex << static_cast<int>(address) << std::dec
                  << ". Press Ctrl-C to stop.\n";

        for (;;) {
            const Si12T::Gesture gesture = touch.poll();
            uint8_t intensity[3] = {0, 0, 0};
            touch.getIntensity(intensity);

            std::cout << "intensity=[" << static_cast<int>(intensity[0]) << ", "
                      << static_cast<int>(intensity[1]) << ", "
                      << static_cast<int>(intensity[2]) << "] position="
                      << std::setw(4) << touch.getPosition() << " gesture="
                      << gestureName(gesture) << '\n';
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    } catch (const std::exception& error) {
        std::cerr << "Usage: " << argv[0]
                  << " [i2c-device] [7-bit-address]\nError: "
                  << error.what() << std::endl;
        return EXIT_FAILURE;
    }
}
