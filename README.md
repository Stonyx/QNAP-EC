# QNAP-EC - HWMon Driver for QNAP IT8528 E.C. Chips

A Linux hwmon driver kernel module for the QNAP IT8528 Embedded Controller chip (and possibly others).  This driver supports reading the fan speeds and temperatures as well as reading and writing the fan P.W.M. values from the ITE Tech Inc. IT8528 embedded controller chip that is used in many QNAP NAS models.  Because the IT8528 chip can run custom firmware this driver is most likely specific to the firmware that QNAP uses on these chips.  It is based on the reverse engineering knowledge originally gathered by [guedou](https://github.com/guedou) with lots of operational and testing help provided by [r-pufky](https://github.com/r-pufky).

In order to provide the greatest compatibility, this driver uses a library that is supplied by QNAP in it's NAS operating system.  The libuLinux_hal library that is part of this repository was taken from a QNAP-TS873A model.  In order to ensure proper functionality, you should replace the libuLinux_hal.so library file with one from the operating system image for the exact QNAP NAS model you will be running this driver on.  Because this driver uses the QNAP library it is conceivable that it will work with other chips used by QNAP that are supported by the libuLinux_hal library.

Using a vanilla Ubuntu 20.04.2.0 Live DVD system as an example, you can build this driver by running the following commands:
```
sudo apt install build-essential git
git clone https://github.com/Stonyx/QNAP-EC
cd QNAP-EC
sudo make install
```
This will compile, link, and install the needed files along with inserting the module into the kernel (it uses modprobe to insert the module into the kernel which will NOT persist after a reboot).

If you would like the kernel module to skip checking for the presence of the IT8528 chip (for example to run it on a QNAP NAS unit with a different chip to see if this driver will work) run the following to perform the installation:
```
sudo make install EXTRA_MODULE_CFLAGS=-DSKIP_CHECK
```

For development purposes there is a simulated libuLinux_hal library included that can be used when developing on a machine that doesn’t have a compatible embedded controller chip.  To build the simulated library run:
```
make sim-lib
```
This will replace the libuLinux_hal library with the simulated library so that running `sudo make install EXTRA_MODULE_CFLAGS=-DSKIP_CHECK` will install the simulated library (along with not checking for the presence of the IT8528 chip).

To uninstall the module completely run:
```
sudo make uninstall
```

This driver has three components, the kernel module file called `qnap-ec.ko` which would be installed in the `/lib/modules/5.8.0-43-generic/extra/qnap-ec` directory on a vanilla Ubuntu 20.04.2.0 Live DVD system, the helper program file called `qnap-ec` which would be installed in the `/usr/local/sbin` directory (on a vanilla Ubuntu 20.04.2.0 Live DVD system) and the QNAP library file called `libuLinux_hal.so` which would be installed in the `/usr/local/lib` directory (on a vanilla Ubuntu 20.04.2.0 Live DVD).

If this driver is being installed on a Linux distribution with a different folder structure (for example Unraid) the files will need to be manually installed.  The `qnap-ec.ko` kernel module file location will depend on where the system usually expects kernel modules to be located.  The `qnap-ec` helper program file will need to reside in one of the following locations in order for the kernel module to be able to call it correctly:
```
/usr/local/sbin
/usr/local/bin
/usr/sbin
/usr/bin
/sbin
/bin
```

And the `libuLinux_hal.so` QNAP library file will need to be in a location where the dynamic linker will be able to find it.
