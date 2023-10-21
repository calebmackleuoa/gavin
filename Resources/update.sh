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

echo Update complete

echo Booting cluster from update
bash run.sh

