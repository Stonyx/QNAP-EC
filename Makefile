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

# Define names and paths
MODULE_NAME = qnap-ec
MODULE_OBJECT_FILE = qnap-ec.o
MODULE_PATH = /lib/modules/$(shell uname -r)/extra
HELPER_C_FILE = qnap-ec-helper.c
HELPER_BINARY_FILE = qnap-ec
HELPER_PATH = /usr/local/sbin
LIBRARY_SO_FILE = libuLinux_hal.so
LIBRARY_PATH = /usr/local/lib
DEPMOD_COMMAND = $(shell which depmod)
INSTALL_COMMAND = $(shell which install)

# Set compiler flags for the module (ccflags-y is used for both builtin and modules) and for the
#   helper
ccflags-y = -lgcc -O2 -Wall $(EXTRA_CFLAGS) -DDEBUG
CFLAGS = -ldl -O2 -Wall $(EXTRA_FLAGS)

# Check if we've been invoked from the kernel build system and can use its language
ifneq ($(KERNELRELEASE),)
	# Set the module object filename
  obj-m += $(MODULE_OBJECT_FILE)
else
  # Set the kernel directory and working directory
  KDIR = /lib/modules/$(shell uname -r)/build
  PWD = $(shell pwd)
endif

all: clean module helper

module:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

helper: $(HELPER_C_FILE)
	$(CC) -o $(HELPER_BINARY_FILE) $^ $(CFLAGS)

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	$(RM) $(HELPER_BINARY_FILE)

install: module helper
	$(MAKE) -C $(KDIR) M=$(PWD) INSTALL_MOD_DIR=extra/$(MODULE_NAME) modules_install
	$(DEPMOD_COMMAND) -a
	$(INSTALL_COMMAND) $(HELPER_BINARY_FILE) $(HELPER_PATH)
	$(INSTALL_COMMAND) $(LIBRARY_SO_FILE) $(LIBRARY_PATH)

uninstall:
	$(RM) -r $(MODULE_PATH)/$(MODULE_NAME)
	$(DEPMOD_COMMAND) -a
	$(RM) $(HELPER_PATH)/$(HELPER_BINARY_FILE)
	$(RM) $(LIBRARY_PATH)/$(LIBRARY_SO_FILE)
