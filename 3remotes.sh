#!/bin/bash

./build_cli.sh

rm -rf remote7 remote8 remote9
 
mkdir remote7

cd remote7

cp ../exe-ocd.sh .

cmake -DPICO_BOARD=pico_w  -DHOSTNAME="remote7" -DWIFI_SSID="WIFI_SSID" -DWIFI_PASSWORD="WIFI_PASSWORD"  ..


make

cd ../

mkdir remote8

cd remote8

cp ../exe-ocd.sh .

cmake -DPICO_BOARD=pico_w  -DHOSTNAME="remote8" -DWIFI_SSID="WIFI_SSID" -DWIFI_PASSWORD="WIFI_PASSWORD"   ..

make

cd ../

mkdir remote9

cd remote9

cp ../exe-ocd.sh .

cmake -DPICO_BOARD=pico_w  -DHOSTNAME="remote9" -DWIFI_SSID="WIFI_SSID" -DWIFI_PASSWORD="WIFI_PASSWORD"   ..

make

cd ../

cd ../

echo "Done"



