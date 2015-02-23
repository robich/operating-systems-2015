#!/bin/bash

cd /
cd usr/src/linux
echo "Compiling..."
make -j2 bzImage
echo "Copying..."
sudo cp /usr/src/linux/arch/x86/boot/bzImage /boot/vmlinuz-3.18.3+
echo "Making modules..."
make modules
echo "Installing modules..."
sudo make modules_install
echo "Updating kernel..."
sudo update-initramfs -k 3.18.3+ -u
echo "Rebooting..."
sudo reboot
echo "Welcome back!"
exit 0
