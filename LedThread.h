#pragma once

#include <QObject>

class HardwareHal;
class QThread;

class LedThread : public QObject
{
    Q_OBJECT

public:
    explicit LedThread(HardwareHal* hal, QObject* parent = nullptr);
    ~LedThread() override;

    LedThread(const LedThread&) = delete;
    LedThread& operator=(const LedThread&) = delete;

public slots:
    void start();

signals:
    void cmd(const QString& command);

private:
    class Worker;

    QThread* thread_;
    Worker* worker_;
};