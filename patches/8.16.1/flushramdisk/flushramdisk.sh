#!/bin/bash

if [ "$(pgrep -x $(basename $0))" != "$$" ]; then
  echo "Error: another instance of $(basename $0) is already running"
  exit 1
fi

MONITOR_PATH="/var/spool/asterisk/monitor/"
MONITOR_PATH_RAM="/mnt/ramdisk/"

#Check if ramdisk exists.
if [ -d $MONITOR_PATH_RAM ]; then
  echo $MONITOR_PATH_RAM
  #Create a list of subdirectories names.
  MONITOR_RAM_SUBDIRS=$(find $MONITOR_PATH_RAM -maxdepth 1 -mindepth 1 -type d -exec basename {} \;)

  for MONITOR_RAM_SUBDIR in $MONITOR_RAM_SUBDIRS;
  do
    echo $MONITOR_PATH_RAM$MONITOR_RAM_SUBDIR
    #Check if subdirectory exists on harddrive, if not create it.
    if [ ! -d $MONITOR_PATH$MONITOR_RAM_SUBDIR ]; then
      mkdir $MONITOR_PATH$MONITOR_RAM_SUBDIR
    fi
    #Move files.
    find $MONITOR_PATH_RAM$MONITOR_RAM_SUBDIR -maxdepth 1 -type f -exec mv {} $MONITOR_PATH$MONITOR_RAM_SUBDIR \;

    #MONITOR_RAM_FILES=$(find $MONITOR_PATH_RAM$MONITOR_RAM_SUBDIR -maxdepth 1 -type f -exec basename {} \;)
    #for MONITOR_RAM_FILE in $MONITOR_RAM_FILES;
    #do
    #  echo $MONITOR_RAM_FILE
    #  mv $MONITOR_RAM_PATH$MONITOR_RAM_SUBDIR/$MONITOR_FILE $MONITOR_PATH$MONITOR_RAM_SUBDIR/$MONITOR_FILE
    #done
  done
fi
