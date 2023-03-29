#!/bin/bash

./build_cli.sh

rm -rf remote1 remote2 remote3 remote4 remote5 remote6
 
mkdir remote1

cd remote1

cp ../exe-ocd.sh .

cmake -DPICO_BOARD=pico_w  -DHOSTNAME="remote1" -DWIFI_SSID="WIFI_SSID" -DWIFI_PASSWORD="WIFI_PASSWORD" -DFREERTOS_KERNEL_PATH:PATH=/home/devel/FreeRTOS-Kernel ..


make

cd ../

mkdir remote2

cd remote2

cp ../exe-ocd.sh .

cmake -DPICO_BOARD=pico_w  -DHOSTNAME="remote2" -DWIFI_SSID="WIFI_SSID" -DWIFI_PASSWORD="WIFI_PASSWORD" -DFREERTOS_KERNEL_PATH:PATH=/home/devel/FreeRTOS-Kernel ..

make

cd ../

mkdir remote3

cd remote3

cp ../exe-ocd.sh .

cmake -DPICO_BOARD=pico_w  -DHOSTNAME="remote3" -DWIFI_SSID="WIFI_SSID" -DWIFI_PASSWORD="WIFI_PASSWORD" -DFREERTOS_KERNEL_PATH:PATH=/home/devel/FreeRTOS-Kernel ..

make

cd ../

mkdir remote4

cd remote4

cp ../exe-ocd.sh .

cmake -DPICO_BOARD=pico_w  -DHOSTNAME="remote4" -DWIFI_SSID="WIFI_SSID" -DWIFI_PASSWORD="WIFI_PASSWORD" -DFREERTOS_KERNEL_PATH:PATH=/home/devel/FreeRTOS-Kernel ..

make

cd ../

mkdir remote5

cd remote5

cp ../exe-ocd.sh .

cmake -DPICO_BOARD=pico_w  -DHOSTNAME="remote5" -DWIFI_SSID="WIFI_SSID" -DWIFI_PASSWORD="WIFI_PASSWORD" -DFREERTOS_KERNEL_PATH:PATH=/home/devel/FreeRTOS-Kernel ..

make

cd ../

mkdir remote6

cd remote6

cp ../exe-ocd.sh .

cmake -DPICO_BOARD=pico_w  -DHOSTNAME="remote6" -DWIFI_SSID="WIFI_SSID" -DWIFI_PASSWORD="WIFI_PASSWORD" -DFREERTOS_KERNEL_PATH:PATH=/home/devel/FreeRTOS-Kernel ..

make

cd ../

echo "Done"



