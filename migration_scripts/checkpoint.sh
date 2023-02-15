#!/bin/bash

#Usage - checkpoint.sh [container name] [file name] [underlying transfer protocol]
#rm -rf checkpoint_ws.tar.gz
rm -rf $2
filename='rank.txt'

start1=`date +%s%6N`
podman container checkpoint -P -e $2 $1
podman container checkpoint --with-previous -e $2 $1 --tcp-established
end1=`date +%s%6N`

if [[ "$3" == "rsync" ]]
then
    while read line; do
        rsync -aqz $2 devi@$line:/home/devi/
        python3 client.py $line
    done < $filename
elif [[ "$3" == "udpcast" ]]
then
    udp-sender --file $2 --nokbd --autostart 1 --mcast-rdv-address 234.0.60.201
    while read line; do
        python3 client.py $line
    done < $filename
else
    echo "Invalid. Choose rsync or udpcast for file transfer"
fi

podman rm $1

#TODO - 
#1. Running parallel operations in bash is not optimal. Find an  alternative
#2. Use the same socket for udpcast and running restore command