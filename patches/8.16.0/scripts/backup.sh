#!/bin/bash

if [ "$(pgrep -x $(basename $0))" != "$$" ]; then
  echo "Error: another instance of $(basename $0) is already running"
  exit 1
fi

IFCONFIG="/sbin/ifconfig"
TRANSFER="/etc/asterisk/transferfile.sh"
BACKUP_SUBDIR="SISTEMA"

DELIM=:
ADDR=`$IFCONFIG | grep -i addr: `

if [[ $ADDR = "" ]]; then
  DELIM=t
fi
echo delim = $DELIM
#Get LAN Neotel IP Address
IP=`$IFCONFIG | grep 10.88.8.* | cut -d$DELIM -f2 | awk '{ print $1} ' `
#Get LAN Client IP Address
if [[ $IP = "" ]]; then
  IP=`$IFCONFIG | grep 192.168.* | cut -d$DELIM  -f2 | awk '{ print $1} ' `
fi
if [[ $IP = "" ]]; then
  IP=`$IFCONFIG | grep 172.16.* | cut -d$DELIM  -f2 | awk '{ print $1} ' `
fi
if [[ $IP = "" ]]; then
  IP=`$IFCONFIG | grep 10.* | cut -d$DELIM  -f2 | awk '{ print $1} ' `
fi
if [[ $IP = "" ]]; then
  IP=`$IFCONFIG | grep -i inet | cut -d$DELIM  -f2 | awk '{ print $1} ' `
fi

#Asterisk Configuration
tar -zcvf /$IP.asterisk.configuration.tar.gz /etc/dahdi /etc/asterisk/ /var/lib/asterisk/sounds/ /var/lib/asterisk/moh/ /etc/watcher/ /etc/modprobe.d/
$TRANSFER /$IP.asterisk.configuration.tar.gz $BACKUP_SUBDIR

#Postgres Configuration
pg_dump asterisk > /$IP.postgresql.bk --username postgres
$TRANSFER /$IP.postgresql.bk $BACKUP_SUBDIR

#General Configuration
tar -zcvf /$IP.general.configuration.tar.gz /etc/sysconfig/iptables /etc/sysconfig/network-scripts/ifcfg-* /etc/sysconfig/network-scripts/route-* /etc/rc.d/rc.local /etc/resolv.conf
$TRANSFER /$IP.general.configuration.tar.gz $BACKUP_SUBDIR

#%u: day of week (1..7); 1 represents Monday
if [ $(date +%u) -eq 7 ]; then
  #Asterisk Source Code
  tar -zcvf /$IP.asterisk.sourcecode.tar.gz /usr/src/certified-asterisk-1.8.11-cert2 /usr/src/libpri-1.4.14 /usr/src/openr2-1.3.3 /usr/src/dahdi-linux-2.9.1 /usr/src/watcher /usr/src/*.sh
  $TRANSFER /$IP.asterisk.sourcecode.tar.gz $BACKUP_SUBDIR
fi
