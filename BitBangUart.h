#ifndef BITBANGUART_H
#define BITBANGUART_H

#include <cstdint>
#include <string>
#include <vector>

struct gpiod_chip;
struct gpiod_line;

class BitBangUart
{
public:
    BitBangUart(const std::string &txChipName,
                unsigned int txLineOffset,
                const std::string &rxChipName,
                unsigned int rxLineOffset,
                unsigned int baudRate);
    ~BitBangUart();

    BitBangUart(const BitBangUart &) = delete;
    BitBangUart &operator=(const BitBangUart &) = delete;

    bool open(std::string *errorMessage = nullptr);
    void close();
    bool isOpen() const;

    bool elevateProcess(std::string *errorMessage = nullptr) const;

    bool writeByte(uint8_t value, std::string *errorMessage = nullptr);
    bool writeBytes(const std::vector<uint8_t> &data, std::string *errorMessage = nullptr);

    bool readByte(uint8_t *value,
                  int startTimeoutUs,
                  std::string *errorMessage = nullptr);
    bool readBytes(std::vector<uint8_t> *data,
                   std::size_t count,
                   int startTimeoutUs,
                   std::string *errorMessage = nullptr);
    bool waitForEdge(bool *fallingEdge,
                     uint64_t *edgeTimeNs,
                     int timeoutUs,
                     std::string *errorMessage = nullptr) const;

    unsigned int baudRate() const;
    uint64_t bitPeriodNs() const;

private:
    bool setTxValue(int value, std::string *errorMessage);
    bool sampleRxValue(int *value, std::string *errorMessage) const;
    bool waitForStartBit(int timeoutUs, uint64_t *startBitNs, std::string *errorMessage) const;
    bool sleepUntil(uint64_t targetNs, std::string *errorMessage) const;
    uint64_t monotonicNowNs() const;
    static std::string errnoMessage(const std::string &prefix);

    std::string txChipName_;
    unsigned int txLineOffset_;
    std::string rxChipName_;
    unsigned int rxLineOffset_;
    unsigned int baudRate_;
    uint64_t bitPeriodNs_;

    gpiod_chip *txChip_;
    gpiod_line *txLine_;
    gpiod_chip *rxChip_;
    gpiod_line *rxLine_;
};

#endif