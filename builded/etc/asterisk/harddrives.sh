#!/bin/bash

#smartctl 5.43 2012-06-30 r3573 [x86_64-linux-2.6.32-504.16.2.el6.x86_64] (local build)
#Copyright (C) 2002-12 by Bruce Allen, http://smartmontools.sourceforge.net

#=== START OF READ SMART DATA SECTION ===
#SMART Attributes Data Structure revision number: 16
#Vendor Specific SMART Attributes with Thresholds:
#ID# ATTRIBUTE_NAME          FLAG     VALUE WORST THRESH TYPE      UPDATED  WHEN_FAILED RAW_VALUE
#  1 Raw_Read_Error_Rate     0x002f   200   200   051    Pre-fail  Always       -       0
#  3 Spin_Up_Time            0x0027   141   139   021    Pre-fail  Always       -       3933
#  4 Start_Stop_Count        0x0032   100   100   000    Old_age   Always       -       92
#  5 Reallocated_Sector_Ct   0x0033   200   200   140    Pre-fail  Always       -       0
#  7 Seek_Error_Rate         0x002e   200   200   000    Old_age   Always       -       0
#  9 Power_On_Hours          0x0032   066   066   000    Old_age   Always       -       24848

echo Device"|"Attribute"|"Value

for dev in {hda,hdb,hdc,hdd,hde,hdf,hdg,hdh,sda,sdb,sdc,sdd,sde,sdf,sdg,sdh,cciss/c0d0,sg0,sg1,sg2}
do
  #echo $dev
  if [ ! -e "/dev/$dev" ]; then
    continue
  fi
  if [[ $dev = "cciss/c0d0" ]] || [[ $dev = "sg0" ]]; then
    #echo $dev
    for x in {0..2}
    do
      parse=0
      command="smartctl -A /dev/$dev -d cciss,$x"
      #echo $command
      result=$($command 2>/dev/null)
      exitcode=$?
      #echo $exitcode
      if [ $exitcode -ne 0 ]; then
        continue
      fi
      while read -r line; do
        #echo $line
        IFS=' ' read -r -a array <<< "$line"
        if [[ ${array[0]} = "ID#" ]]; then
          parse=1
          continue
        elif [ $parse -eq 0 ]; then
          continue
        fi
        IFS=', ' read -r -a array <<< "$line"
        echo "$dev|${array[1]}|${array[9]}"
      done <<< "$result"
    done
  else
    #echo $dev
    parse=0
    command="smartctl -A /dev/$dev"
    #echo $command
    result=$($command 2>/dev/null)
    exitcode=$?
    #echo $exitcode
    if [ $exitcode -ne 0 ]; then
      continue
    fi
    while read -r line; do
      #echo $line
      IFS=' ' read -r -a array <<< "$line"
      if [[ ${array[0]} = "ID#" ]]; then
        parse=1
        continue
      elif [ $parse -eq 0 ]; then
        continue
      fi
      IFS=', ' read -r -a array <<< "$line"
      echo "$dev|${array[1]}|${array[9]}"
    done <<< "$result"
  fi
done

for hpcli in {hpacucli,hpssacli}
do
  #echo $hpcli
  command="command -v $hpcli"
  #echo $command
  result=$($command 2>&1>/dev/null)
  exitcode=$?
  #echo $exitcode
  if [ $exitcode -ne 0 ]; then
    continue
  fi
  for slot in {0,1,2,3,4,5,6,7,8,9,0a,0b}
  do
    #echo $slot
    controller=""
    device=""
    command="$hpcli controller slot=$slot physicaldrive all show detail"
    #echo $command
    result=$($command 2>/dev/null)
    #exitcode=$?
    #echo $exitcode
    #if [ $exitcode -ne 0 ]; then
    #  continue
    #fi
    while read -r line; do
      #echo $line
      if [[ $line = "" ]]; then
        continue
      fi
      if [[ $controller = "" ]]; then
        controller=$line
        continue
      fi
      if [[ $line = physicaldrive* ]]; then
        device=${line/physicaldrive/}
        echo $device"|"Controller"|"$controller
        continue
      fi
      if [[ $device = "" ]]; then
        continue
      fi
      if [[ $line = *:* ]]; then
        out=`echo $line | awk -F':' '{attribute=$1;value=$2;gsub(/[[:space:]]*/,"",attribute);gsub(/[[:space:]]*/,"",value);printf("%s|%s\n",attribute,value)}'`
        echo $device"|"$out
        continue
      fi
    done <<< "$result"
  done
done