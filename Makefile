SHELL := /bin/bash

LINUX_VERSION ?= 6.18.33
LINUX_URL := https://cdn.kernel.org/pub/linux/kernel/v6.x/linux-$(LINUX_VERSION).tar.xz

LINUX_SOURCE_DIR := build/linux-src
LINUX_BUILD_DIR := build/linux-build
INITRAMFS_ROOT_DIR := build/initramfs/rootfs
INITRAMFS_IMAGE := build/initramfs/initramfs.cpio.gz
INIT_PROGRAM := build/initramfs/init

ISO_SOURCE_DIR := targets/x86_64/iso
ISO_STAGE_DIR := build/iso/x86_64/iso
DIST_DIR := dist/x86_64
DIST_ISO := $(DIST_DIR)/linux.iso
KERNEL_IMAGE := $(LINUX_BUILD_DIR)/arch/x86/boot/bzImage

.PHONY: build-x86_64 clean

build-x86_64: $(DIST_ISO)

$(LINUX_SOURCE_DIR)/.stamp:
	mkdir -p $(LINUX_SOURCE_DIR)
	if [ ! -f $(LINUX_SOURCE_DIR)/Makefile ]; then \
		curl -fsSL $(LINUX_URL) | tar -xJ --strip-components=1 -C $(LINUX_SOURCE_DIR); \
	fi
	touch $@

$(LINUX_BUILD_DIR)/.config: $(LINUX_SOURCE_DIR)/.stamp
	mkdir -p $(LINUX_BUILD_DIR)
	$(MAKE) -C $(LINUX_SOURCE_DIR) O=$(abspath $(LINUX_BUILD_DIR)) defconfig

$(KERNEL_IMAGE): $(LINUX_BUILD_DIR)/.config
	$(MAKE) -C $(LINUX_SOURCE_DIR) O=$(abspath $(LINUX_BUILD_DIR)) -j$$(nproc) bzImage

$(INIT_PROGRAM): src/initramfs/init.c
	mkdir -p $(dir $@)
	cc -Os -static -nostdlib -fno-pie -no-pie -fno-stack-protector -ffreestanding -Wl,-e,_start -Wl,--build-id=none $< -o $@

$(INITRAMFS_IMAGE): $(INIT_PROGRAM)
	rm -rf $(INITRAMFS_ROOT_DIR)
	mkdir -p $(INITRAMFS_ROOT_DIR)/proc $(INITRAMFS_ROOT_DIR)/sys $(INITRAMFS_ROOT_DIR)/dev $(INITRAMFS_ROOT_DIR)/tmp $(INITRAMFS_ROOT_DIR)/mnt
	cp $(INIT_PROGRAM) $(INITRAMFS_ROOT_DIR)/init
	mkdir -p $(dir $@)
	cd $(INITRAMFS_ROOT_DIR) && find . | cpio -o -H newc | gzip -9 > $(abspath $@)

$(DIST_ISO): $(KERNEL_IMAGE) $(INITRAMFS_IMAGE)
	rm -rf $(ISO_STAGE_DIR)
	mkdir -p $(ISO_STAGE_DIR)
	cp -R $(ISO_SOURCE_DIR)/. $(ISO_STAGE_DIR)/
	mkdir -p $(ISO_STAGE_DIR)/boot
	cp $(KERNEL_IMAGE) $(ISO_STAGE_DIR)/boot/bzImage
	cp $(INITRAMFS_IMAGE) $(ISO_STAGE_DIR)/boot/initramfs.cpio.gz
	mkdir -p $(DIST_DIR)
	command -v mformat >/dev/null 2>&1 || { echo "missing dependency: mformat (install mtools)"; exit 1; }
	grub-mkrescue -o $@ $(ISO_STAGE_DIR)

clean:
	rm -rf build dist
