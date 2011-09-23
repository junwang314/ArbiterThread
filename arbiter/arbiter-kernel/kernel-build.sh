#!/bin/bash

rm -f linux-image-*.deb
rm -f linux-headers-*.deb

cd linux-2.6.32
cp ../kernelconfig .config
export CONCURRENCY_LEVEL=3
#make-kpkg clean
fakeroot make-kpkg --initrd --append-to-version=arbiter kernel-image kernel-headers
cd ..
sudo dpkg -i linux-image-*.deb
sudo dpkg -i linux-headers-*.deb
sudo update-initramfs -c -k 2.6.32arbiter
sudo update-grub

