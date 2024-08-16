#!/bin/bash

#Mar  1 15:42:13 NEO1 kernel: wcte12xp: Setting yellow alarm
#Apr 17 12:27:10 localhost kernel: wct2xxp: Setting yellow alarm on span 2
#May  6 13:01:21 NEO1 kernel: wct4xxp 0000:06:00.0: Setting yellow alarm span 3

echo Date"|"Driver"|"Span

regexEvent='kernel: ([a-z0-9]*)( 0000:..:....)?: Setting yellow alarm( (on )?span ([1-9]))?'
regexDate='(Jan|Feb|Mar|Apr|May|Jun|Jul|Aug|Sep|Oct|Nov|Dec) ( ?[1-3]?[0-9]) ([0-2][0-9]):([0-5][0-9]):([0-5][0-9])'

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
  while read line
  do
    #echo $line
    if [[ $line =~ $regexEvent ]]; then
      #ES UN EVENTO "ALARM"
      driver=${BASH_REMATCH[1]}
      span=${BASH_REMATCH[5]}
      date=`getDate "$line"`
      echo $date"|"$driver"|"$span
    fi
  done < $file
done
