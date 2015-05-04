#!/bin/bash

if [ "$(id -u)" != "0" ]; then
   echo -e "[\e[31mError\e[0m] This script must be run as root" 1>&2
   exit 1
fi

sudo mkdir $1
sudo ./vfat -s -f -odirect_io /mnt/hgfs/shared/testfs/./testfs.fat $1
