#include "Scs0009Servo.h"

#include "BitBangUart.h"

#include <sstream>

namespace {

const uint8_t kHeader = 0xFF;
const uint8_t kInstructionPing = 0x01;
const uint8_t kInstructionRead = 0x02;
const uint8_t kInstructionWrite = 0x03;
const uint8_t kRegisterPresentPositionLow = 0x38;
const uint8_t kRegisterGoalPositionLow = 0x2A;

}

Scs0009Servo::Scs0009Servo(BitBangUart *uart)
    : uart_(uart)
{
}

bool Scs0009Servo::ping(uint8_t id, std::string *errorMessage)
{
    if (!sendInstruction(id, kInstructionPing, std::vector<uint8_t>(), errorMessage))
        return false;

    std::vector<uint8_t> packet;
    return readStatusPacket(id, &packet, errorMessage);
}

bool Scs0009Servo::writePosition(uint8_t id,
                                 uint16_t position,
                                 uint16_t time,
                                 uint16_t speed,
                                 std::string *errorMessage)
{
    std::vector<uint8_t> parameters;
    parameters.push_back(kRegisterGoalPositionLow);
    parameters.push_back(static_cast<uint8_t>(position & 0xFF));
    parameters.push_back(static_cast<uint8_t>((position >> 8) & 0xFF));
    parameters.push_back(static_cast<uint8_t>(time & 0xFF));
    parameters.push_back(static_cast<uint8_t>((time >> 8) & 0xFF));
    parameters.push_back(static_cast<uint8_t>(speed & 0xFF));
    parameters.push_back(static_cast<uint8_t>((speed >> 8) & 0xFF));
    return sendInstruction(id, kInstructionWrite, parameters, errorMessage);
}

bool Scs0009Servo::readPosition(uint8_t id, uint16_t *position, std::string *errorMessage)
{
    if (!position) {
        if (errorMessage)
            *errorMessage = "readPosition received null output pointer";
        return false;
    }

    std::vector<uint8_t> parameters;
    parameters.push_back(kRegisterPresentPositionLow);
    parameters.push_back(0x02);
    if (!sendInstruction(id, kInstructionRead, parameters, errorMessage))
        return false;

    std::vector<uint8_t> packet;
    if (!readStatusPacket(id, &packet, errorMessage))
        return false;

    if (packet.size() < 8) {
        if (errorMessage)
            *errorMessage = "status packet too short for position read";
        return false;
    }

    *position = static_cast<uint16_t>(packet[5]) | (static_cast<uint16_t>(packet[6]) << 8);
    return true;
}

bool Scs0009Servo::sendInstruction(uint8_t id,
                                   uint8_t instruction,
                                   const std::vector<uint8_t> &parameters,
                                   std::string *errorMessage)
{
    if (!uart_ || !uart_->isOpen()) {
        if (errorMessage)
            *errorMessage = "uart is not open";
        return false;
    }

    std::vector<uint8_t> packet;
    packet.reserve(parameters.size() + 6);
    packet.push_back(kHeader);
    packet.push_back(kHeader);
    packet.push_back(id);
    packet.push_back(static_cast<uint8_t>(parameters.size() + 2));
    packet.push_back(instruction);
    packet.insert(packet.end(), parameters.begin(), parameters.end());
    packet.push_back(checksum(packet));
    return uart_->writeBytes(packet, errorMessage);
}

bool Scs0009Servo::readStatusPacket(uint8_t expectedId,
                                    std::vector<uint8_t> *packet,
                                    std::string *errorMessage)
{
    if (!packet) {
        if (errorMessage)
            *errorMessage = "readStatusPacket received null vector";
        return false;
    }

    packet->clear();

    uint8_t byte = 0;
    do {
        if (!uart_->readByte(&byte, 20000, errorMessage))
            return false;
    } while (byte != kHeader);
    packet->push_back(byte);

    if (!uart_->readByte(&byte, 2000, errorMessage))
        return false;
    if (byte != kHeader) {
        if (errorMessage)
            *errorMessage = "invalid status packet header";
        return false;
    }
    packet->push_back(byte);

    if (!uart_->readByte(&byte, 2000, errorMessage))
        return false;
    packet->push_back(byte);
    if (byte != expectedId) {
        if (errorMessage) {
            std::ostringstream stream;
            stream << "unexpected servo id " << static_cast<int>(byte)
                   << ", expected " << static_cast<int>(expectedId);
            *errorMessage = stream.str();
        }
        return false;
    }

    if (!uart_->readByte(&byte, 2000, errorMessage))
        return false;
    packet->push_back(byte);

    const std::size_t payloadBytes = static_cast<std::size_t>(byte);
    std::vector<uint8_t> tail;
    if (!uart_->readBytes(&tail, payloadBytes, 2000, errorMessage))
        return false;
    packet->insert(packet->end(), tail.begin(), tail.end());

    if (packet->size() < 6) {
        if (errorMessage)
            *errorMessage = "status packet too short";
        return false;
    }

    const uint8_t receivedChecksum = packet->back();
    packet->pop_back();
    const uint8_t computedChecksum = checksum(*packet);
    packet->push_back(receivedChecksum);

    if (receivedChecksum != computedChecksum) {
        if (errorMessage)
            *errorMessage = "status packet checksum mismatch";
        return false;
    }

    const uint8_t errorByte = packet->at(4);
    if (errorByte != 0) {
        if (errorMessage) {
            std::ostringstream stream;
            stream << "servo returned error 0x" << std::hex << static_cast<int>(errorByte);
            *errorMessage = stream.str();
        }
        return false;
    }

    return true;
}

uint8_t Scs0009Servo::checksum(const std::vector<uint8_t> &packet) const
{
    unsigned int sum = 0;
    for (std::size_t index = 2; index < packet.size(); ++index)
        sum += packet[index];
    return static_cast<uint8_t>(~(sum & 0xFF));
}