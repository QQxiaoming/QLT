#include "BitBangUart.h"
#include "Scs0009Servo.h"

#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

namespace {

void printUsage(const char *program)
{
    std::cerr
        << "Usage:\n"
        << "  " << program << " ping <id>\n"
        << "  " << program << " read-pos <id>\n"
        << "  " << program << " write-pos <id> <position> <time> <speed>\n\n"
        << "  " << program << " monitor-rx [count]\n"
        << "  " << program << " loopback <byte-hex>\n\n"
        << "  " << program << " ping-trace <id> [bytes]\n\n"
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

void printByteHex(uint8_t value)
{
    std::cout << "0x"
              << std::hex << std::uppercase << std::setw(2) << std::setfill('0')
              << static_cast<int>(value)
              << std::dec << std::nouppercase << std::setfill(' ');
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

    const std::string command = argv[1];
    if (command == "monitor-rx") {
        int count = 20;
        if (argc >= 3 && (!parseInt(argv[2], &count) || count <= 0)) {
            std::cerr << "invalid edge count\n";
            return 1;
        }

        for (int index = 0; index < count; ++index) {
            bool fallingEdge = false;
            uint64_t edgeTimeNs = 0;
            if (!uart.waitForEdge(&fallingEdge, &edgeTimeNs, 5000000, &errorMessage)) {
                std::cerr << "monitor-rx failed: " << errorMessage << '\n';
                return 1;
            }

            std::cout << (fallingEdge ? "falling" : "rising")
                      << " @ " << edgeTimeNs << " ns\n";
        }
        return 0;
    }

    if (command == "loopback") {
        if (argc != 3) {
            printUsage(argv[0]);
            return 1;
        }

        int byteValue = 0;
        if (!parseInt(argv[2], &byteValue) || byteValue < 0 || byteValue > 255) {
            std::cerr << "invalid loopback byte\n";
            return 1;
        }

        if (!uart.writeByte(static_cast<uint8_t>(byteValue), &errorMessage)) {
            std::cerr << "loopback write failed: " << errorMessage << '\n';
            return 1;
        }

        uint8_t readValue = 0;
        if (!uart.readByte(&readValue, 500000, &errorMessage)) {
            std::cerr << "loopback read failed: " << errorMessage << '\n';
            return 1;
        }

        std::cout << "tx=";
        printByteHex(static_cast<uint8_t>(byteValue));
        std::cout << " rx=";
        printByteHex(readValue);
        std::cout << '\n';
        return readValue == static_cast<uint8_t>(byteValue) ? 0 : 2;
    }

    Scs0009Servo servo(&uart);

    if (command == "ping-trace") {
        int traceBytes = 8;
        int id = 0;
        if (!parseInt(argv[2], &id) || id < 0 || id > 253) {
            std::cerr << "invalid servo id\n";
            return 1;
        }
        if (argc >= 4 && (!parseInt(argv[3], &traceBytes) || traceBytes <= 0 || traceBytes > 64)) {
            std::cerr << "invalid trace byte count\n";
            return 1;
        }

        if (!servo.sendPing(static_cast<uint8_t>(id), &errorMessage)) {
            std::cerr << "send ping failed: " << errorMessage << '\n';
            return 1;
        }

        std::cout << "ping sent, waiting for up to " << traceBytes << " byte(s)\n";
        for (int index = 0; index < traceBytes; ++index) {
            uint8_t value = 0;
            if (!uart.readByte(&value, index == 0 ? 50000 : 5000, &errorMessage)) {
                std::cerr << "trace stopped after " << index << " byte(s): " << errorMessage << '\n';
                return index == 0 ? 2 : 0;
            }

            std::cout << "byte[" << index << "]=";
            printByteHex(value);
            std::cout << '\n';
        }
        return 0;
    }

    int id = 0;
    if (!parseInt(argv[2], &id) || id < 0 || id > 253) {
        std::cerr << "invalid servo id\n";
        return 1;
    }

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