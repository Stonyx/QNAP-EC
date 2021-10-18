/*
 * Copyright (C) 2021 Stonyx
 * https://www.stonyx.com/
 *
 * This driver is free software. You can redistribute it and/or modify it under the terms of the
 * GNU General Public License Version 3 (or at your option any later version) as published by The
 * Free Software Foundation.
 *
 * This driver is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See
 * the GNU General Public License for more details.
 *
 * If you did not received a copy of the GNU General Public License along with this script see
 * http://www.gnu.org/copyleft/gpl.html or write to The Free Software Foundation, 675 Mass Ave,
 * Cambridge, MA 02139, USA.
 */

// Define the I/O control function types enum
// Note: these function types are based on the following function signatures in the
//       libuLinux_hal.so library as decompiled by Ghidra
//       int ec_sys_get_fan_speed(int param_1, uint* param_2)
//       int ec_sys_get_fan_pwm(undefined4 param_1, int* param_2)
//       int ec_sys_get_temperature(int param_1, double*param_2)
//       int ec_sys_set_fan_speed(undefined4 param_1, int param_2)
//       where int is 4 bytes long, uint4 is 4 bytes long, undefined4 is 4 bytes long and assumed
//       unsigned, and double is 8 bytes long
// Note: Ghidra was used instead of IDA since it seems to be more accurate at determining
//       function signatures
enum qnap_ec_ioctl_function_type {
  int32_func_int32_uint32pointer,
  int32_func_uint32_int32pointer,
  int32_func_int32_doublepointer,
  int32_func_uint32_int32
};

// Define the I/O control command structure
// Note: we are using an int64 field instead of a double field because floating point math is not
//       possible in kernel space
struct qnap_ec_ioctl_command {
  enum qnap_ec_ioctl_function_type function_type;
  char function_name[50];
  int32_t argument1_int32;
  uint32_t argument1_uint32;
  uint32_t argument2_uint32;
  int32_t argument2_int32;
  int64_t argument2_int64;        
  int32_t return_value_int32;
};

// Define I/O control commands
// Note: using I/O control number 10 to match the major number of the miscellaneous device
#define QNAP_EC_IOCTL_CALL _IOR(10, 0, struct qnap_ec_ioctl_command)
#define QNAP_EC_IOCTL_RETURN _IOW(10, 1, struct qnap_ec_ioctl_command)