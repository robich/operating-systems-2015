#!/bin/bash

if [ "$(id -u)" != "0" ]; then
   echo -e "[\e[31mError\e[0m] This script must be run as root." 1>&2
   exit 1
fi

if [ "$#" -ne 1 ]; then
    echo "This script takes exactly one argument." 1>&2
    exit 1
fi

dir=`pwd`/$1
sudo mkdir $dir

if [ $? -ne 0 ]; then
    echo "Cannot create dir"
fi

sudo ./vfat -s -f -odirect_io /mnt/hgfs/shared/testfs/./testfs.fat $dir
