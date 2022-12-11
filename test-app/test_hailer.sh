#! /bin/sh

# Simple shell script to enable easy testing of HAILER for dev purposes
# Run with sudo privilages --> sudo ./test_hailer.sh

(cd .. && make ) 
gcc test_app_a.c -I../src/include -lhailer -o test_app_a.exe;
gcc test_app_b.c -I../src/include -lhailer -o test_app_b.exe;

killall test_app_a.exe;
sleep 1;
killall test_app_b.exe;
sleep 1;
killall hailer_server;
rm -f /var/.hailer_server_address.sock;

../bin/hailer_server wlan0 &