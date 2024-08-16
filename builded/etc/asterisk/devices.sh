#!/bin/bash

#[1]
#active=yes
#alarms=UNCONFIGURED
#description=T4XXP (PCI) Card 0 Span 1
#name=TE4 / 0 / 1
#manufacturer=Digium
#devicetype=Wildcard TE420 (5th Gen)
#location=Board ID Switch 0
#basechan=1
#totchans=31
#irq=209
#type=digital - E1
#syncsrc=0
#lbo=0 db (CSU)/0-133 feet (DSX-1)
#coding_opts=HDB3
#framing_opts=CCS,CRC4
#coding=
#framing=

echo ID"|"Attribute"|"Value

regexID="\[([0-9]+)\]"

dahdi_scan | while read line;
do
  #echo $line
  if [[ $line =~ $regexID ]]; then
    id=${BASH_REMATCH[1]}
    continue
  fi
  IFS='=' read -ra words <<< "$line"
  attribute=${words[0]}
  value=${words[1]}
  echo $id"|"$attribute"|"$value
done
