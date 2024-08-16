#!/bin/bash
SOURCEDIR=/usr/src
clear

#Install kermit from source
echo "Installing kermit..."
cd $SOURCEDIR/PBX-1.8-RL94/patches/8.16.0/cku302
make linux
cp wermit /usr/local/bin/kermit

#Install asterisk database
echo "Installing asterisk database..."
cd $SOURCEDIR/PBX-1.8-RL94/patches/8.16.0/asterisk/database/
dos2unix database.sh
su postgres ./database.sh

echo "Configuring odbc..."
cp -f $SOURCEDIR/PBX-1.8-RL94/patches/8.16.0/odbc/odbc.ini /etc/odbc.ini
cp -f $SOURCEDIR/PBX-1.8-RL94/patches/8.16.0/odbc/odbcinst.ini /etc/odbcinst.ini
systemctl restart postgresql

echo "Validating odbc..."
odbcinst -q -d
echo "select 1" | isql -v asterisk

#Install watcher
echo "Installing watcher..."
cd $SOURCEDIR/PBX-1.8-RL94/patches/8.16.0/watcher/src/
gcc -D HAVE_INOTIFY -I/usr/include/inotifytools -I/usr/include/curl -O0 -g3 -Wall -lpthread -linotifytools -lcurl watcher.c logger.c thpool.c -o watcher
chmod 755 watcher
cp -fr watcher /usr/sbin/
cd $SOURCEDIR/PBX-1.8-RL94/patches/8.16.0/watcher/
dos2unix watcher
chmod 755 watcher
cp watcher /etc/init.d/watcher
chkconfig --add /etc/init.d/watcher
chkconfig watcher on
if [ ! -d /etc/watcher ]; then
  mkdir /etc/watcher
fi
dos2unix watcher.conf
cp -fr watcher.conf /etc/watcher/

# Create systemd service file for watcher
echo "Creating systemd service file for watcher..."
cat << EOF > /etc/systemd/system/watcher.service
[Unit]
Documentation=man:systemd-sysv-generator(8)
SourcePath=/etc/rc.d/init.d/watcher
Description=SYSV: watcher is the recordings transfer daemon. The Watcher is used to transfer the recordings made by the astersik PBX to the FTP server.
Before=multi-user.target
Before=multi-user.target
Before=multi-user.target
Before=graphical.target
Before=asterisk.service
After=dahdi.service

[Service]
Type=forking
Restart=no
TimeoutSec=5min
IgnoreSIGPIPE=no
KillMode=process
GuessMainPID=no
RemainAfterExit=yes
ExecStart=/etc/rc.d/init.d/watcher start
ExecStop=/etc/rc.d/init.d/watcher stop

[Install]
WantedBy=multi-user.target
EOF

# Reload systemd to recognize the new service and enable it
systemctl daemon-reload
systemctl enable watcher
systemctl start watcher

echo "Watcher service installed and started successfully."
