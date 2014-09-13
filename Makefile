#====================================================================
#
# Author: Caleb Stewart
# Date: 10 September 2013
# Prupose: A top-level make file to build the OS, toolchain, initrd,
# :		etc...
#
# ===================================================================
# = Change Log                                                      |
# ===================================================================
# =    Date    |    Programmer   |    Description                   |
# ===================================================================
# = 10SEP2013  | Caleb Stewart   | Initial Creation                 |
# =            |                 |                                  |
# ===================================================================
# = 15SEP2013  | Caleb Stewart   | Edited so you have to compile    |
# =            |                 | each project seperately          |
# ===================================================================
#
#====================================================================


# This is where you can add project directories to the source
# kernel should come first here!
PROJECTS:=kernel modules user
ALLPROJECTS:=$(PROJECTS:%=all-%)
CLEANPROJECTS:=$(PROJECTS:%=clean-%)
INSTALLPROJECTS:=$(PROJECTS:%=install-%)
FLASHDRIVE:=/dev/disk/by-uuid/40d036a6-adf3-432a-adfe-13e866269ec2

.PHONY: $(ALLPROJECTS) $(CLEANPROJECTS) $(INSTALLPROJECTS) all clean test install prepare_vhd cleanup_vhd fix_vhd perpare_flashdrive cleanup_flashdrive flashdrive
.PHONY: mount umount

export KERNEL_DIR:=$(abspath ./kernel)
export MODULES_DIR:=$(abspath ./modules)
export STEWIEOS_DIR:=$(abspath ./)
export STEWIEOS_CURRENT:=$(abspath ./stewieos-current)
export STEWIEOS_BIN:=/mnt/bin
export STEWIEOS_OPT:=/mnt/opt
export STEWIEOS_ROOT:=/mnt

all: $(ALLPROJECTS)
#@echo "Please build each subsystem one at time. Here is a list of subsystems to build:\n$(PROJECTS)\nE.g. \"make all-'subsystemname'\""

clean: $(CLEANPROJECTS)

install: prepare_vhd $(INSTALLPROJECTS) cleanup_vhd

mount: prepare_vhd

umount: cleanup_vhd

prepare_vhd:
	losetup /dev/loop0 ./stewieos.vhd
	kpartx -v -a /dev/loop0
	mount /dev/mapper/loop0p1 /mnt
	
cleanup_vhd:
	umount /mnt
	kpartx -v -d /dev/loop0
	losetup -d /dev/loop0
	
flashdrive: prepare_flashdrive $(INSTALLPROJECTS) cleanup_flashdrive
	
prepare_flashdrive:
	mount $(FLASHDRIVE) /mnt

cleanup_flashdrive:
	umount /mnt

#Sometimes cleanup_vhd fails, so we need this to make the fix a little easier
#I don't know why it fails, but in the mean time this works.
fix_vhd:
	kpartx -v -d /dev/loop0
	losetup -d /dev/loop0

$(ALLPROJECTS):
	$(MAKE) -C $(@:all-%=%) all

$(CLEANPROJECTS):
	$(MAKE) -C $(@:clean-%=%) clean
	
$(INSTALLPROJECTS):
	$(MAKE) -C $(@:install-%=%) install

# Here, you can add dependencies for specific projects
# vhd: kernel
