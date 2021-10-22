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

#include <stdint.h>

// Note: this simulation is based on the equavelent functions in the libuLinux_hal library as
//       decompiled by IDA and on testing done to determine values returned by the actual
//       libuLinux_hal library functions when running on a TS-673A unit

int8_t ec_sys_get_fan_status(uint8_t channel, uint32_t* status)
{
  switch (channel)
  {
    case 0:
    case 1:
      *status = 0;
      return 0;
    case 2:
    case 3:
    case 4:
    case 5:
      *status = 1;
      return 0;
    case 6:
      *status = 0;
      return 0;
    case 7:
      *status = 1;
      return 0;
    case 10:
    case 11:
      *status = 0;
      return 0;
    case 20:
    case 21:
    case 22:
    case 23:
    case 24:
    case 25:
      *status = 0;
      return 0;
    case 30:
    case 31:
    case 32:
    case 33:
    case 34:
    case 35:
      *status = 0;
      return 0;
    default:
      *status = 0;
      return -1;
  }
}

int8_t ec_sys_get_fan_speed(uint8_t channel, uint32_t* speed)
{
  switch (channel)
  {
    case 0:
      *speed = 651;
      return 0;
    case 1:
      *speed = 661;
      return 0;
    case 2:
    case 3:
    case 4:
    case 5:
      *speed = 65535;
      return 0;
    case 6:
      *speed = 891;
      return 0;
    case 7:
      *speed = 65535;
      return 0;
    case 10:
    case 11:
      *speed = 0;
      *speed = 1 / *speed;
      return 0;
    case 20:
    case 21:
    case 22:
    case 23:
    case 24:
    case 25:
      *speed = 65535;
      return 0;
    case 30:
    case 31:
    case 32:
      *speed = 65535;
      return 0;      
    case 33:
      *speed = 4976;
      return 0;
    case 34:
      *speed = 12096;
      return 0;
    case 35:
      *speed = 65535;
      return 0;
    default:
      *speed = 0;
      return -1;
  }
}

int8_t ec_sys_get_fan_pwm(uint8_t channel, uint32_t* pwm)
{
  switch (channel)
  {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
      *pwm = 76;
      return 0;
    case 20:
    case 21:
    case 22:
    case 23:
    case 24:
    case 25:
      *pwm = 76;
      return 0;
    case 30:
    case 31:
    case 32:
    case 33:
    case 34:
    case 35:
      *pwm = 650;
      return 0;
    default:
      *pwm = 0;
      return -1;
  }
}

int8_t ec_sys_get_temperature(uint8_t channel, double* temperature)
{
  switch (channel)
  {
    case 0:
      *temperature = 29;
      return 0;      
    case 1:
      *temperature = -1;
      return 0;
    case 5:
      *temperature = 23;
      return 0;
    case 6:
      *temperature = 25;
      return 0;
    case 7:
      *temperature = 30;
      return 0;
    case 10:
    case 11:
      *temperature = 0;
      *temperature = 1 / *temperature;
      return 0;
    case 15:
      *temperature = -128;
      return 0;
    case 16:
    case 17:
    case 18:
    case 19:
    case 20:
    case 21:
    case 22:
    case 23:
    case 24:
    case 25:
    case 26:
    case 27:
    case 28:
    case 29:
    case 30:
    case 31:
    case 32:
    case 33:
    case 34:
    case 35:
    case 36:
    case 37:
    case 38:
      *temperature = -1;
      return 0;
    default:
      *temperature = 0;
      return -1;
  }
}

int8_t ec_sys_set_fan_speed(uint8_t channel, uint8_t pwm)
{
  switch (channel)
  {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
    case 20:
    case 21:
    case 22:
    case 23:
    case 24:
    case 25:
    case 30:
    case 31:
    case 32:
    case 33:
    case 34:
    case 35:
      return 0;
    default:
      return -1;
  }
}