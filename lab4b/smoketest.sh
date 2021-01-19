# NAME: Seungwon Kim
# EMAIL: haleykim@g.ucla.edu
# ID: 405111152

#!/bin/bash

./lab4b --period=3 --scale=F --log=log.txt <<-EOF
SCALE=F
PERIOD=1
START
STOP
LOG test
OFF
EOF

if [ $? -ne 0 ]
then
	echo "Test Failed: wrong exit code\n"
else
	echo "Test Passed: valid exit code\n"
fi

if [ ! -s log.txt ]
then
	echo "Test Failed: log.txt not created\n"
else
	echo "Test Passed: log.txt created\n"
fi

grep "SCALE=F" log.txt > /dev/null;
if [ $? -ne 0 ]
then
	echo "Test Failed: SCALE=F not recorded in log.txt\n"
else
	echo "Test Passed: SCALE=F recorded in log.txt\n"
fi

grep "PERIOD=1" log.txt > /dev/null;
if [ $? -ne 0 ]
then
	echo "Test Failed: PERIOD=1 not recorded in log.txt\n"
else
	echo "Test Passed: PERIOD=1 recorded in log.txt\n"
fi

grep "START" log.txt > /dev/null;
if [ $? -ne 0 ]
then
	echo "Test Failed: START not recorded in log.txt\n"
else
	echo "Test Passed: START recorded in log.txt\n"
fi

grep "STOP" log.txt > /dev/null;
if [ $? -ne 0 ]
then
	echo "Test Failed: STOP not recorded in log.txt\n"
else
	echo "Test Passed: STOP recorded in log.txt\n"
fi

grep "LOG test" log.txt > /dev/null;
if [ $? -ne 0 ]
then
	echo "Test Failed: LOG test  not logged in log.txt\n"
else
	echo "Test Passed: LOG test logged in log.txt\n"
fi

grep "OFF" log.txt > /dev/null;
if [ $? -ne 0 ]
then
	echo "Test Failed: OFF not recorded in log.txt\n"
else
	echo "Test Passed: OFF recorded in log.txt\n"
fi