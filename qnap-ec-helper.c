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

#include <argp.h>
#include <stdint.h>
#include <stdio.h>

// Define fan, pwm, and temperature sensor ID arrays
// Note: IDs are based on the switch statements in the it8528_get_fan_speed, it8528_get_fan_pwm, and
//       it8528_get_temperature functions in the libuLinux_hal.so library as decompiled by IDA
static const uint8_t uqnap_ec_fan_ids[] = { 5, 7, 10, 11, 25, 35 };
static const uint8_t qnap_ec_pwm_ids[] = { 5, 7, 25, 35 };
static const uint8_t qnap_ec_temp_ids[] = { 1, 7, 10, 11, 38 };

// Define program details
static const char program_version[] = "qnap-ec-helper 0.1";
static const char program_contact[] = "<info@stonyx.com>";
static const char program_description[] = "QNAP-EC - Retrieve and set data on QNAP embedded controllers";
static const char program_required_args[] = "ARG1 ARG2";

// Function called to parse an option
static error_t parse_option(int key, char* argument, struct argp_state* state)
{
  // Get the input argument from the state
  struct arguments* arguments = state->input;

  return 0;
}

static struct argp_option options[] = {
  {"fan", 'f', "FAN_ID", 0, "ID of fan to retrieve data for" },
  {"pwm", 'p', "PWM_ID", 0, "ID of PWM to retrieve or set data for" },
  {"temp", 't', "TEMPERATURE_SENSOR_ID", 0, "ID of temperature sensor to retrieve data for" },
  {"set", 's', "VALUE", 0, "Value to set data to" },
  { 0 }
};

// Function called as main entry point
int main(int argc, char** argv)
{
  return 0;
}