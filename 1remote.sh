#!/bin/bash

./build_cli.sh

rm -rf remote5  
 
mkdir remote5

cd remote5

cp ../exe-ocd.sh .

cmake -DPICO_BOARD=pico_w  -DHOSTNAME="remote5" -DWIFI_SSID="WIFI_SSID" -DWIFI_PASSWORD="WIFI_PASSWORD" -DFREERTOS_KERNEL_PATH:PATH=/home/devel/FreeRTOS-Kernel ..


make

cd ../

echo "Done"



