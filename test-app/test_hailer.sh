#! /bin/sh

# Simple shell script to enable easy testing of HAILER for dev purposes
# Run with sudo privilages --> sudo ./test_hailer.sh

(cd .. && make ) 
gcc test_app_a.c -I../src/include -lhailer -o test_app_a;
gcc test_app_b.c -I../src/include -lhailer -o test_app_b;

killall test_app_a;
sleep 1;
killall test_app_b;
sleep 1;
killall hailer_server;
rm -f /var/.hailer_server_address.sock;

../bin/hailer_server &