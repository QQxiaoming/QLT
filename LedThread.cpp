#include "LedThread.h"

#include "HardwareHal.h"

#include <algorithm>
#include <QColor>
#include <QThread>
#include <QTimer>
#include <QStringList>

class LedThread::Worker : public QObject
{
public:
    explicit Worker(HardwareHal* hal)
                : hal_(hal), effectTimer_(nullptr), mode_(Mode::None), effectR_(0), effectG_(0), effectB_(0),
                    effectHue_(0), effectOn_(false)
    {
    }

public slots:
    void initialize()
    {
        effectTimer_ = new QTimer(this);
        effectTimer_->setTimerType(Qt::PreciseTimer);
        connect(effectTimer_, &QTimer::timeout, this, &Worker::updateEffect);
    }

    void execute(const QString& command)
    {
        const QStringList arguments = command.simplified().split(' ', Qt::SkipEmptyParts);
        if (arguments.isEmpty()) {
            return;
        }

        const QString operation = arguments.at(0).toLower();
        if (operation == "fixed" && hasArguments(arguments, 4)) {
            stopEffectTimer();
            writeAllColor(toColor(toInt(arguments.at(1))),
                          toColor(toInt(arguments.at(2))),
                          toColor(toInt(arguments.at(3))));
        } else if (operation == "blink" && hasArguments(arguments, 5)) {
            effectR_ = toColor(toInt(arguments.at(1)));
            effectG_ = toColor(toInt(arguments.at(2)));
            effectB_ = toColor(toInt(arguments.at(3)));
            effectOn_ = false;
            mode_ = Mode::Blink;
            writeAllColor(0, 0, 0);
            effectTimer_->start(std::max(1, toInt(arguments.at(4))));
        } else if (operation == "rainbow" && hasArguments(arguments, 1)) {
            stopEffectTimer();
            writeRainbow(0);
        } else if (operation == "rainbow_flow" && hasArguments(arguments, 1)) {
            mode_ = Mode::RainbowFlow;
            effectHue_ = 0;
            writeRainbow(effectHue_);
            effectTimer_->start(kRainbowFlowIntervalMs);
        }
    }

private:
    enum class Mode {
        None,
        Blink,
        RainbowFlow
    };

    static constexpr int kRainbowFlowIntervalMs = 60;

    static bool hasArguments(const QStringList& arguments, int count)
    {
        return arguments.size() == count;
    }

    static int toInt(const QString& value, int minimum = -2147483647, int maximum = 2147483647)
    {
        bool ok = false;
        const int number = value.toInt(&ok);
        if (!ok) {
            return minimum;
        }
        return std::max(minimum, std::min(number, maximum));
    }

    static uint8_t toColor(int value)
    {
        if (value < 0) {
            return 0;
        }
        if (value > 255) {
            return 255;
        }
        return static_cast<uint8_t>(value);
    }

    void stopEffectTimer()
    {
        mode_ = Mode::None;
        if (effectTimer_ != nullptr) {
            effectTimer_->stop();
        }
    }

    void writeAllColor(uint8_t r, uint8_t g, uint8_t b)
    {
        for (int index = 0; index < 6; ++index) {
            hal_->setRgbBothLed(static_cast<uint8_t>(index), r, g, b);
        }
    }

    void writeRainbow(int hueOffset)
    {
        for (int index = 0; index < 6; ++index) {
            const int hue = (hueOffset + index * 60) % 360;
            const QColor color = QColor::fromHsv(hue, 255, 255);
            hal_->setRgbBothLed(static_cast<uint8_t>(index),
                                static_cast<uint8_t>(color.red()),
                                static_cast<uint8_t>(color.green()),
                                static_cast<uint8_t>(color.blue()));
        }
    }

    void updateEffect()
    {
        if (mode_ == Mode::Blink) {
            effectOn_ = !effectOn_;
            writeAllColor(effectOn_ ? effectR_ : 0,
                          effectOn_ ? effectG_ : 0,
                          effectOn_ ? effectB_ : 0);
        } else if (mode_ == Mode::RainbowFlow) {
            effectHue_ = (effectHue_ + 8) % 360;
            writeRainbow(effectHue_);
        }
    }

    HardwareHal* hal_;
    QTimer* effectTimer_;
    Mode mode_;
    uint8_t effectR_;
    uint8_t effectG_;
    uint8_t effectB_;
    int effectHue_;
    bool effectOn_;
};

LedThread::LedThread(HardwareHal* hal, QObject* parent)
    : QObject(parent), thread_(new QThread(this)), worker_(new Worker(hal))
{
    worker_->moveToThread(thread_);

    connect(thread_, &QThread::started, worker_, &Worker::initialize);
    connect(this, &LedThread::cmd, worker_, &Worker::execute, Qt::QueuedConnection);
    connect(thread_, &QThread::finished, worker_, &QObject::deleteLater);
}

LedThread::~LedThread()
{
    thread_->quit();
    thread_->wait();
}

void LedThread::start()
{
    if (!thread_->isRunning()) {
        thread_->start();
    }
}

