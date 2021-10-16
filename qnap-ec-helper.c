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
#include <stdio.h>
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
  int32_t (*int32_function_int32_uint32pointer)(int32_t, uint32_t*);
  int32_t (*int32_function_uint32_int32pointer)(uint32_t, int32_t*);
  int32_t (*int32_function_int32_doublepointer)(int32_t, double*);
  int32_t (*int32_function_uint32_int32)(uint32_t, int32_t);
  double double_value;

  // Open the system log
  openlog("qnap-ec", LOG_PID, LOG_USER);

  // Open the qnap-ec device
  device = open("/dev/qnap-ec", O_RDWR);
  if (device < 0)
  {
    syslog(LOG_DEBUG, "open function returned a non zero value trying to open the /dev/qnap-ec "
      "device");
    closelog();
    exit(EXIT_FAILURE);
  }

  // Make a I/O control call to the device to find out which function in the library needs to be
  //   called
  if (ioctl(device, QNAP_EC_IOCTL_CALL, (int32_t*)&ioctl_command) != 0)
  {
    syslog(LOG_DEBUG, "ioctl function returned a non zero value trying to make the initial ioctl "
      "call");
    close(device);
    closelog();
    exit(EXIT_FAILURE);
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
    case int32_func_int32_uint32pointer:
      // Clear any previous dynamic link errors
      dlerror();

      // Get a pointer to the function
      int32_function_int32_uint32pointer = dlsym(library, ioctl_command.function_name);
      error = dlerror();
      if (error != NULL)
      {
        syslog(LOG_DEBUG, "dlsym function returned the following error when looking for the %s "
          "function:\n%s", ioctl_command.function_name, error);
        dlclose(library);
        close(device);
        closelog();
        exit(EXIT_FAILURE);
      }

      // Log debug messages
      syslog(LOG_DEBUG, "%s function called with %i, %u arguments", ioctl_command.function_name,
        ioctl_command.argument1_int32, ioctl_command.argument2_uint32);

      // Call the library function
      ioctl_command.return_value_int32 = int32_function_int32_uint32pointer(ioctl_command.
        argument1_int32, &ioctl_command.argument2_uint32);

      // Log debug messages
      syslog(LOG_DEBUG, "%s function returned %i", ioctl_command.function_name,
        ioctl_command.return_value_int32);
      syslog(LOG_DEBUG, "%s function second argument set to %u after the function returned",
        ioctl_command.function_name, ioctl_command.argument2_uint32);

      break;
    case int32_func_uint32_int32pointer:
      // Clear any previous dynamic link errors
      dlerror();

      // Get a pointer to the function
      int32_function_uint32_int32pointer = dlsym(library, ioctl_command.function_name);
      error = dlerror();
      if (error != NULL)
      {
        syslog(LOG_DEBUG, "dlsym function returned the following error when looking for the %s "
          "function:\n%s", ioctl_command.function_name, error);
        dlclose(library);
        close(device);
        closelog();
        exit(EXIT_FAILURE);
      }

      // Log debug messages
      syslog(LOG_DEBUG, "%s function called with %u, %i arguments", ioctl_command.function_name,
        ioctl_command.argument1_uint32, ioctl_command.argument2_int32);

      // Call the library function
      ioctl_command.return_value_int32 = int32_function_uint32_int32pointer(ioctl_command.
        argument1_uint32, &ioctl_command.argument2_int32);

      // Log debug messages
      syslog(LOG_DEBUG, "%s function returned %i", ioctl_command.function_name,
        ioctl_command.return_value_int32);
      syslog(LOG_DEBUG, "%s function second argument set to %u after the function returned",
        ioctl_command.function_name, ioctl_command.argument2_uint32);

      break;
    case int32_func_int32_doublepointer:
      // Clear any previous dynamic link errors
      dlerror();

      // Get a pointer to the function
      int32_function_int32_doublepointer = dlsym(library, ioctl_command.function_name);
      error = dlerror();
      if (error != NULL)
      {
        syslog(LOG_DEBUG, "dlsym function returned the following error when looking for the %s "
          "function:\n%s", ioctl_command.function_name, error);
        dlclose(library);
        close(device);
        closelog();
        exit(EXIT_FAILURE);
      }

      // Log debug messages
      syslog(LOG_DEBUG, "%s function called with %i, %f arguments", ioctl_command.function_name,
        ioctl_command.argument1_int32, (double)ioctl_command.argument2_int64);

      // Cast the int64 field to a double value
      // Note: we are using an int64 field instead of a double field because floating point math
      //       is not possible in kernel space
      double_value = (double)ioctl_command.argument2_int64;

      // Call the library function
      ioctl_command.return_value_int32 = int32_function_int32_doublepointer(ioctl_command.
        argument1_int32, &double_value);

      // Round and cast the double value back to the int64 field
      ioctl_command.argument2_int64 = (int64_t)(double_value + 0.5);

      // Log debug messages
      syslog(LOG_DEBUG, "%s function returned %i", ioctl_command.function_name, 
        ioctl_command.return_value_int32);
      syslog(LOG_DEBUG, "%s function second argument set to %li after the function returned",
        ioctl_command.function_name, ioctl_command.argument2_int64);

      break;
    case int32_func_uint32_int32:
      // Clear any previous dynamic link errors
      dlerror();

      // Get a pointer to the function
      int32_function_uint32_int32 = dlsym(library, ioctl_command.function_name);
      error = dlerror();
      if (error  != NULL)
      {
        syslog(LOG_DEBUG, "dlsym function returned the following error when looking for the %s "
          "function:\n%s", ioctl_command.function_name, error);
        dlclose(library);
        close(device);
        closelog();
        exit(EXIT_FAILURE);
      }

      // Log debug messages
      syslog(LOG_DEBUG, "%s function called with %u, %i arguments", ioctl_command.function_name,
        ioctl_command.argument1_uint32, ioctl_command.argument2_int32);

      // Call the library function
      ioctl_command.return_value_int32 = int32_function_uint32_int32(ioctl_command.argument1_uint32,
        ioctl_command.argument2_int32);

      // Log debug messages
      syslog(LOG_DEBUG, "%s function returned %i", ioctl_command.function_name,
        ioctl_command.return_value_int32);

      break;
    default:
      syslog(LOG_DEBUG, "invalid function type received via initial ioctl call");
      dlclose(library);
      close(device);
      closelog();
      exit(EXIT_FAILURE);
  }

  // Make the I/O control call to the device to return the data
  if (ioctl(device, QNAP_EC_IOCTL_RETURN, (int32_t*)&ioctl_command) != 0)
  {
    syslog(LOG_DEBUG, "ioctl function returned non zero value when making final ioctl call");
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