cd /home/caleb/

if [ $? -eq 0 ]
then
    echo Killing current cluster
    killall output
fi

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
echo Rebooting system in 5
sleep 1

clear
echo Rebooting system in 4
sleep 1

clear
echo Rebooting system in 3
sleep 1

clear
echo Rebooting system in 2
sleep 1

clear
echo Rebooting system in 1
sleep 1

clear
echo Rebooting...
sleep 2

sudo reboot now
