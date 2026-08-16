#ifndef SCS0009SERVO_H
#define SCS0009SERVO_H

#include <cstdint>
#include <string>
#include <vector>

class BitBangUart;

class Scs0009Servo
{
public:
    explicit Scs0009Servo(BitBangUart *uart);

    bool ping(uint8_t id, std::string *errorMessage = nullptr);
    bool sendPing(uint8_t id, std::string *errorMessage = nullptr);
    bool writePosition(uint8_t id,
                       uint16_t position,
                       uint16_t time,
                       uint16_t speed,
                       std::string *errorMessage = nullptr);
    bool readPosition(uint8_t id, uint16_t *position, std::string *errorMessage = nullptr);

private:
    bool sendInstruction(uint8_t id,
                         uint8_t instruction,
                         const std::vector<uint8_t> &parameters,
                         std::string *errorMessage);
    bool readStatusPacket(uint8_t expectedId,
                          std::vector<uint8_t> *packet,
                          std::string *errorMessage);
    uint8_t checksum(const std::vector<uint8_t> &packet) const;

    BitBangUart *uart_;
};

#endif