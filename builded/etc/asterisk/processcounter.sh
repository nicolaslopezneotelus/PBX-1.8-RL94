#!/bin/bash
# Usage ./processcounter.sh name count
# Fails if number of processes with name (partial match) is greater than count

MAX=$2;
COUNT="$(pgrep -f $1 | wc -l)";

if [ $COUNT -gt $MAX ]; then
  echo "[$(date)] Error: more than $2 instances of $1 are already running" >> /tmp/processcounter.log
  exit 1
fi

