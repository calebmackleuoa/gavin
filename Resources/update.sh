echo Killing current cluster
killall output

echo Pulling update from server
git pull

echo Compiling update
bash compile.sh

echo Update complete

echo Booting cluster from update
bash run.sh

