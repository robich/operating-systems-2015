#!/bin/bash

# Test root access
if [ "$(id -u)" != "0" ]; then
   echo -e "[\e[31mError\e[0m] This script must be run as root" 1>&2
   exit 1
fi

# Save working directory
$dir=${PWD}

# Copy Kernel Directory
echo -e "[\e[94mInfo\e[0m] Copying Kernel..."
sudo cp -r /usr/src/linux/kernel/* /usr/git/operating-systems-2015/assignment01/linux/kernel
# Copy Syscalls Directory
echo -e "[\e[94mInfo\e[0m] Copying Syscalls..."
sudo cp -r /usr/src/linux/arch/x86/syscalls/* /usr/git/operating-systems-2015/assignment01/linux/arch/x86/syscalls
# Copy Include Directory
echo -e "[\e[94mInfo\e[0m] Copying Include..."
sudo cp -r /usr/src/linux/include/* /usr/git/operating-systems-2015/assignment01/linux/include

cd /usr/git/operating-systems-2015/

# Save on git
echo -e "[\e[94mInfo\e[0m] Commit to git..."
git add  --all .
git commit
echo -e "[\e[94mInfo\e[0m] Git pull..."
git pull
echo -e "[\e[94mInfo\e[0m] Git push..."
git push

# Go back to working directory
cd $dir

echo "[\e[32mOK\e[0m]"
