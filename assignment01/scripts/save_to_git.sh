#!/bin/bash

# Save working directory
$dir=${PWD}

# Copy Kernel Directory
echo "[\e[94mInfo\e[0m] Copying Kernel..."
sudo cp -r /usr/src/linux/kernel/* /usr/git/operating-systems-2015/assignment01/linux/kernel
# Copy Syscalls Directory
echo "[\e[94mInfo\e[0m] Copying Syscalls..."
sudo cp -r /usr/src/linux/arch/x86/syscalls* /usr/git/operating-systems-2015/assignment01/linux/arch/x86/syscalls

cd /usr/git/operating-systems-2015/assignment01/linux

# Save on git
echo "[\e[94mInfo\e[0m] Commit to git..."
git add  --all .
git commit
echo "[\e[94mInfo\e[0m] Git pull..."
git pull
echo "[\e[94mInfo\e[0m] Git push..."
git push

# Go back to working directory
cd $dir

echo "[\e[32mOK\e[0m]"
