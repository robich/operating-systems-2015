#!/bin/bash

# Copy important folders into git folder

# cpkerneltogit
echo "Copying Kernel..."
sudo cp -r /usr/src/linux/kernel/* /usr/git/operating-systems-2015/assignment01/linux/kernel
# cpsyscallstogit
echo "Copying Syscalls..."
sudo cp -r /usr/src/linux/arch/x86/syscalls* /usr/git/operating-systems-2015/assignment01/linux/arch/x86/syscalls

cd /usr/git/operating-systems-2015/assignment01/linux

# Save on git
echo "Commit to git..."
git add  --all .
git commit
echo "Git pull..."
git pull
echo "Git push..."
git push
