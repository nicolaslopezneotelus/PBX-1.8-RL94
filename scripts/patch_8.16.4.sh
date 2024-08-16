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

if [[ $RELEASE = "rocky-release-9.4-1.5.el9.noarch" ]]; then
  cd $SOURCEDIR/PBX-1.8-RL94/packages/x86_64/5.14.0-427.13.1.el9_4.x86_64

  #yum install hyperv-daemons
  exec rpm -Uvh --force hypervkvpd-0-0.42.20190303git.el9.x86_64.rpm \
                        hypervfcopyd-0-0.42.20190303git.el9.x86_64.rpm \
                        hypervvssd-0-0.42.20190303git.el9.x86_64.rpm   \
                        hyperv-daemons-0-0.42.20190303git.el9.x86_64.rpm \
                        hyperv-daemons-license-0-0.42.20190303git.el9.noarch.rpm

  #Copy configuration files
  echo "Copying configuration files..."
  cp -fr $SOURCEDIR/PBX-1.8-RL94/patches/8.16.4/asterisk/configs/* /etc/asterisk/
  dos2unix /etc/asterisk/*
  echo
else
  echo "Release $RELEASE not supported"
  exit
fi

#reboot
