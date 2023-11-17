ps -a | grep "output"

if [ $? -ne 0 ]
then
    ./output
fi
