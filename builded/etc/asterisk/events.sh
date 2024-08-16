#!/bin/bash

#Stopped Dump dmesg to /var/log/dmesg
#Jun 17 12:33:14 localhost systemd: Stopping Network Time Service...
#Jun 17 12:33:14 localhost rsyslogd: [origin software="rsyslogd" swVersion="7.4.7" x-pid="756" x-info="http://www.rsyslog.c$

#Jun 17 12:36:35 localhost rsyslogd: [origin software="rsyslogd" swVersion="7.4.7" x-pid="751" x-info="http://www.rsyslog.c$
#Jun 17 12:36:12 localhost journal: Runtime journal is using 8.0M (max 382.8M, leaving 574.3M of free 3.7G, current limit 3$
#Jun 17 12:36:12 localhost kernel: Initializing cgroup subsys cpuset
#Jun 15 11:11:01 localhost rsyslogd: [origin software="rsyslogd" swVersion="7.4.7" x-pid="756" x-info="http://www.rsyslog.c$

echo Date"|"Type"|"Message

regexInit='rsyslogd: .* start'
regexStartup='kernel: Initializing cgroup subsys cpuset'
regexShutdown='Stopped Dump dmesg'

regexDate='(Jan|Feb|Mar|Apr|May|Jun|Jul|Aug|Sep|Oct|Nov|Dec) ( ?[1-3]?[0-9]) ([0-2][0-9]):([0-5][0-9]):([0-5][0-9])'
startup=0
year=`date +%Y`

getDate () {
  line=$1
  if [[ $line =~ $regexDate ]]; then
    monthName=${BASH_REMATCH[1]}
    day=${BASH_REMATCH[2]}
    hour=${BASH_REMATCH[3]}
    minute=${BASH_REMATCH[4]}
    second=${BASH_REMATCH[5]}
    month=""

    case $monthName in
      Jan) month=1;;
      Feb) month=2;;
      Mar) month=3;;
      Apr) month=4;;
      May) month=5;;
      Jun) month=6;;
      Jul) month=7;;
      Aug) month=8;;
      Sep) month=9;;
      Oct) month=10;;
      Nov) month=11;;
      Dec) month=12;;
      *) month=1;;
    esac

    echo $year-`printf "%02d" $month`-`printf "%02d" $day` $hour:$minute:$second
  fi
}

for file in `ls /var/log/messages* -r`
do
  #echo $file
  line=""
  tmpline=""
  shutdownline=""
  while read line
  do
    #echo $line
    if [[ $line =~ $regexStartup ]]; then
      #ES UN EVENTO "STARTUP"
      if [ $startup -eq 1 ]; then
        #SEGUIDO DE OTRO EVENTO "STARTUP"
        date=`getDate "$shutdownline"`
        echo $date"|"log"|"$shutdownline
        #echo $shutdownline
      fi
      startup=1
      date=`getDate "$line"`
      echo $date"|"startup
      #echo $line
    elif [[ $line =~ $regexShutdown ]]; then
      startup=0
      date=`getDate "$line"`
      echo $date"|"shutdown      
      #echo $line
    fi
    if [[ $line =~ $regexInit ]]; then
      shutdownline=$tmpline
    fi
    tmpline=$line
  done < $file
done
