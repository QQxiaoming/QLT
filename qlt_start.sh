#!/bin/bash

export QT_QPA_PLATFORM=linuxfb:fb=/dev/fb$(cat /proc/fb | grep ili9342c | awk '{print $1}')
export PATH=$PATH:/usr/local/m5stack/bin
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/m5stack/lib

start-stop-daemon -S -b -x /root/QLT
