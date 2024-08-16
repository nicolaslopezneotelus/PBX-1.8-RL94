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
  cd $SOURCEDIR/PBX-1.8-RL94/packages/x86_64/5.14.0-427.13.1.el9_4.x86_64/

  #Install software dependencies
  echo "Installing software dependencies..."
  #yum install lm_sensors
  exec rpm -Uvh --force lm_sensors-3.6.0-10.el9.x86_64.rpm \
                        dmidecode-3.5-3.el9.x86_64.rpm
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

#Apply asterisk database patches
echo "Applying asterisk database patches..."
cd $SOURCEDIR/PBX-1.8-RL94/patches/8.16.1/asterisk/database/
dos2unix database.sh
su postgres ./database.sh

#Install watcher
echo "Installing watcher..."
cd $SOURCEDIR/PBX-1.8-RL94/patches/8.16.1/watcher/src/
gcc -D HAVE_INOTIFY -I/usr/include/inotifytools -I/usr/include/curl -O0 -g3 -Wall -lpthread -linotifytools -lcurl watcher.c logger.c thpool.c -o watcher
chmod 755 watcher
cp -fr watcher /usr/sbin/
cd $SOURCEDIR/PBX-1.8-RL94/patches/8.16.1/watcher/
dos2unix watcher
chmod 755 watcher
cp -fr watcher /etc/init.d/
if [ ! -d /etc/watcher ]; then
  mkdir /etc/watcher
fi
dos2unix watcher.conf
cp -fr watcher.conf /etc/watcher/
systemctl enable watcher #chkconfig watcher on





