#include "BitBangUart.h"
#include "Scs0009Servo.h"

#include <cstdlib>
#include <iostream>
#include <string>

namespace {

void printUsage(const char *program)
{
    std::cerr
        << "Usage:\n"
        << "  " << program << " ping <id>\n"
        << "  " << program << " read-pos <id>\n"
        << "  " << program << " write-pos <id> <position> <time> <speed>\n\n"
        << "Default GPIO mapping for your board layout:\n"
        << "  TX: gpiochip1 line 10  (PB10)\n"
        << "  RX: gpiochip0 line 6   (PA6)\n"
        << "  Baud: 38400\n";
}

bool parseInt(const char *text, int *value)
{
    if (!text || !value)
        return false;

    char *end = nullptr;
    const long parsed = std::strtol(text, &end, 0);
    if (!end || *end != '\0')
        return false;

    *value = static_cast<int>(parsed);
    return true;
}

}

int main(int argc, char *argv[])
{
    if (argc < 3) {
        printUsage(argv[0]);
        return 1;
    }

    BitBangUart uart("gpiochip1", 10, "gpiochip0", 6, 38400);
    std::string errorMessage;
    if (!uart.open(&errorMessage)) {
        std::cerr << "open uart failed: " << errorMessage << '\n';
        return 1;
    }

    if (!uart.elevateProcess(&errorMessage)) {
        std::cerr << "real-time setup failed: " << errorMessage << '\n';
        return 1;
    }

    Scs0009Servo servo(&uart);

    int id = 0;
    if (!parseInt(argv[2], &id) || id < 0 || id > 253) {
        std::cerr << "invalid servo id\n";
        return 1;
    }

    const std::string command = argv[1];
    if (command == "ping") {
        if (!servo.ping(static_cast<uint8_t>(id), &errorMessage)) {
            std::cerr << "ping failed: " << errorMessage << '\n';
            return 1;
        }
        std::cout << "servo " << id << " responded\n";
        return 0;
    }

    if (command == "read-pos") {
        uint16_t position = 0;
        if (!servo.readPosition(static_cast<uint8_t>(id), &position, &errorMessage)) {
            std::cerr << "read position failed: " << errorMessage << '\n';
            return 1;
        }
        std::cout << position << '\n';
        return 0;
    }

    if (command == "write-pos") {
        if (argc != 6) {
            printUsage(argv[0]);
            return 1;
        }

        int position = 0;
        int time = 0;
        int speed = 0;
        if (!parseInt(argv[3], &position) || !parseInt(argv[4], &time) || !parseInt(argv[5], &speed)) {
            std::cerr << "invalid write-pos arguments\n";
            return 1;
        }

        if (!servo.writePosition(static_cast<uint8_t>(id),
                                 static_cast<uint16_t>(position),
                                 static_cast<uint16_t>(time),
                                 static_cast<uint16_t>(speed),
                                 &errorMessage)) {
            std::cerr << "write position failed: " << errorMessage << '\n';
            return 1;
        }
        std::cout << "ok\n";
        return 0;
    }

    printUsage(argv[0]);
    return 1;
}