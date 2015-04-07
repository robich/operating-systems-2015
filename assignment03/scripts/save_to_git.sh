#!/bin/bash

# Test root access
if [ "$(id -u)" != "0" ]; then
   echo -e "[\e[31mError\e[0m] This script must be run as root" 1>&2
   exit 1
fi

# Save working directory
dir=`pwd`

# Save git credentials for 15 mins
git config --global credential.helper cache

echo -e "[\e[94mInfo\e[0m] Copying Kernel..."
sudo cp -r /usr/src/linux/kernel/* /usr/git/operating-systems-2015/assignment03/linux/kernel
# Copy Syscalls Directory
echo -e "[\e[94mInfo\e[0m] Copying Include..."
sudo cp -r /usr/src/linux/include/linux/* /usr/git/operating-systems-2015/assignment03/include/linux

cd /usr/git/operating-systems-2015/

# Save on git
echo -e "[\e[94mInfo\e[0m] Commit to git..."
sudo git add  --all .
sudo git commit
echo -e "[\e[94mInfo\e[0m] Git pull..."
sudo git pull
echo -e "[\e[94mInfo\e[0m] Git push..."

if [ $? -ne 0 ]; then
    echo -e "\e[31mERROR: Git couldn't connect to the url. Exiting.\e[0m"
    exit 1
fi

sudo git push

# Go back to working directory
cd $dir

echo "[\e[32mOK\e[0m]"
