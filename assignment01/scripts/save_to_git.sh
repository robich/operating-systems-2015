#!/bin/bash

# Copy important folders
echo "Copying Kernel..."
cpkerneltogit
echo "Copying Syscalls..."
cpsyscallstogit

cd /usr/git/operating-systems-2015/assignment01/linux

# Git push
echo "Commit to git...
git add  --all .
git commit
echo "Git pull..."
git pull
echo "Git push..."
git push
