#!/bin/bash

MONITOR_PATH="/var/spool/asterisk/monitor/"
RECORDING_PATH="/var/spool/asterisk/recording/"
TMP_PATH="/var/spool/asterisk/tmp/"
WATCHER_LOG_PATH="/var/log/watcher/"

find $MONITOR_PATH -maxdepth 2 -mtime +90 -type f -exec rm "{}" \;
find $RECORDING_PATH -maxdepth 2 -mtime +90 -type f -exec rm "{}" \;
find $TMP_PATH -maxdepth 2 -mtime +7 -type f -exec rm "{}" \;
find $WATCHER_LOG_PATH -maxdepth 2 -mtime +15 -type f -exec rm "{}" \;
