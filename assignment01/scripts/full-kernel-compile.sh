#!/bin/bash
# Compiles and installs the linux kernel, then reboots


cd ../linux
echo -e "\e[32mCompiling...\e[0m"
sudo make -j2 bzImage
echo -e "\e[32mCopying...\e[0m"
sudo cp arch/x86/boot/bzImage /boot/vmlinuz-3.18.3+
echo -e "\e[32mMaking modules...\e[0m"
sudo make modules
echo -e "\e[32mInstalling modules...\e[0m"
sudo make modules_install
echo -e "\e[32mUpdating kernel...\e[0m"
sudo update-initramfs -k 3.18.3+ -u
echo -e "\e[32mRebooting...\e[0m"
sudo reboot
