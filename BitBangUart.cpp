#include "BitBangUart.h"

#include <gpiod.h>

#include <cerrno>
#include <climits>
#include <cstring>
#include <ctime>
#include <sched.h>
#include <sstream>
#include <sys/mman.h>
#include <unistd.h>

namespace {

timespec toTimespec(uint64_t nanoseconds)
{
    timespec spec;
    spec.tv_sec = static_cast<time_t>(nanoseconds / 1000000000ULL);
    spec.tv_nsec = static_cast<long>(nanoseconds % 1000000000ULL);
    return spec;
}

}

BitBangUart::BitBangUart(const std::string &txChipName,
                         unsigned int txLineOffset,
                         const std::string &rxChipName,
                         unsigned int rxLineOffset,
                         unsigned int baudRate)
    : txChipName_(txChipName)
    , txLineOffset_(txLineOffset)
    , rxChipName_(rxChipName)
    , rxLineOffset_(rxLineOffset)
    , baudRate_(baudRate)
    , bitPeriodNs_(baudRate == 0 ? 0 : 1000000000ULL / baudRate)
    , txChip_(nullptr)
    , txLine_(nullptr)
    , rxChip_(nullptr)
    , rxLine_(nullptr)
{
}

BitBangUart::~BitBangUart()
{
    close();
}

bool BitBangUart::open(std::string *errorMessage)
{
    close();

    txChip_ = gpiod_chip_open_by_name(txChipName_.c_str());
    if (!txChip_) {
        if (errorMessage)
            *errorMessage = errnoMessage("open tx gpio chip failed");
        return false;
    }

    txLine_ = gpiod_chip_get_line(txChip_, txLineOffset_);
    if (!txLine_) {
        if (errorMessage)
            *errorMessage = errnoMessage("get tx gpio line failed");
        close();
        return false;
    }

    if (gpiod_line_request_output(txLine_, "scs0009-tx", 1) < 0) {
        if (errorMessage)
            *errorMessage = errnoMessage("request tx gpio output failed");
        close();
        return false;
    }

    rxChip_ = gpiod_chip_open_by_name(rxChipName_.c_str());
    if (!rxChip_) {
        if (errorMessage)
            *errorMessage = errnoMessage("open rx gpio chip failed");
        close();
        return false;
    }

    rxLine_ = gpiod_chip_get_line(rxChip_, rxLineOffset_);
    if (!rxLine_) {
        if (errorMessage)
            *errorMessage = errnoMessage("get rx gpio line failed");
        close();
        return false;
    }

    if (gpiod_line_request_both_edges_events(rxLine_, "scs0009-rx") < 0) {
        if (errorMessage)
            *errorMessage = errnoMessage("request rx gpio events failed");
        close();
        return false;
    }

    return true;
}

void BitBangUart::close()
{
    if (rxLine_) {
        gpiod_line_release(rxLine_);
        rxLine_ = nullptr;
    }
    if (rxChip_) {
        gpiod_chip_close(rxChip_);
        rxChip_ = nullptr;
    }
    if (txLine_) {
        gpiod_line_release(txLine_);
        txLine_ = nullptr;
    }
    if (txChip_) {
        gpiod_chip_close(txChip_);
        txChip_ = nullptr;
    }
}

bool BitBangUart::isOpen() const
{
    return txLine_ && rxLine_;
}

bool BitBangUart::elevateProcess(std::string *errorMessage) const
{
    if (mlockall(MCL_CURRENT | MCL_FUTURE) < 0) {
        if (errorMessage)
            *errorMessage = errnoMessage("mlockall failed");
        return false;
    }

    sched_param parameter;
    std::memset(&parameter, 0, sizeof(parameter));
    parameter.sched_priority = 80;
    if (sched_setscheduler(0, SCHED_FIFO, &parameter) < 0) {
        if (errorMessage)
            *errorMessage = errnoMessage("sched_setscheduler failed");
        return false;
    }

    cpu_set_t cpuSet;
    CPU_ZERO(&cpuSet);
    CPU_SET(0, &cpuSet);
    if (sched_setaffinity(0, sizeof(cpuSet), &cpuSet) < 0) {
        if (errorMessage)
            *errorMessage = errnoMessage("sched_setaffinity failed");
        return false;
    }

    return true;
}

bool BitBangUart::writeByte(uint8_t value, std::string *errorMessage)
{
    if (!isOpen()) {
        if (errorMessage)
            *errorMessage = "bit-bang uart is not open";
        return false;
    }

    uint64_t edgeTime = monotonicNowNs();

    if (!setTxValue(0, errorMessage))
        return false;

    edgeTime += bitPeriodNs_;
    if (!sleepUntil(edgeTime, errorMessage))
        return false;

    for (int bit = 0; bit < 8; ++bit) {
        if (!setTxValue((value >> bit) & 0x01, errorMessage))
            return false;
        edgeTime += bitPeriodNs_;
        if (!sleepUntil(edgeTime, errorMessage))
            return false;
    }

    if (!setTxValue(1, errorMessage))
        return false;
    edgeTime += bitPeriodNs_;
    return sleepUntil(edgeTime, errorMessage);
}

bool BitBangUart::writeBytes(const std::vector<uint8_t> &data, std::string *errorMessage)
{
    for (std::size_t index = 0; index < data.size(); ++index) {
        if (!writeByte(data[index], errorMessage))
            return false;
    }
    return true;
}

bool BitBangUart::readByte(uint8_t *value, int startTimeoutUs, std::string *errorMessage)
{
    if (!value) {
        if (errorMessage)
            *errorMessage = "readByte received null output pointer";
        return false;
    }

    if (!waitForStartBit(startTimeoutUs, errorMessage))
        return false;

    uint8_t assembled = 0;
    uint64_t sampleTime = monotonicNowNs() + bitPeriodNs_ + bitPeriodNs_ / 2;
    for (int bit = 0; bit < 8; ++bit) {
        if (!sleepUntil(sampleTime, errorMessage))
            return false;

        int rxValue = 1;
        if (!sampleRxValue(&rxValue, errorMessage))
            return false;

        if (rxValue)
            assembled |= static_cast<uint8_t>(1U << bit);

        sampleTime += bitPeriodNs_;
    }

    if (!sleepUntil(sampleTime, errorMessage))
        return false;

    int stopBit = 1;
    if (!sampleRxValue(&stopBit, errorMessage))
        return false;
    if (stopBit == 0) {
        if (errorMessage)
            *errorMessage = "invalid stop bit";
        return false;
    }

    *value = assembled;
    return true;
}

bool BitBangUart::readBytes(std::vector<uint8_t> *data,
                            std::size_t count,
                            int startTimeoutUs,
                            std::string *errorMessage)
{
    if (!data) {
        if (errorMessage)
            *errorMessage = "readBytes received null vector";
        return false;
    }

    data->clear();
    data->reserve(count);
    for (std::size_t index = 0; index < count; ++index) {
        uint8_t value = 0;
        const int timeoutUs = index == 0 ? startTimeoutUs : static_cast<int>(bitPeriodNs_ / 1000ULL * 12ULL);
        if (!readByte(&value, timeoutUs, errorMessage))
            return false;
        data->push_back(value);
    }
    return true;
}

unsigned int BitBangUart::baudRate() const
{
    return baudRate_;
}

uint64_t BitBangUart::bitPeriodNs() const
{
    return bitPeriodNs_;
}

bool BitBangUart::setTxValue(int value, std::string *errorMessage)
{
    if (gpiod_line_set_value(txLine_, value) < 0) {
        if (errorMessage)
            *errorMessage = errnoMessage("set tx gpio value failed");
        return false;
    }
    return true;
}

bool BitBangUart::sampleRxValue(int *value, std::string *errorMessage) const
{
    const int rxValue = gpiod_line_get_value(rxLine_);
    if (rxValue < 0) {
        if (errorMessage)
            *errorMessage = errnoMessage("read rx gpio value failed");
        return false;
    }

    *value = rxValue;
    return true;
}

bool BitBangUart::waitForStartBit(int timeoutUs, std::string *errorMessage) const
{
    if (timeoutUs < 0)
        timeoutUs = 0;

    timespec timeout;
    timeout.tv_sec = timeoutUs / 1000000;
    timeout.tv_nsec = static_cast<long>(timeoutUs % 1000000) * 1000L;

    while (true) {
        const int waitResult = gpiod_line_event_wait(rxLine_, &timeout);
        if (waitResult < 0) {
            if (errorMessage)
                *errorMessage = errnoMessage("wait for rx edge failed");
            return false;
        }
        if (waitResult == 0) {
            if (errorMessage)
                *errorMessage = "timeout waiting for start bit";
            return false;
        }

        gpiod_line_event event;
        if (gpiod_line_event_read(rxLine_, &event) < 0) {
            if (errorMessage)
                *errorMessage = errnoMessage("read rx edge event failed");
            return false;
        }

        if (event.event_type != GPIOD_LINE_EVENT_FALLING_EDGE)
            continue;

        int rxValue = 1;
        if (!sampleRxValue(&rxValue, errorMessage))
            return false;

        if (rxValue == 0)
            return true;
    }
}

bool BitBangUart::sleepUntil(uint64_t targetNs, std::string *errorMessage) const
{
    const timespec deadline = toTimespec(targetNs);
    int result = 0;
    do {
        result = clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &deadline, nullptr);
    } while (result == EINTR);

    if (result != 0) {
        if (errorMessage)
            *errorMessage = std::string("clock_nanosleep failed: ") + std::strerror(result);
        return false;
    }

    return true;
}

uint64_t BitBangUart::monotonicNowNs() const
{
    timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return static_cast<uint64_t>(now.tv_sec) * 1000000000ULL + static_cast<uint64_t>(now.tv_nsec);
}

std::string BitBangUart::errnoMessage(const std::string &prefix)
{
    std::ostringstream stream;
    stream << prefix << ": " << std::strerror(errno);
    return stream.str();
}