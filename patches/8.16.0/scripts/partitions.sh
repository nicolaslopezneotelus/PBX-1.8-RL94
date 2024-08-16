#!/bin/bash

#Filesystem     1048576-blocks  Used Available Capacity Mounted on
#/dev/sda3              465191  4245    437309       1% /
#tmpfs                     938    11       928       2% /dev/shm
#/dev/sda1                 190    93        87      52% /boot

#Filesystem         1048576-blocks      Used Available Capacity Mounted on
#/dev/cciss/c0d1p2       452956      2589    426988       1% /
#/dev/cciss/c0d1p1         1181        40      1081       4% /boot
#none                      1971         0      1971       0% /dev/shm
#none                      2500       159      2342       7% /mnt/ramdisk

echo Filesystem"|"Size"|"Used"|"Available"|"Use"|"Mounted

parse=0
df -P -t xfs -t ext3 -t tmpfs -B 1048576 | awk -v OFS='|' '{print $1,$2,$3,$4,$5,$6}' | while read line;
do
  #echo $line
  if [[ $line = "" ]]; then
    continue
  elif [[ $line = "Filesystem|1048576-blocks|Used|Available|Capacity|Mounted" ]]; then
    parse=1
    continue
  elif [ $parse -eq 0 ]; then
    continue
  fi
  echo $line
done
