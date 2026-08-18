#pragma once

#include <QObject>

#include "HardwareHal.h"

class QThread;

class GestureThread : public QObject
{
    Q_OBJECT

public:
    explicit GestureThread(HardwareHal* hal, QObject* parent = nullptr);
    ~GestureThread() override;

    GestureThread(const GestureThread&) = delete;
    GestureThread& operator=(const GestureThread&) = delete;

public slots:
    void start();

signals:
    void gestureDetected(HardwareHal::HeadPetGesture gesture);

private:
    class Worker;

    QThread* thread_;
    Worker* worker_;
};

Q_DECLARE_METATYPE(HardwareHal::HeadPetGesture)