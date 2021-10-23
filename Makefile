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

# Define names, paths, and commands
MODULE_NAME = qnap-ec
MODULE_OBJECT_FILE = qnap-ec.o
MODULE_PATH = /lib/modules/$(shell uname -r)/extra/$(MODULE_NAME)
HELPER_C_FILE = qnap-ec-helper.c
HELPER_BINARY_FILE = qnap-ec
HELPER_PATH = /usr/local/sbin
LIBRARY_SO_FILE = libuLinux_hal.so
LIBRARY_PATH = /usr/local/lib
SIM_LIB_C_FILE = libuLinux_hal-simulated.c
SIM_LIB_BINARY_FILE = libuLinux_hal.so
DEPMOD_COMMAND = $(shell which depmod)
INSTALL_COMMAND = $(shell which install)
MODPROBE_COMMAND = $(shell which modprobe)

# Set compiler flags for the module (ccflags-y is used for both builtin and modules), for the
#   helper, and for the simulated library
ccflags-y = -Wall -O2 -lgcc $(EXTRA_MODULE_CFLAGS)
HELPER_CFLAGS = -Wall -O2 -ldl $(EXTRA_HELPER_CFLAGS)
SIM_LIB_CFLAGS = -Wall -shared -fPIC -O2 $(EXTRA_SIM_LIB_CLFAGS)

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
	$(CC) -o $(HELPER_BINARY_FILE) $^ $(HELPER_CFLAGS)

sim-lib: $(SIM_LIB_C_FILE)
	$(CC) -o $(SIM_LIB_BINARY_FILE) $^ $(SIM_LIB_CFLAGS)

clean:
	$(RM) $(HELPER_BINARY_FILE)
	$(MAKE) -C $(KDIR) M=$(PWD) clean

install: module helper
	$(INSTALL_COMMAND) $(LIBRARY_SO_FILE) $(LIBRARY_PATH)
	$(INSTALL_COMMAND) $(HELPER_BINARY_FILE) $(HELPER_PATH)
	$(MAKE) -C $(KDIR) M=$(PWD) INSTALL_MOD_DIR=extra/$(MODULE_NAME) modules_install
	$(DEPMOD_COMMAND) --all
	$(MODPROBE_COMMAND) $(MODULE_NAME)

uninstall:
	-$(MODPROBE_COMMAND) --remove $(MODULE_NAME)
	$(RM) --recursive $(MODULE_PATH)
	$(DEPMOD_COMMAND) --all
	$(RM) $(HELPER_PATH)/$(HELPER_BINARY_FILE)
	$(RM) $(LIBRARY_PATH)/$(LIBRARY_SO_FILE)