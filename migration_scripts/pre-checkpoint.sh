#!/bin/bash

#Usage - pre-checkpoint.sh [container name] [file name] [underlying transfer protocol]
#rm -rf pre-checkpoint_ws.tar.gz
rm -rf $2
filename='rank.txt'

start1=`date +%s%6N`
podman container checkpoint -P -e $2 $1
end1=`date +%s%6N`

#echo pre-dump run time is `expr $end1 - $start1` microseconds

if [[ "$3" == "rsync" ]]
then
    while read line; do
        rsync -aqz $2 devi@$line:/home/devi/ &
    done < $filename
elif [[ "$3" == "udpcast" ]]
then
    udp-sender --file $2 --nokbd --autostart 1 --mcast-rdv-address 234.0.60.201
else
    echo "Invalid. Choose rsync or udpcast for file transfer"
fi

#TODO - 
#1. Running parallel operations in bash is not optimal. Find an  alternative
#2. Decide when to run pre-checkpoint and using which rank file
                                                                   