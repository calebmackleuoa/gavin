cd /home/caleb/

echo Killing current cluster
killall output

echo Removing old version
sudo rm -R  gavin

echo Cloning update from server
git clone "https://github.com/calebmackleuoa/gavin.git"

cd /home/caleb/gavin/Resources

echo Compiling update
bash compile.sh
sleep 2

clear

echo Update complete
sleep 3

clear
echo Rebooting system 5
sleep 1
echo Rebooting system 4
sleep 1
echo Rebooting system 3
sleep 1
echo Rebooting system 2
sleep 1
echo Rebooting system 1
sleep 1
echo Rebooting...
sleep 2

sudo reboot now
