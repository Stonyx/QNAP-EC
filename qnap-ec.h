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

// Define the I/O control enums
enum qnap_ec_ioctl_call_func_functions {
  ec_sys_get_fan_status,
  ec_sys_get_fan_speed,
  ec_sys_get_fan_pwm,
  ec_sys_get_temperature,
  ec_sys_set_fan_speed
};

// Define the I/O control data structures
struct qnap_ec_ioctl_call_func_data {
  enum qnap_ec_ioctl_call_func_functions function;
  uint32_t argument1;
  uint32_t argument2;
};
struct qnap_ec_ioctl_return_data {
  uint32_t value;
};

// Define I/O control commands
// Note: 0x01 is the first IOCtl number not used according to the kernel documentation
#define QNAP_EC_IOCTL_CALL_FUNC _IOR(0x01, 0x01, struct qnap_ec_ioctl_call_func_data)
#define QNAP_EC_IOCTL_RETURN _IOW(0x01, 0x02, struct qnap_ec_ioctl_return_data)