#!/bin/bash

export QT_QPA_PLATFORM=linuxfb:fb=/dev/fb$(cat /proc/fb | grep ili9342c | awk '{print $1}')
export PATH=$PATH:/usr/local/m5stack/bin
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/m5stack/lib

# try start QLT program, check ps, if QLT program is not started, restart QLT program
while true; do
    start-stop-daemon -S -b -x /root/QLT
    sleep 5
    if ! pgrep -a -x "QLT"; then
        echo "QLT program is not running, restarting..."
    else
        echo "QLT program is running."
        break
    fi
done

