#kernel string
APPEND_VERSION:=arbiter
MODULES_DIR:=2.6.32$(APPEND_VERSION)

#debian kernel packages
KERN_PACKAGE:=build/linux-image-2.6.32$(APPEND_VERSIION).deb

HEADER_PACKAGE:=build/linux-headers-2.6.32$(APPEND_VERSIION).deb

TARGET_PACKAGES:=$(KERN_PACKAGE) $(HEADER_PACKAGE)

#linux source info
LINUX_DIR:=linux-2.6.32
KERNEL_CONFIG:=$(LINUX_DIR)/.config

#concurrency level of the build, set to be [num of cpu on your machine] + 1
CONCUR_LEVEL:=3

all: $(TARGET_PACKAGES)

$(KERNEL_CONFIG):kernelconfig
	cp kernelconfig $@

build/arbiter-kernel-build-stamp: $(KERNEL_CONFIG)
	rm -fr build
	mkdir -p build
#cp kernelconfig $(KERNEL_CONFIG)
	(cd $(LINUX_DIR); export CONCURRENCY_LEVEL=$(CONCUR_LEVEL); fakeroot make-kpkg --initrd --append-to-version=$(APPEND_VERSION) kernel-image kernel-headers)
	mv linux-image-2.6.32$(APPEND_VERSIION)*.deb $(KERN_PACKAGE)
	mv linux-headers-2.6.32$(APPEND_VERSIION)*.deb $(HEADER_PACKAGE)
	touch $@


$(TARGET_PACKAGES): build/arbiter-kernel-build-stamp
#still need to rebuild since we cannot track source changes, need to find a way to fix this
	rm -f build/arbiter-kernel-build-stamp

install:
#cannot depend on target packages since we force to build everytime
	stat $(TARGET_PACKAGES)
	rm /boot/*arbiter*
	rm -fr /lib/modules/$(MODULES_DIR)
	dpkg -i $(KERN_PACKAGE)
	dpkg -i $(HEADER_PACKAGE)
	update-initramfs -c -k $(MODULES_DIR)
	update-grub

clean:
	rm -fr build


clean-linux:
	(cd linux-2.6.32; make clean)
	(cd linux-2.6.32; make mrproper)
