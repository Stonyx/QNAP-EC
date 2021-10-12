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

#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include "qnap-ec.h"

// Define fan, pwm, and temperature sensor ID arrays
// Note: IDs are based on the switch statements in the it8528_get_fan_speed, it8528_get_fan_pwm, and
//       it8528_get_temperature functions in the libuLinux_hal.so library as decompiled by IDA
static const uint8_t qnap_ec_fan_ids[] = { 5, 7, 10, 11, 25, 35 };
static const uint8_t qnap_ec_pwm_ids[] = { 5, 7, 25, 35 };
static const uint8_t qnap_ec_temp_ids[] = { 1, 7, 10, 11, 38 };

// Function called as main entry point
int main(int argc, char** argv)
{
  // Declare needed variables
  int device;
  struct qnap_ec_ioctl_call_func_data ioctl_call_func_data;

  // Open the qnap-ec device
  device = open("/dev/qnap-ec", O_RDWR);
  if (device < 0)
  {
    return -1;
  }

  ioctl(device, QNAP_EC_IOCTL_CALL_FUNC, (int32_t*)&ioctl_call_func_data);

  // Close the qnap-ec device
  close(device);

  return 0;
}