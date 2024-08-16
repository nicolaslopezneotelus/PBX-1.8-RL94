rpm -Uvh --force /usr/src/PBX-1.8-RL94/packages/x86_64/5.14.0-427.13.1.el9_4.x86_64/dos2unix-7.4.2-4.el9.x86_64.rpm

cd /usr/src/PBX-1.8-RL94/scripts
dos2unix *.sh
chmod 755 *.sh

./update.sh
reboot

cd /usr/src/PBX-1.8-RL94/scripts
./asterisk_install.sh
./patch_8.16.0.sh
./patch_8.16.1.sh
#./patch_8.16.2.sh no sirve
./patch_8.16.3.sh
./patch_8.16.4.sh