#!/usr/bin/env bash

kill -TERM "$(pgrep huinit)"
make huinit
rm -f /tmp/huinit.log
printf '1\n2\n3\n' >input_wc
echo '/usr/bin/sleep 21 /home/sdd/huinit/input_sleep_21 /home/sdd/huinit/output_sleep_21' >configuration.text
echo '/usr/bin/sleep 5 /home/sdd/huinit/input_sleep_5 /home/sdd/huinit/output_sleep_5' >>configuration.text
echo '/usr/bin/sleep 13 /home/sdd/huinit/input_sleep_13  /home/sdd/huinit/output_sleep_13' >>configuration.text
./huinit "$(pwd)"/configuration.text
sleep 1
printf 'sleep 21 %d\n' $(pgrep -f 'sleep 21')
printf 'sleep 5 %d\n' $(pgrep -f 'sleep 5')
printf 'sleep 13 %d\n' $(pgrep -f 'sleep 13')

huinit="$(pgrep huinit)"
sleep_5="$(pgrep -f 'sleep 5')"

echo 'kill sleep 5'
pkill "${sleep_5}"
sleep 1
printf 'sleep 21 %d\n' $(pgrep -f 'sleep 21')
printf 'sleep 5 %d\n' $(pgrep -f 'sleep 5')
printf 'sleep 13 %d\n' $(pgrep -f 'sleep 13')
echo 'edit configuration.text'
echo '/usr/bin/sleep 5 /home/sdd/huinit/input_sleep_5 /home/sdd/huinit/output_sleep_5' >configuration.text
sleep 1
kill -HUP "${huinit}"
sleep 1
printf 'sleep 21 %d\n' $(pgrep -f 'sleep 21')
printf 'sleep 5 %d\n' $(pgrep -f 'sleep 5')
printf 'sleep 13 %d\n' $(pgrep -f 'sleep 13')
kill -TERM "${huinit}"
