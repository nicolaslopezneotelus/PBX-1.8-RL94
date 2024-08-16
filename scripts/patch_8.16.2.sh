#!/bin/bash
SOURCEDIR=/usr/src

#Apply asterisk database patches
echo "Applying asterisk database patches..."
cd $SOURCEDIR/PBX-1.8-RL94/patches/8.16.3/asterisk/database/
dos2unix database.sh
su postgres ./database.sh

cp -f /etc/asterisk/sip.conf /etc/asterisk/sip.conf.bak
sed -e "s/#tcpblindaddr=0.0.0.0:5060//" /etc/asterisk/sip.conf > /etc/asterisk/sip.conf.tmp && mv -f /etc/asterisk/sip.conf.tmp /etc/asterisk/sip.conf
echo "#tcpblindaddr=0.0.0.0:5060" >> /etc/asterisk/sip.conf
sed -e "s/#tcpenabled=yes//" /etc/asterisk/sip.conf > /etc/asterisk/sip.conf.tmp && mv -f /etc/asterisk/sip.conf.tmp /etc/asterisk/sip.conf
echo "#tcpenabled=yes" >> /etc/asterisk/sip.conf

#Copy configuration files
echo "Copying configuration files..."
cp -fr $SOURCEDIR/PBX-1.8-RL94/patches/8.16.3/scripts/* /etc/asterisk/

#Install watcher
echo "Installing watcher..."
cd $SOURCEDIR/PBX-1.8-RL94/patches/8.16.3/watcher/src/
gcc -D HAVE_INOTIFY -I/usr/include/inotifytools -I/usr/include/curl -O0 -g3 -Wall -lpthread -linotifytools -lcurl watcher.c list.c logger.c thpool.c -o watcher
chmod 755 watcher
service watcher stop
cp -fr watcher /usr/sbin/
cd $SOURCEDIR/PBX-1.8-RL94/patches/8.16.3/watcher/
dos2unix watcher
chmod 755 watcher
cp -fr watcher /etc/init.d/
if [ ! -d /etc/watcher ]; then
  mkdir /etc/watcher
fi
dos2unix watcher.conf
cp -fr watcher.conf /etc/watcher/
systemctl enable watcher #chkconfig watcher on
service watcher restart