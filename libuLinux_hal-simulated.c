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
#include <stdlib.h>
#include <time.h>

/*
 * This simulation is based on the equavelent functions in the libuLinux_hal library as decompiled
 * by IDA and on testing done to determine values returned by the actual libuLinux_hal library
 * functions when running on a TS-673A unit as shown below:
 *
 * ec_sys_get_fan_status(0, 0) called as int8_t function(uint8_t, uint32_t*):
 * returned = 0, argument 1 after call = 0, argument 2 after call = 0
 * ec_sys_get_fan_status(1, 0) called as int8_t function(uint8_t, uint32_t*):
 * returned = 0, argument 1 after call = 1, argument 2 after call = 0
 * ec_sys_get_fan_status(2, 0) called as int8_t function(uint8_t, uint32_t*):
 * returned = 0, argument 1 after call = 2, argument 2 after call = 1
 * ec_sys_get_fan_status(3, 0) called as int8_t function(uint8_t, uint32_t*):
 * returned = 0, argument 1 after call = 3, argument 2 after call = 1
 * ec_sys_get_fan_status(4, 0) called as int8_t function(uint8_t, uint32_t*):
 * returned = 0, argument 1 after call = 4, argument 2 after call = 1
 * ec_sys_get_fan_status(5, 0) called as int8_t function(uint8_t, uint32_t*):
 * returned = 0, argument 1 after call = 5, argument 2 after call = 1
 * ec_sys_get_fan_status(6, 0) called as int8_t function(uint8_t, uint32_t*):
 * returned = 0, argument 1 after call = 6, argument 2 after call = 0
 * ec_sys_get_fan_status(7, 0) called as int8_t function(uint8_t, uint32_t*):
 * returned = 0, argument 1 after call = 7, argument 2 after call = 1
 * ec_sys_get_fan_status(8, 0) called as int8_t function(uint8_t, uint32_t*):
 * returned = -1, argument 1 after call = 8, argument 2 after call = 0
 * ec_sys_get_fan_status(9, 0) called as int8_t function(uint8_t, uint32_t*):
 * returned = -1, argument 1 after call = 9, argument 2 after call = 0
 * ec_sys_get_fan_status(10, 0) called as int8_t function(uint8_t, uint32_t*):
 * returned = 0, argument 1 after call = 10, argument 2 after call = 0
 * ec_sys_get_fan_status(11, 0) called as int8_t function(uint8_t, uint32_t*):
 * returned = 0, argument 1 after call = 11, argument 2 after call = 0
 * ec_sys_get_fan_status(12, 0) called as int8_t function(uint8_t, uint32_t*):
 * returned = -1, argument 1 after call = 12, argument 2 after call = 0
 * ec_sys_get_fan_status(13, 0) called as int8_t function(uint8_t, uint32_t*):
 * returned = -1, argument 1 after call = 13, argument 2 after call = 0
 * ec_sys_get_fan_status(14, 0) called as int8_t function(uint8_t, uint32_t*):
 * returned = -1, argument 1 after call = 14, argument 2 after call = 0
 * ec_sys_get_fan_status(15, 0) called as int8_t function(uint8_t, uint32_t*):
 * returned = -1, argument 1 after call = 15, argument 2 after call = 0
 * ec_sys_get_fan_status(16, 0) called as int8_t function(uint8_t, uint32_t*):
 * returned = -1, argument 1 after call = 16, argument 2 after call = 0
 * ec_sys_get_fan_status(17, 0) called as int8_t function(uint8_t, uint32_t*):
 * returned = -1, argument 1 after call = 17, argument 2 after call = 0
 * ec_sys_get_fan_status(18, 0) called as int8_t function(uint8_t, uint32_t*):
 * returned = -1, argument 1 after call = 18, argument 2 after call = 0
 * ec_sys_get_fan_status(19, 0) called as int8_t function(uint8_t, uint32_t*):
 * returned = -1, argument 1 after call = 19, argument 2 after call = 0
 * ec_sys_get_fan_status(20, 0) called as int8_t function(uint8_t, uint32_t*):
 * returned = 0, argument 1 after call = 20, argument 2 after call = 0
 * ec_sys_get_fan_status(21, 0) called as int8_t function(uint8_t, uint32_t*):
 * returned = 0, argument 1 after call = 21, argument 2 after call = 0
 * ec_sys_get_fan_status(22, 0) called as int8_t function(uint8_t, uint32_t*):
 * returned = 0, argument 1 after call = 22, argument 2 after call = 0
 * ec_sys_get_fan_status(23, 0) called as int8_t function(uint8_t, uint32_t*):
 * returned = 0, argument 1 after call = 23, argument 2 after call = 0
 * ec_sys_get_fan_status(24, 0) called as int8_t function(uint8_t, uint32_t*):
 * returned = 0, argument 1 after call = 24, argument 2 after call = 0
 * ec_sys_get_fan_status(25, 0) called as int8_t function(uint8_t, uint32_t*):
 * returned = 0, argument 1 after call = 25, argument 2 after call = 0
 * ec_sys_get_fan_status(26, 0) called as int8_t function(uint8_t, uint32_t*):
 * returned = -1, argument 1 after call = 26, argument 2 after call = 0
 * ec_sys_get_fan_status(27, 0) called as int8_t function(uint8_t, uint32_t*):
 * returned = -1, argument 1 after call = 27, argument 2 after call = 0
 * ec_sys_get_fan_status(28, 0) called as int8_t function(uint8_t, uint32_t*):
 * returned = -1, argument 1 after call = 28, argument 2 after call = 0
 * ec_sys_get_fan_status(29, 0) called as int8_t function(uint8_t, uint32_t*):
 * returned = -1, argument 1 after call = 29, argument 2 after call = 0
 * ec_sys_get_fan_status(30, 0) called as int8_t function(uint8_t, uint32_t*):
 * returned = 0, argument 1 after call = 30, argument 2 after call = 0
 * ec_sys_get_fan_status(31, 0) called as int8_t function(uint8_t, uint32_t*):
 * returned = 0, argument 1 after call = 31, argument 2 after call = 0
 * ec_sys_get_fan_status(32, 0) called as int8_t function(uint8_t, uint32_t*):
 * returned = 0, argument 1 after call = 32, argument 2 after call = 0
 * ec_sys_get_fan_status(33, 0) called as int8_t function(uint8_t, uint32_t*):
 * returned = 0, argument 1 after call = 33, argument 2 after call = 0
 * ec_sys_get_fan_status(34, 0) called as int8_t function(uint8_t, uint32_t*):
 * returned = 0, argument 1 after call = 34, argument 2 after call = 0
 * ec_sys_get_fan_status(35, 0) called as int8_t function(uint8_t, uint32_t*):
 * returned = 0, argument 1 after call = 35, argument 2 after call = 0
 * ec_sys_get_fan_speed(0, 0) called as int8_t function(uint8_t, uint32_t*):
 * returned = 0, argument 1 after call = 0, argument 2 after call = 666
 * ec_sys_get_fan_speed(1, 0) called as int8_t function(uint8_t, uint32_t*):
 * returned = 0, argument 1 after call = 1, argument 2 after call = 661
 * ec_sys_get_fan_speed(2, 0) called as int8_t function(uint8_t, uint32_t*):
 * returned = 0, argument 1 after call = 2, argument 2 after call = 65535
 * ec_sys_get_fan_speed(3, 0) called as int8_t function(uint8_t, uint32_t*):
 * returned = 0, argument 1 after call = 3, argument 2 after call = 65535
 * ec_sys_get_fan_speed(4, 0) called as int8_t function(uint8_t, uint32_t*):
 * returned = 0, argument 1 after call = 4, argument 2 after call = 65535
 * ec_sys_get_fan_speed(5, 0) called as int8_t function(uint8_t, uint32_t*):
 * returned = 0, argument 1 after call = 5, argument 2 after call = 65535
 * ec_sys_get_fan_speed(6, 0) called as int8_t function(uint8_t, uint32_t*):
 * returned = 0, argument 1 after call = 6, argument 2 after call = 891
 * ec_sys_get_fan_speed(7, 0) called as int8_t function(uint8_t, uint32_t*):
 * returned = 0, argument 1 after call = 7, argument 2 after call = 65535
 * ec_sys_get_fan_speed(8, 0) called as int8_t function(uint8_t, uint32_t*):
 * returned = -1, argument 1 after call = 8, argument 2 after call = 0
 * ec_sys_get_fan_speed(9, 0) called as int8_t function(uint8_t, uint32_t*):
 * returned = -1, argument 1 after call = 9, argument 2 after call = 0
 * ec_sys_get_fan_speed(10, 0) called as int8_t function(uint8_t, uint32_t*):
 * returned = 0, argument 1 after call = 10, argument 2 after call = -2
 * ec_sys_get_fan_speed(11, 0) called as int8_t function(uint8_t, uint32_t*):
 * returned = 0, argument 1 after call = 11, argument 2 after call = -2
 * ec_sys_get_fan_speed(12, 0) called as int8_t function(uint8_t, uint32_t*):
 * returned = -1, argument 1 after call = 12, argument 2 after call = 0
 * ec_sys_get_fan_speed(13, 0) called as int8_t function(uint8_t, uint32_t*):
 * returned = -1, argument 1 after call = 13, argument 2 after call = 0
 * ec_sys_get_fan_speed(14, 0) called as int8_t function(uint8_t, uint32_t*):
 * returned = -1, argument 1 after call = 14, argument 2 after call = 0
 * ec_sys_get_fan_speed(15, 0) called as int8_t function(uint8_t, uint32_t*):
 * returned = -1, argument 1 after call = 15, argument 2 after call = 0
 * ec_sys_get_fan_speed(16, 0) called as int8_t function(uint8_t, uint32_t*):
 * returned = -1, argument 1 after call = 16, argument 2 after call = 0
 * ec_sys_get_fan_speed(17, 0) called as int8_t function(uint8_t, uint32_t*):
 * returned = -1, argument 1 after call = 17, argument 2 after call = 0
 * ec_sys_get_fan_speed(18, 0) called as int8_t function(uint8_t, uint32_t*):
 * returned = -1, argument 1 after call = 18, argument 2 after call = 0
 * ec_sys_get_fan_speed(19, 0) called as int8_t function(uint8_t, uint32_t*):
 * returned = -1, argument 1 after call = 19, argument 2 after call = 0
 * ec_sys_get_fan_speed(20, 0) called as int8_t function(uint8_t, uint32_t*):
 * returned = 0, argument 1 after call = 20, argument 2 after call = 65535
 * ec_sys_get_fan_speed(21, 0) called as int8_t function(uint8_t, uint32_t*):
 * returned = 0, argument 1 after call = 21, argument 2 after call = 65535
 * ec_sys_get_fan_speed(22, 0) called as int8_t function(uint8_t, uint32_t*):
 * returned = 0, argument 1 after call = 22, argument 2 after call = 65535
 * ec_sys_get_fan_speed(23, 0) called as int8_t function(uint8_t, uint32_t*):
 * returned = 0, argument 1 after call = 23, argument 2 after call = 65535
 * ec_sys_get_fan_speed(24, 0) called as int8_t function(uint8_t, uint32_t*):
 * returned = 0, argument 1 after call = 24, argument 2 after call = 65535
 * ec_sys_get_fan_speed(25, 0) called as int8_t function(uint8_t, uint32_t*):
 * returned = 0, argument 1 after call = 25, argument 2 after call = 65535
 * ec_sys_get_fan_speed(26, 0) called as int8_t function(uint8_t, uint32_t*):
 * returned = -1, argument 1 after call = 26, argument 2 after call = 0
 * ec_sys_get_fan_speed(27, 0) called as int8_t function(uint8_t, uint32_t*):
 * returned = -1, argument 1 after call = 27, argument 2 after call = 0
 * ec_sys_get_fan_speed(28, 0) called as int8_t function(uint8_t, uint32_t*):
 * returned = -1, argument 1 after call = 28, argument 2 after call = 0
 * ec_sys_get_fan_speed(29, 0) called as int8_t function(uint8_t, uint32_t*):
 * returned = -1, argument 1 after call = 29, argument 2 after call = 0
 * ec_sys_get_fan_speed(30, 0) called as int8_t function(uint8_t, uint32_t*):
 * returned = 0, argument 1 after call = 30, argument 2 after call = 65535
 * ec_sys_get_fan_speed(31, 0) called as int8_t function(uint8_t, uint32_t*):
 * returned = 0, argument 1 after call = 31, argument 2 after call = 65535
 * ec_sys_get_fan_speed(32, 0) called as int8_t function(uint8_t, uint32_t*):
 * returned = 0, argument 1 after call = 32, argument 2 after call = 65535
 * ec_sys_get_fan_speed(33, 0) called as int8_t function(uint8_t, uint32_t*):
 * returned = 0, argument 1 after call = 33, argument 2 after call = 4976
 * ec_sys_get_fan_speed(34, 0) called as int8_t function(uint8_t, uint32_t*):
 * returned = 0, argument 1 after call = 34, argument 2 after call = 12096
 * ec_sys_get_fan_speed(35, 0) called as int8_t function(uint8_t, uint32_t*):
 * returned = 0, argument 1 after call = 35, argument 2 after call = 65535
 * ec_sys_get_fan_pwm(0, 0) called as int8_t function(uint8_t, uint32_t*):
 * returned = 0, argument 1 after call = 0, argument 2 after call = 76
 * ec_sys_get_fan_pwm(1, 0) called as int8_t function(uint8_t, uint32_t*):
 * returned = 0, argument 1 after call = 1, argument 2 after call = 76
 * ec_sys_get_fan_pwm(2, 0) called as int8_t function(uint8_t, uint32_t*):
 * returned = 0, argument 1 after call = 2, argument 2 after call = 76
 * ec_sys_get_fan_pwm(3, 0) called as int8_t function(uint8_t, uint32_t*):
 * returned = 0, argument 1 after call = 3, argument 2 after call = 76
 * ec_sys_get_fan_pwm(4, 0) called as int8_t function(uint8_t, uint32_t*):
 * returned = 0, argument 1 after call = 4, argument 2 after call = 76
 * ec_sys_get_fan_pwm(5, 0) called as int8_t function(uint8_t, uint32_t*):
 * returned = 0, argument 1 after call = 5, argument 2 after call = 76
 * ec_sys_get_fan_pwm(6, 0) called as int8_t function(uint8_t, uint32_t*):
 * returned = 0, argument 1 after call = 6, argument 2 after call = 76
 * ec_sys_get_fan_pwm(7, 0) called as int8_t function(uint8_t, uint32_t*):
 * returned = 0, argument 1 after call = 7, argument 2 after call = 76
 * ec_sys_get_fan_pwm(8, 0) called as int8_t function(uint8_t, uint32_t*):
 * returned = -1, argument 1 after call = 8, argument 2 after call = 0
 * ec_sys_get_fan_pwm(9, 0) called as int8_t function(uint8_t, uint32_t*):
 * returned = -1, argument 1 after call = 9, argument 2 after call = 0
 * ec_sys_get_fan_pwm(10, 0) called as int8_t function(uint8_t, uint32_t*):
 * returned = -1, argument 1 after call = 10, argument 2 after call = 0
 * ec_sys_get_fan_pwm(11, 0) called as int8_t function(uint8_t, uint32_t*):
 * returned = -1, argument 1 after call = 11, argument 2 after call = 0
 * ec_sys_get_fan_pwm(12, 0) called as int8_t function(uint8_t, uint32_t*):
 * returned = -1, argument 1 after call = 12, argument 2 after call = 0
 * ec_sys_get_fan_pwm(13, 0) called as int8_t function(uint8_t, uint32_t*):
 * returned = -1, argument 1 after call = 13, argument 2 after call = 0
 * ec_sys_get_fan_pwm(14, 0) called as int8_t function(uint8_t, uint32_t*):
 * returned = -1, argument 1 after call = 14, argument 2 after call = 0
 * ec_sys_get_fan_pwm(15, 0) called as int8_t function(uint8_t, uint32_t*):
 * returned = -1, argument 1 after call = 15, argument 2 after call = 0
 * ec_sys_get_fan_pwm(16, 0) called as int8_t function(uint8_t, uint32_t*):
 * returned = -1, argument 1 after call = 16, argument 2 after call = 0
 * ec_sys_get_fan_pwm(17, 0) called as int8_t function(uint8_t, uint32_t*):
 * returned = -1, argument 1 after call = 17, argument 2 after call = 0
 * ec_sys_get_fan_pwm(18, 0) called as int8_t function(uint8_t, uint32_t*):
 * returned = -1, argument 1 after call = 18, argument 2 after call = 0
 * ec_sys_get_fan_pwm(19, 0) called as int8_t function(uint8_t, uint32_t*):
 * returned = -1, argument 1 after call = 19, argument 2 after call = 0
 * ec_sys_get_fan_pwm(20, 0) called as int8_t function(uint8_t, uint32_t*):
 * returned = 0, argument 1 after call = 20, argument 2 after call = 76
 * ec_sys_get_fan_pwm(21, 0) called as int8_t function(uint8_t, uint32_t*):
 * returned = 0, argument 1 after call = 21, argument 2 after call = 76
 * ec_sys_get_fan_pwm(22, 0) called as int8_t function(uint8_t, uint32_t*):
 * returned = 0, argument 1 after call = 22, argument 2 after call = 76
 * ec_sys_get_fan_pwm(23, 0) called as int8_t function(uint8_t, uint32_t*):
 * returned = 0, argument 1 after call = 23, argument 2 after call = 76
 * ec_sys_get_fan_pwm(24, 0) called as int8_t function(uint8_t, uint32_t*):
 * returned = 0, argument 1 after call = 24, argument 2 after call = 76
 * ec_sys_get_fan_pwm(25, 0) called as int8_t function(uint8_t, uint32_t*):
 * returned = 0, argument 1 after call = 25, argument 2 after call = 76
 * ec_sys_get_fan_pwm(26, 0) called as int8_t function(uint8_t, uint32_t*):
 * returned = -1, argument 1 after call = 26, argument 2 after call = 0
 * ec_sys_get_fan_pwm(27, 0) called as int8_t function(uint8_t, uint32_t*):
 * returned = -1, argument 1 after call = 27, argument 2 after call = 0
 * ec_sys_get_fan_pwm(28, 0) called as int8_t function(uint8_t, uint32_t*):
 * returned = -1, argument 1 after call = 28, argument 2 after call = 0
 * ec_sys_get_fan_pwm(29, 0) called as int8_t function(uint8_t, uint32_t*):
 * returned = -1, argument 1 after call = 29, argument 2 after call = 0
 * ec_sys_get_fan_pwm(30, 0) called as int8_t function(uint8_t, uint32_t*):
 * returned = 0, argument 1 after call = 30, argument 2 after call = 650
 * ec_sys_get_fan_pwm(31, 0) called as int8_t function(uint8_t, uint32_t*):
 * returned = 0, argument 1 after call = 31, argument 2 after call = 650
 * ec_sys_get_fan_pwm(32, 0) called as int8_t function(uint8_t, uint32_t*):
 * returned = 0, argument 1 after call = 32, argument 2 after call = 650
 * ec_sys_get_fan_pwm(33, 0) called as int8_t function(uint8_t, uint32_t*):
 * returned = 0, argument 1 after call = 33, argument 2 after call = 650
 * ec_sys_get_fan_pwm(34, 0) called as int8_t function(uint8_t, uint32_t*):
 * returned = 0, argument 1 after call = 34, argument 2 after call = 650
 * ec_sys_get_fan_pwm(35, 0) called as int8_t function(uint8_t, uint32_t*):
 * returned = 0, argument 1 after call = 35, argument 2 after call = 650
 * ec_sys_get_temperature(0, 0.000000) called as int8_t function(uint8_t, double*):
 * returned = 0, argument 1 after call = 0, argument 2 after call = 29.000000
 * ec_sys_get_temperature(1, 0.000000) called as int8_t function(uint8_t, double*):
 * returned = 0, argument 1 after call = 1, argument 2 after call = -1.000000
 * ec_sys_get_temperature(2, 0.000000) called as int8_t function(uint8_t, double*):
 * returned = -1, argument 1 after call = 2, argument 2 after call = 0.000000
 * ec_sys_get_temperature(3, 0.000000) called as int8_t function(uint8_t, double*):
 * returned = -1, argument 1 after call = 3, argument 2 after call = 0.000000
 * ec_sys_get_temperature(4, 0.000000) called as int8_t function(uint8_t, double*):
 * returned = -1, argument 1 after call = 4, argument 2 after call = 0.000000
 * ec_sys_get_temperature(5, 0.000000) called as int8_t function(uint8_t, double*):
 * returned = 0, argument 1 after call = 5, argument 2 after call = 23.000000
 * ec_sys_get_temperature(6, 0.000000) called as int8_t function(uint8_t, double*):
 * returned = 0, argument 1 after call = 6, argument 2 after call = 25.000000
 * ec_sys_get_temperature(7, 0.000000) called as int8_t function(uint8_t, double*):
 * returned = 0, argument 1 after call = 7, argument 2 after call = 30.000000
 * ec_sys_get_temperature(8, 0.000000) called as int8_t function(uint8_t, double*):
 * returned = -1, argument 1 after call = 8, argument 2 after call = 0.000000
 * ec_sys_get_temperature(9, 0.000000) called as int8_t function(uint8_t, double*):
 * returned = -1, argument 1 after call = 9, argument 2 after call = 0.000000
 * ec_sys_get_temperature(10, 0.000000) called as int8_t function(uint8_t, double*):
 * returned = 0, argument 1 after call = 10, argument 2 after call = -2.000000
 * ec_sys_get_temperature(11, 0.000000) called as int8_t function(uint8_t, double*):
 * returned = 0, argument 1 after call = 11, argument 2 after call = -2.000000
 * ec_sys_get_temperature(12, 0.000000) called as int8_t function(uint8_t, double*):
 * returned = -1, argument 1 after call = 12, argument 2 after call = 0.000000
 * ec_sys_get_temperature(13, 0.000000) called as int8_t function(uint8_t, double*):
 * returned = -1, argument 1 after call = 13, argument 2 after call = 0.000000
 * ec_sys_get_temperature(14, 0.000000) called as int8_t function(uint8_t, double*):
 * returned = -1, argument 1 after call = 14, argument 2 after call = 0.000000
 * ec_sys_get_temperature(15, 0.000000) called as int8_t function(uint8_t, double*):
 * returned = 0, argument 1 after call = 15, argument 2 after call = -128.000000
 * ec_sys_get_temperature(16, 0.000000) called as int8_t function(uint8_t, double*):
 * returned = 0, argument 1 after call = 16, argument 2 after call = -1.000000
 * ec_sys_get_temperature(17, 0.000000) called as int8_t function(uint8_t, double*):
 * returned = 0, argument 1 after call = 17, argument 2 after call = -1.000000
 * ec_sys_get_temperature(18, 0.000000) called as int8_t function(uint8_t, double*):
 * returned = 0, argument 1 after call = 18, argument 2 after call = -1.000000
 * ec_sys_get_temperature(19, 0.000000) called as int8_t function(uint8_t, double*):
 * returned = 0, argument 1 after call = 19, argument 2 after call = -1.000000
 * ec_sys_get_temperature(20, 0.000000) called as int8_t function(uint8_t, double*):
 * returned = 0, argument 1 after call = 20, argument 2 after call = -1.000000
 * ec_sys_get_temperature(21, 0.000000) called as int8_t function(uint8_t, double*):
 * returned = 0, argument 1 after call = 21, argument 2 after call = -1.000000
 * ec_sys_get_temperature(22, 0.000000) called as int8_t function(uint8_t, double*):
 * returned = 0, argument 1 after call = 22, argument 2 after call = -1.000000
 * ec_sys_get_temperature(23, 0.000000) called as int8_t function(uint8_t, double*):
 * returned = 0, argument 1 after call = 23, argument 2 after call = -1.000000
 * ec_sys_get_temperature(24, 0.000000) called as int8_t function(uint8_t, double*):
 * returned = 0, argument 1 after call = 24, argument 2 after call = -1.000000
 * ec_sys_get_temperature(25, 0.000000) called as int8_t function(uint8_t, double*):
 * returned = 0, argument 1 after call = 25, argument 2 after call = -1.000000
 * ec_sys_get_temperature(26, 0.000000) called as int8_t function(uint8_t, double*):
 * returned = 0, argument 1 after call = 26, argument 2 after call = -1.000000
 * ec_sys_get_temperature(27, 0.000000) called as int8_t function(uint8_t, double*):
 * returned = 0, argument 1 after call = 27, argument 2 after call = -1.000000
 * ec_sys_get_temperature(28, 0.000000) called as int8_t function(uint8_t, double*):
 * returned = 0, argument 1 after call = 28, argument 2 after call = -1.000000
 * ec_sys_get_temperature(29, 0.000000) called as int8_t function(uint8_t, double*):
 * returned = 0, argument 1 after call = 29, argument 2 after call = -1.000000
 * ec_sys_get_temperature(30, 0.000000) called as int8_t function(uint8_t, double*):
 * returned = 0, argument 1 after call = 30, argument 2 after call = -1.000000
 * ec_sys_get_temperature(31, 0.000000) called as int8_t function(uint8_t, double*):
 * returned = 0, argument 1 after call = 31, argument 2 after call = -1.000000
 * ec_sys_get_temperature(32, 0.000000) called as int8_t function(uint8_t, double*):
 * returned = 0, argument 1 after call = 32, argument 2 after call = -1.000000
 * ec_sys_get_temperature(33, 0.000000) called as int8_t function(uint8_t, double*):
 * returned = 0, argument 1 after call = 33, argument 2 after call = -1.000000
 * ec_sys_get_temperature(34, 0.000000) called as int8_t function(uint8_t, double*):
 * returned = 0, argument 1 after call = 34, argument 2 after call = -1.000000
 * ec_sys_get_temperature(35, 0.000000) called as int8_t function(uint8_t, double*):
 * returned = 0, argument 1 after call = 35, argument 2 after call = -1.000000
 * ec_sys_get_temperature(36, 0.000000) called as int8_t function(uint8_t, double*):
 * returned = 0, argument 1 after call = 36, argument 2 after call = -1.000000
 * ec_sys_get_temperature(37, 0.000000) called as int8_t function(uint8_t, double*):
 * returned = 0, argument 1 after call = 37, argument 2 after call = -1.000000
 * ec_sys_get_temperature(38, 0.000000) called as int8_t function(uint8_t, double*):
 * returned = 0, argument 1 after call = 38, argument 2 after call = -1.000000
 */

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
      // Set speed to a random number between 650 and 660
      srand(time(NULL) + 1);
      *speed = rand() % (660 + 1 - 650) + 650;
      return 0;
    case 1:
      // Set speed to a random number between 650 and 660
      srand(time(NULL) + 2);
      *speed = rand() % (660 + 1 - 650) + 650;
      return 0;
    case 2:
    case 3:
    case 4:
    case 5:
      *speed = 65535;
      return 0;
    case 6:
      // Set speed to a random number between 890 and 900
      srand(time(NULL) + 3);
      *speed = rand() % (900 + 1 - 890) + 890;
      return 0;
    case 7:
      *speed = 65535;
      return 0;
    case 10:
    case 11:
      *speed = -2;
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
      *pwm = 75;
      return 0;
    case 20:
    case 21:
    case 22:
    case 23:
    case 24:
    case 25:
      *pwm = 75;
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
      // Set temperature to a random number between 28 and 30
      srand(time(NULL) + 4);
      *temperature = rand() % (30 + 1 - 28) + 28;
      return 0;      
    case 1:
      *temperature = -1;
      return 0;
    case 5:
      // Set temperature to a random number between 23 and 25
      srand(time(NULL) + 5);
      *temperature = rand() % (25 + 1 - 23) + 23;
      return 0;
    case 6:
      // Set temperature to a random number between 23 and 25
      srand(time(NULL) + 6);
      *temperature = rand() % (25 + 1 - 23) + 23;
      return 0;
    case 7:
      // Set temperature to a random number between 28 and 30
      srand(time(NULL) + 7);
      *temperature = rand() % (30 + 1 - 28) + 28;
      return 0;
    case 10:
    case 11:
      *temperature = -2;
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