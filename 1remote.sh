#!/bin/bash

./build_cli.sh

rm -rf remote5  
 
mkdir remote5

cd remote5

cp ../exe-ocd.sh .

cmake -DPICO_BOARD=pico_w  -DHOSTNAME="remote5" -DWIFI_SSID="ATTtpHTfPi_Guest" -DWIFI_PASSWORD="t?bqxvcqh?6t"  ..


make

cd ../

echo "Done"



