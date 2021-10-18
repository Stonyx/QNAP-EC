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
// Note: these function types are based on function signatures in the libuLinux_hal.so library
//       as decompiled by Ghidra (where int is 4 bytes long, uint4 is 4 bytes long, undefined4 is
//       4 bytes long and assumed signed, and double is 8 bytes long):
//       int ec_sys_get_fan_speed(int param_1, uint* param_2)
//       int ec_sys_get_fan_pwm(undefined4 param_1, int* param_2)
//       int ec_sys_get_temperature(int param_1, double*param_2)
//       int ec_sys_set_fan_speed(undefined4 param_1, int param_2)
//       and as decompiled by IDA (where all by the first two arguments are assumed to be local
//       variable assignments):
//       __int64 __fastcall ec_sys_get_fan_speed(int a1, _DWORD *a2, __int64 a3, __int64 a4,
//                                               int a5, int a6)
//       __int64 __fastcall ec_sys_get_fan_pwm(int a1, _DWORD *a2, __int64 a3, __int64 a4, int a5,
//                                             int a6)
//       __int64 __fastcall ec_sys_get_temperature(int a1, double *a2, __int64 a3, __int64 a4,
//                                                 int a5, int a6)
//       __int64 __fastcall ec_sys_set_fan_speed(int a1, int a2, __int64 a3, __int64 a4, int a5,
//                                               int a6)
//       and on trial and error testing to determine which function signatures result in the
//       correct behaviour
enum qnap_ec_ioctl_function_type {
  int16_func_uint16_uint16pointer,
  int16_func_uint16_doublepointer,
  int16_func_uint16_uint16
};

// Define the I/O control command structure
// Note: we are using an int64 field instead of a double field because floating point math is not
//       possible in kernel space
struct qnap_ec_ioctl_command {
  enum qnap_ec_ioctl_function_type function_type;
  char function_name[50];
  uint16_t argument1_uint16;
  uint16_t argument2_uint16;
  int64_t argument2_int64;        
  int16_t return_value_int16;
};

// Define I/O control commands
// Note: using I/O control number 10 to match the major number of the miscellaneous device
#define QNAP_EC_IOCTL_CALL _IOR(10, 0, struct qnap_ec_ioctl_command)
#define QNAP_EC_IOCTL_RETURN _IOW(10, 1, struct qnap_ec_ioctl_command)