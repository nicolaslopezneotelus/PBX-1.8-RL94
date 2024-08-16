rpm -e corosync corosynclib corosynclib-devel

cd /usr/src
unzip corosync-1.4.8.zip
cd corosync-1.4.8/
./autogen.sh
./configure
make
make install
#chkconfig corosync on
#corosync-cfgtool -s