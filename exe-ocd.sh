#!/bin/bash
openocd -f interface/raspberrypi-swd.cfg -f target/rp2040.cfg -c "program pico_w/wifi/freertos/iperf/picow_freertos_iperf_server_nosys.elf verify reset exit"
