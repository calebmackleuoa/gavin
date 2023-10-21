ps -a | grep "output"

if [ $? -ne 0 ]
then
    cd /home/caleb/gavin/Resources/v3
    ./output
fi
