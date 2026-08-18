#include "GestureThread.h"

#include <algorithm>
#include <functional>
#include <QThread>
#include <QTimer>

namespace {

enum class TouchState {
    Idle,
    Touched,
    Swiping
};

struct TouchData {
    uint8_t intensity[3];

    int16_t position() const
    {
        const uint16_t total = intensity[0] + intensity[1] + intensity[2];
        if (total == 0) {
            return 0;
        }

        const int32_t weighted = intensity[0] * -100 + intensity[2] * 100;
        return static_cast<int16_t>(weighted / total);
    }

    uint8_t maxIntensity() const
    {
        return std::max(intensity[0], std::max(intensity[1], intensity[2]));
    }

    bool isTouched() const
    {
        return maxIntensity() >= 1;
    }
};

class GestureRecognizer
{
public:
    GestureRecognizer()
        : state_(TouchState::Idle), initialPosition_(0)
    {
    }

    HardwareHal::HeadPetGesture update(const TouchData& data)
    {
        switch (state_) {
        case TouchState::Idle:
            if (data.isTouched()) {
                state_ = TouchState::Touched;
                initialPosition_ = data.position();
                return HardwareHal::HeadPetGesture::Press;
            }
            break;

        case TouchState::Touched: {
            if (!data.isTouched()) {
                state_ = TouchState::Idle;
                return HardwareHal::HeadPetGesture::Release;
            }

            const int16_t delta = data.position() - initialPosition_;
            if (delta > 40) {
                state_ = TouchState::Swiping;
                return HardwareHal::HeadPetGesture::SwipeForward;
            }
            if (delta < -40) {
                state_ = TouchState::Swiping;
                return HardwareHal::HeadPetGesture::SwipeBackward;
            }
            break;
        }

        case TouchState::Swiping:
            if (!data.isTouched()) {
                state_ = TouchState::Idle;
                return HardwareHal::HeadPetGesture::Release;
            }
            break;
        }

        return HardwareHal::HeadPetGesture::None;
    }

private:
    TouchState state_;
    int16_t initialPosition_;
};

}  // namespace

class GestureThread::Worker : public QObject
{
public:
    Worker(HardwareHal* hal, const std::function<void(HardwareHal::HeadPetGesture)>& emitGesture)
        : hal_(hal), emitGesture_(emitGesture), timer_(nullptr)
    {
    }

public slots:
    void initialize()
    {
        timer_ = new QTimer(this);
        timer_->setInterval(50);
        timer_->setTimerType(Qt::PreciseTimer);
        connect(timer_, &QTimer::timeout, this, &Worker::poll);
        timer_->start();
    }

private slots:
    void poll()
    {
        TouchData data = {{0, 0, 0}};
        if (!hal_->readTouchIntensities(data.intensity)) {
            return;
        }

        const HardwareHal::HeadPetGesture gesture = recognizer_.update(data);
        if (gesture != HardwareHal::HeadPetGesture::None) {
            emitGesture_(gesture);
        }
    }

private:
    HardwareHal* hal_;
    std::function<void(HardwareHal::HeadPetGesture)> emitGesture_;
    QTimer* timer_;
    GestureRecognizer recognizer_;
};

GestureThread::GestureThread(HardwareHal* hal, QObject* parent)
    : QObject(parent), thread_(new QThread(this)),
      worker_(new Worker(hal, [this](HardwareHal::HeadPetGesture gesture) { emit gestureDetected(gesture); }))
{
    worker_->moveToThread(thread_);
    connect(thread_, &QThread::started, worker_, &Worker::initialize);
    connect(thread_, &QThread::finished, worker_, &QObject::deleteLater);
}

GestureThread::~GestureThread()
{
    thread_->quit();
    thread_->wait();
}

void GestureThread::start()
{
    if (!thread_->isRunning()) {
        thread_->start();
    }
}