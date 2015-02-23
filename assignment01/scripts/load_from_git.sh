#!/bin/bash

# Test root access
if [ "$(id -u)" != "0" ]; then
   echo -e "[\e[31mError\e[0m] This script must be run as root" 1>&2
   exit 1
fi

# Save current directory
$dir=${PWD}

# Go to git folder
cd /usr/git/operating-systems-2015

# Pull from git
echo -e "[\e[94mInfo\e[0m] Pulling from git..."
git pull

# Copy Kernel
echo -e "[\e[94mInfo\e[0m] Copying Kernel Directory..."
sudo cp -r /usr/git/operating-systems-2015/assignment01/linux/kernel/* /usr/src/linux/kernel
# Copy Syscalls
echo -e "[\e[94mInfo\e[0m] Copying Syscalls Directory..."
sudo cp -r /usr/git/operating-systems-2015/assignment01/linux/arch/x86/syscalls/* /usr/src/linux/arch/x86/syscalls

# Return into initial directory
cd $dir
