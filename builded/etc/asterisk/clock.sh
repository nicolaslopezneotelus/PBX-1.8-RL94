#!/bin/bash

echo Time"|"Uptime

time=`date +"%Y-%m-%d %H:%M:%S"`
uptime=`awk '{printf("%d\n",$1/(24*60*60))}' /proc/uptime`

echo $time"|"$uptime
