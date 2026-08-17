#include <QApplication>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>

#include "RobotFaceWidget.h"
#include "PY32IOExpander.hpp"

void showRgbColor(m5::PY32IOExpander &io_expander, uint8_t r, uint8_t g, uint8_t b)
{
    for (int i = 0; i < 12; i++) {
        io_expander.setLedColor(i, r, g, b);
    }
    io_expander.refreshLeds();
}

void setServoPowerEnabled(m5::PY32IOExpander &io_expander, bool enabled)
{
    io_expander.digitalWrite(0, enabled ? true : false);
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    //1. 开启m5 bus供电
    system("echo 131 > /sys/class/gpio/export");
    system("echo out > /sys/class/gpio/PI3/direction");
    system("echo 1 > /sys/class/gpio/PI3/value");
    
    //2. 初始化PY32IOExpander
    m5::PY32IOExpander io_expander("/dev/i2c-1");
    if (!io_expander.begin()) {
        std::fprintf(stderr, "Failed to initialize PY32IOExpander on /dev/i2c-1\n");
        return 1;
    }

    // 3. VM EN
    io_expander.setDirection(0, true);  // Output
    io_expander.setPullMode(0, true);   // Pull-up
    setServoPowerEnabled(io_expander, true);
    usleep(200*1000);

    // 4. RGB
    io_expander.setDirection(13, true);   // Output
    io_expander.setPullMode(13, true);    // Pull-up
    io_expander.setDriveMode(13, false);  // Push-pull
    io_expander.setLedCount(12);
    usleep(200*1000);
    showRgbColor(io_expander, 0, 0, 0);
    usleep(50*1000);
    showRgbColor(io_expander, 0, 0, 0);
    usleep(100*1000);

    // 5. 设置RGB颜色 
    io_expander.setLedColor(0, 255, 0, 0);
    io_expander.refreshLeds();
    io_expander.setLedColor(11,255, 0, 0);
    io_expander.refreshLeds();

    RobotFaceWidget face;
    face.setWindowFlag(Qt::FramelessWindowHint);
    face.showFullScreen();

    return app.exec();
}
