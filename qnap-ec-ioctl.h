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
//       libuLinux_hal.so library as decompiled by IDA
//       __int64 __fastcall ec_sys_get_fan_status(int a1, _DWORD *a2, __int64 a3, __int64 a4,
//                                                __int64 a5, int a6)
//       __int64 __fastcall ec_sys_get_fan_speed(int a1, _DWORD *a2, __int64 a3, __int64 a4,
//                                               int a5, int a6)
//       __int64 __fastcall ec_sys_get_fan_pwm(int a1, _DWORD *a2, __int64 a3, __int64 a4, int a5,
//                                             int a6)
//       __int64 __fastcall ec_sys_get_temperature(int a1, double *a2, __int64 a3, __int64 a4,
//                                                 int a5, int a6)
//       __int64 __fastcall ec_sys_set_fan_speed(int a1, int a2, __int64 a3, __int64 a4, int a5,
//                                               int a6)
enum qnap_ec_ioctl_function_type {
  int64_func_int_uint32pointer,
  int64_func_int_doublepointer,
  int64_func_int_int
};

// Define the I/O control command structure
// Note: we are using an int64 field instead of a double field because floating point math is not
//       possible in kernel space
struct qnap_ec_ioctl_command {
  enum qnap_ec_ioctl_function_type function_type;
  char function_name[50];
  int argument1_int;
  uint32_t argument2_uint32;
  int64_t argument2_int64;        
  int argument2_int;
  int64_t return_value_int64;
};

// Define I/O control commands
// Note: 0x01 is the first IOCtl number not used according to the kernel documentation
#define QNAP_EC_IOCTL_CALL _IOR(0x01, 0x01, struct qnap_ec_ioctl_command)
#define QNAP_EC_IOCTL_RETURN _IOW(0x01, 0x02, struct qnap_ec_ioctl_command)