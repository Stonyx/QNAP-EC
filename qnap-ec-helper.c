/*
 * Copyright (C) 2021 Stonyx
 * https://www.stonyx.com/
 *
 * This program is free software. You can redistribute it and/or modify it under the terms of the
 * GNU General Public License Version 3 (or at your option any later version) as published by The
 * Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See
 * the GNU General Public License for more details.
 *
 * If you did not received a copy of the GNU General Public License along with this script see
 * http://www.gnu.org/copyleft/gpl.html or write to The Free Software Foundation, 675 Mass Ave,
 * Cambridge, MA 02139, USA.
 */

#include <dlfcn.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>
#include <syslog.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include "qnap-ec-ioctl.h"

// Function called as main entry point
int main(int argc, char** argv)
{
  // Declare needed variables
  char* error;
  int device;
  void* library;
  struct qnap_ec_ioctl_command ioctl_command;
  int8_t (*int8_function_uint8_uint32pointer)(uint8_t, uint32_t*);
  int8_t (*int8_function_uint8_doublepointer)(uint8_t, double*);
  int8_t (*int8_function_uint8_uint8)(uint8_t, uint8_t);
  double double_value;

  // Open the system log
  openlog("qnap-ec", LOG_PID, LOG_USER);

  // Open the qnap-ec device
  device = open("/dev/qnap-ec", O_RDWR);
  if (device < 0)
  {
    syslog(LOG_ERR, "unable to open qnap-ec device (/dev/qnap-ec)");
    closelog();
    exit(EXIT_FAILURE);
  }

  // Make a I/O control call to the device to find out which function in the library needs to be
  //   called
  if (ioctl(device, QNAP_EC_IOCTL_CALL, &ioctl_command) != 0)
  {
    close(device);
    closelog();
    exit(EXIT_FAILURE);
  }

  // Open the libuLinux_ini library
  library = dlopen("/usr/local/lib/libuLinux_ini.so", RTLD_LAZY | RTLD_GLOBAL);
  if (library == NULL)
  {
    // Open the libuLinux_ini library by name only and rely on the path to resolve its path
    library = dlopen("libuLinux_ini.so", RTLD_LAZY);
    if (library == NULL)
    {
      syslog(LOG_ERR, "libuLinux_ini library not found in the expected path (/usr/local/lib) or "
        "any of the paths searched in by the dynamic linker");
      close(device);
      closelog();
      exit(EXIT_FAILURE);
    }
  }

  // Open the libuLinux_hal library
  library = dlopen("/usr/local/lib/libuLinux_hal.so", RTLD_LAZY);
  if (library == NULL)
  {
    // Open the libuLinux_hal library by name only and rely on the path to resolve its path
    library = dlopen("libuLinux_hal.so", RTLD_LAZY);
    if (library == NULL)
    {
      syslog(LOG_ERR, "libuLinux_hal library not found in the expected path (/usr/local/lib) or "
        "any of the paths searched in by the dynamic linker");
      close(device);
      closelog();
      exit(EXIT_FAILURE);
    }
  }

  // Switch based on the function type
  switch (ioctl_command.function_type)
  {
    case int8_func_uint8_uint32pointer:
      // Clear any previous dynamic link errors
      dlerror();

      // Get a pointer to the function
      int8_function_uint8_uint32pointer = dlsym(library, ioctl_command.function_name);
      error = dlerror();
      if (error != NULL)
      {
        syslog(LOG_ERR, "encountered the following dynamic linker error: %s", error);
        dlclose(library);
        close(device);
        closelog();
        exit(EXIT_FAILURE);
      }

      // Call the library function
      ioctl_command.return_value_int8 = int8_function_uint8_uint32pointer(ioctl_command.
        argument1_uint8, &ioctl_command.argument2_uint32);

      break;
    case int8_func_uint8_doublepointer:
      // Clear any previous dynamic link errors
      dlerror();

      // Get a pointer to the function
      int8_function_uint8_doublepointer = dlsym(library, ioctl_command.function_name);
      error = dlerror();
      if (error != NULL)
      {
        syslog(LOG_ERR, "encountered the following dynamic linker error: %s", error);
        dlclose(library);
        close(device);
        closelog();
        exit(EXIT_FAILURE);
      }

      // Cast the int64 field to a double value (see note below)
      double_value = (double)((long double)ioctl_command.argument2_int64 / (long double)1000);

      // Call the library function
      ioctl_command.return_value_int8 = int8_function_uint8_doublepointer(ioctl_command.
        argument1_uint8, &double_value);

      // Cast the double value back to the int64 field by multiplying it by 1000 and rounding it
      // Note: we are using an int64 field instead of a double field because floating point math
      //       is not possible in kernel space and because an int64 value can hold a 19 digit
      //       integer while a double value can hold a 16 digit integer without loosing precision
      //       we can multiple the double value by 1000 to move three digits after the decimal
      //       point to before the decimal point and still fit the value in an int64 value and
      //       preserve three digits after the decimal point
      ioctl_command.argument2_int64 = (int64_t)((long double)double_value * (long double)1000 +
        (long double)0.5);

      break;
    case int8_func_uint8_uint8:
      // Clear any previous dynamic link errors
      dlerror();

      // Get a pointer to the function
      int8_function_uint8_uint8 = dlsym(library, ioctl_command.function_name);
      error = dlerror();
      if (error  != NULL)
      {
        syslog(LOG_ERR, "encountered the following dynamic linker error: %s", error);
        dlclose(library);
        close(device);
        closelog();
        exit(EXIT_FAILURE);
      }

      syslog(LOG_INFO, "calling %s function with %i and %i arguments", ioctl_command.function_name,
        ioctl_command.argument1_uint8, ioctl_command.argument2_uint8);

      // Call the library function
      ioctl_command.return_value_int8 = int8_function_uint8_uint8(ioctl_command.argument1_uint8,
        ioctl_command.argument2_uint8);

      syslog(LOG_INFO, "function %s returned %i", ioctl_command.function_name, 
        ioctl_command.return_value_int8);

      break;
    default:
      dlclose(library);
      close(device);
      closelog();
      exit(EXIT_FAILURE);
  }

  // Make the I/O control call to the device to return the data
  if (ioctl(device, QNAP_EC_IOCTL_RETURN, &ioctl_command) != 0)
  {
    dlclose(library);
    close(device);
    closelog();
    exit(EXIT_FAILURE);
  }

  // Close the libuLinux_hal library
  dlclose(library);

  // Close the qnap-ec device
  close(device);

  // Close the system log
  closelog();

  exit(EXIT_SUCCESS);
}