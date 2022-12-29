#! /bin/sh

# Simple shell script to enable easy testing of HAILER for dev purposes
# Run with sudo privilages --> sudo ./test_hailer.sh

killall hailer_test_a;
sleep 1;
killall hailer_test_b;
sleep 1;
killall hailer_server;
rm -f /var/.hailer_server_address.sock;

../bin/bin/hailer_server wlan0 &