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
MODULE_O_FILE = qnap-ec.o
MODULE_BINARY_FILE = qnap-ec.ko
MODULE_PATH = $(INSTALL_MOD_PATH)/lib/modules/$(shell uname -r)/extra
HELPER_C_FILE = qnap-ec-helper.c
HELPER_BINARY_FILE = qnap-ec
HELPER_PATH = $(DESTDIR)/usr/local/sbin
LIBRARY_BINARY_FILE = libuLinux_hal.so
LIBRARY_PATH = $(DESTDIR)/usr/local/lib
SIM_LIB_C_FILE = libuLinux_hal-simulated.c
SIM_LIB_BINARY_FILE = libuLinux_hal.so
DEPMOD_COMMAND = $(shell which depmod)
INSTALL_COMMAND = $(shell which install)
MODPROBE_COMMAND = $(shell which modprobe)

# Set compiler flags for the module (ccflags-y is used for both builtin and modules), for the
#   helper, and for the simulated library
ccflags-y = -Wall -O2 -lgcc $(MODULE_EXTRA_CFLAGS)
HELPER_CFLAGS = -Wall -O2 -export-dynamic -ldl $(HELPER_EXTRA_CFLAGS)
SIM_LIB_CFLAGS = -Wall -O2 -fPIC -shared $(SIM_LIB_EXTRA_CLFAGS)

# Check if we've been invoked from the kernel build system and can use its language
ifneq ($(KERNELRELEASE),)
	# Set the module object filename
  obj-m += $(MODULE_O_FILE)
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
	$(INSTALL_COMMAND) $(LIBRARY_BINARY_FILE) $(LIBRARY_PATH)
	$(INSTALL_COMMAND) $(HELPER_BINARY_FILE) $(HELPER_PATH)
	$(MAKE) -C $(KDIR) M=$(PWD) modules_install
	$(DEPMOD_COMMAND) --all
	$(MODPROBE_COMMAND) $(MODULE_NAME)

uninstall:
	-$(MODPROBE_COMMAND) --remove $(MODULE_NAME)
	$(RM) $(MODULE_PATH)/$(MODULE_BINARY_FILE)
	$(DEPMOD_COMMAND) --all
	$(RM) $(HELPER_PATH)/$(HELPER_BINARY_FILE)
	$(RM) $(LIBRARY_PATH)/$(LIBRARY_BINARY_FILE)