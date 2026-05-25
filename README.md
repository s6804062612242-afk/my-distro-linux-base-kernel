# my-os

Minimal Linux boot ISO example.

Prerequisites
- gcc, make, curl, tar
- grub-mkrescue, xorriso, mtools (for building ISO)
- qemu-system-x86_64 (for testing)
- Linux kernel build deps: bc, bison, flex, libssl-dev, libelf-dev, libncurses-dev

Build

Run
- make build-x86_64

Run (example)
- qemu-system-x86_64 -cdrom dist/x86_64/linux.iso

Notes
- The build downloads Linux source on first run, builds a bzImage, and packages a tiny initramfs so the ISO boots into Linux without a disk image.
- GRUB offers normal, verbose, and debug entries. Debug enables extra kernel logging and serial console output.
- The initramfs boots into bash and provides a `c` command that runs a sample static C program.
- The old custom-kernel sources remain in the tree for reference, but they are no longer part of the build.
