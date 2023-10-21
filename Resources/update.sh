cd /home/caleb/gavin/

echo Killing current cluster
killall output

echo Pulling update from server
git pull

cd Resources

echo Compiling update
bash compile.sh

echo Update complete

echo Booting cluster from update
bash run.sh

