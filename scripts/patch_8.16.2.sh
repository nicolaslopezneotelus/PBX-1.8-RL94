#!/bin/bash
SOURCEDIR=/usr/src
RELEASE=`rpm -q rocky-release`

function exec {
  "$@"
  local EXIT_CODE=$?
  if [ $EXIT_CODE -ne 0 ]; then
    echo "$* failed with exit code $EXIT_CODE." >&2
    exit
  fi
}

clear

#Modify the file permissions
chmod -R 755 $SOURCEDIR/*

if [[ $RELEASE = "rocky-release-5.14.0-427.13.1.el9_4.x86_64.noarch" ]]; then
  cd $SOURCEDIR/PBX-1.8-RL94/packages/x86_64/5.14.0-427.13.1.el9_4.x86_64/

  #Install software dependencies
  echo "Installing software dependencies..."
  #yum install lm_sensors
  exec rpm -Uvh --force lm_sensors-3.4.0-23.20180522git70f7e08.el8.x86_64.rpm 
else
  echo "Release $RELEASE not supported"
  exit
fi

#Configure sensors
echo "Configuring sensors..."
YES "" | sensors-detect

#Configure scheduled tasks
echo "Configuring scheduled tasks..."
cp -f $SOURCEDIR/PBX-1.8-RL94/patches/8.16.1/cron/root /var/spool/cron/root
chmod 644 /var/spool/cron/root
service crond restart