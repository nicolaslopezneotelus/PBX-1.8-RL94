#!/bin/bash

if [ "$(pgrep -x $(basename $0))" != "$$" ]; then
  echo "Error: another instance of $(basename $0) is already running"
  exit 1
fi

MONITOR_PATH="/var/spool/asterisk/monitor/"
MONITOR_PATH_RAM="/mnt/ramdisk/"
TRANSFER="/etc/asterisk/transferfile.sh"

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
    #Move files that was last changed 60 minutes ago.
    find $MONITOR_PATH_RAM$MONITOR_RAM_SUBDIR -maxdepth 1 -cmin +60 ! -name 'ONDE*' -type f -exec mv {} $MONITOR_PATH$MONITOR_RAM_SUBDIR \;

    #MONITOR_RAM_FILES=$(find $MONITOR_PATH_RAM$MONITOR_RAM_SUBDIR -maxdepth 1 -cmin +60 ! -name 'ONDE*' -type f -exec basename {} \;)
    #for MONITOR_RAM_FILE in $MONITOR_RAM_FILES;
    #do
    #  echo $MONITOR_RAM_FILE
    #  mv $MONITOR_RAM_PATH$MONITOR_RAM_SUBDIR/$MONITOR_FILE $MONITOR_PATH$MONITOR_RAM_SUBDIR/$MONITOR_FILE
    #done

    #Move files that was last changed 120 minutes ago.
    find $MONITOR_PATH_RAM$MONITOR_RAM_SUBDIR -maxdepth 1 -cmin +120 -name 'ONDE*' -type f -exec mv {} $MONITOR_PATH$MONITOR_RAM_SUBDIR \;
  done
fi

echo $MONITOR_PATH
#Create a list of subdirectories names.
MONITOR_SUBDIRS=$(find $MONITOR_PATH -maxdepth 1 -mindepth 1 -type d -exec basename {} \;)

for MONITOR_SUBDIR in $MONITOR_SUBDIRS;
do
  echo $MONITOR_PATH$MONITOR_SUBDIR
  #Transfer files that was last changed 60 minutes ago.
  MONITOR_FILES=$(find $MONITOR_PATH$MONITOR_SUBDIR -maxdepth 1 -cmin +60 ! -name 'ONDE*' -type f -exec basename {} \;)
  for MONITOR_FILE in $MONITOR_FILES;
  do
    echo $MONITOR_FILE
    $TRANSFER $MONITOR_PATH$MONITOR_SUBDIR/$MONITOR_FILE $MONITOR_SUBDIR/upload
  done
  #Transfer files that was last changed 120 minutes ago.
  MONITOR_FILES=$(find $MONITOR_PATH$MONITOR_SUBDIR -maxdepth 1 -cmin +120 -name 'ONDE*' -type f -exec basename {} \;)
  for MONITOR_FILE in $MONITOR_FILES;
  do
    echo $MONITOR_FILE
    $TRANSFER $MONITOR_PATH$MONITOR_SUBDIR/$MONITOR_FILE $MONITOR_SUBDIR/upload
  done
done