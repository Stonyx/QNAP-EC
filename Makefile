# Copyright (C) 2021 Stonyx
# https://www.stonyx.com/
#
# This driver is free software. You can redistribute it and/or modify it under the terms of the GNU
# General Public License Version 3 (or at your option any later version) as published by The Free
# Software Foundation.
#
# This driver is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
# even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
# General Public License for more details.
#
# If you did not received a copy of the GNU General Public License along with this script see
# http://www.gnu.org/copyleft/gpl.html or write to The Free Software Foundation, 675 Mass Ave,
# Cambridge, MA 02139, USA.


# Define filenames, paths, and commands
# Note: we are using simply expanded variables exclusively to make things more predictable
LIBRARY_BINARY_FILE := libuLinux_hal.so
LIBRARY_COMPILED_PATH := /usr/local/lib
LIBRARY_PACKAGED_PATH := /usr/lib
HELPER_C_FILE := qnap-ec-helper.c
HELPER_BINARY_FILE := qnap-ec
HELPER_COMPILED_PATH := /usr/local/sbin
HELPER_PACKAGED_PATH := /usr/sbin
MODULE_NAME := qnap-ec
MODULE_O_FILE := qnap-ec.o
MODULE_BINARY_FILE := qnap-ec.ko
MODULE_PATH := /lib/modules/$(shell uname -r)/extra
SIM_LIB_C_FILE := libuLinux_hal-simulated.c
SIM_LIB_BINARY_FILE := libuLinux_hal.so
SLACK_DESC_FILE := slack-desc
SLACK_DESC_PATH := /install
DEPMOD_COMMAND := $(shell which depmod)
INSTALL_COMMAND := $(shell which install)
MODPROBE_COMMAND := $(shell which modprobe)

# Set the helper compiler flags, add target specific flags, and add any extra flags
HELPER_CFLAGS := -Wall -O2 -export-dynamic -ldl
package : HELPER_CFLAGS += -DPACKAGE
HELPER_CFLAGS += $(HELPER_EXTRA_CFLAGS)

# Set the simulated library compiler flags
SIM_LIB_CFLAGS := -Wall -O2 -fPIC -shared $(SIM_LIB_EXTRA_CLFAGS)

# Check if the KERNELRELEASE variable is not defined
# Note: if KERNELRELEASE is not defined we are in this make file for the first time as part of the
#       make process and if it is been defined we are in this make file for the second time as part
#       of the kbuild process
ifndef KERNELRELEASE
  # Set the kernel build directory, working directory, and target specific module compiler flags
  # Note: target specific variables need to be set in this section of the if statement since
  #       the target is not known after the kbuild process is invoked
  KDIR := /lib/modules/$(shell uname -r)/build
  PWD := $(shell pwd)
  package : MODULE_CFLAGS := -DPACKAGE
else
	# Set the module object filename
  obj-m := $(MODULE_O_FILE)

  # Set the module compiler flags using any passed in flags from the first run through this make
  #   file as part of the make process and add any extra flags
  ccflags-y := -Wall -O2 -lgcc $(MODULE_CFLAGS) $(MODULE_EXTRA_CFLAGS)
endif

# Define the all target
all: clean helper module

# Define the clean target
clean:
	$(RM) $(HELPER_BINARY_FILE)
	$(MAKE) -C $(KDIR) M=$(PWD) clean

# Define the helper target
helper:
	$(CC) -o $(HELPER_BINARY_FILE) $(HELPER_C_FILE) $(HELPER_CFLAGS)

# Define the module target
module:
	$(MAKE) -C $(KDIR) M=$(PWD) MODULE_CFLAGS=$(MODULE_CFLAGS) modules

# Define the install target
install: clean helper module
	$(INSTALL_COMMAND) --owner=root --group=root --mode=644 $(LIBRARY_BINARY_FILE) \
	  $(LIBRARY_COMPILED_PATH)/$(LIBRARY_BINARY_FILE)
	$(INSTALL_COMMAND) --strip --owner=root --group=root --mode=755 $(HELPER_BINARY_FILE) \
	  $(HELPER_COMPILED_PATH)/$(HELPER_BINARY_FILE)
	$(MAKE) -C $(KDIR) M=$(PWD) modules_install
	$(DEPMOD_COMMAND) --quick
	-$(MODPROBE_COMMAND) $(MODULE_NAME)

# Define the package target
package: clean helper module
	$(INSTALL_COMMAND) --owner=root --group=root --mode=644 -D $(LIBRARY_BINARY_FILE) \
	  $(DESTDIR)$(LIBRARY_PACKAGED_PATH)/$(LIBRARY_BINARY_FILE)
	$(INSTALL_COMMAND) --strip --owner=root --group=root --mode=755 -D $(HELPER_BINARY_FILE) \
	  $(DESTDIR)$(HELPER_PACKAGED_PATH)/$(HELPER_BINARY_FILE)
	$(MAKE) -C $(KDIR) M=$(PWD) INSTALL_MOD_PATH=$(DESTDIR) modules_install
ifdef SLACKWARE
	$(INSTALL_COMMAND) --owner=root --group=root --mode=644 -D $(SLACK_DESC_FILE) \
	  $(DESTDIR)$(SLACK_DESC_PATH)/$(SLACK_DESC_FILE)
endif

# Define the uninstall target
uninstall:
	-$(MODPROBE_COMMAND) --remove $(MODULE_NAME)
	$(RM) $(MODULE_PATH)/$(MODULE_BINARY_FILE)
	$(DEPMOD_COMMAND) --all
	$(RM) $(HELPER_COMPILED_PATH)/$(HELPER_BINARY_FILE)
	$(RM) $(LIBRARY_COMPILED_PATH)/$(LIBRARY_BINARY_FILE)

# Define the sim-lib target
sim-lib:
	$(CC) -o $(SIM_LIB_BINARY_FILE) $(SIM_LIB_C_FILE) $(SIM_LIB_CFLAGS)