/*
 * Copyright (C) 2021-2022 Stonyx
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

#include <linux/fs.h>
#include <linux/hwmon.h>
#include <linux/io.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include "qnap-ec-ioctl.h"

// Define the pr_err prefix
#undef pr_fmt
#define pr_fmt(fmt) "%s @ %s: " fmt, "qnap-ec", __FUNCTION__

// Define module details
MODULE_DESCRIPTION("QNAP EC Driver");
MODULE_VERSION("1.1.1");
MODULE_AUTHOR("Stonyx - https://www.stonyx.com/");
MODULE_LICENSE("GPL");
MODULE_PARM_DESC(val_pwm_channels, "Validate PWM channels");
MODULE_PARM_DESC(sim_pwm_enable, "Simulate pwmX_enable sysfs attributes");
MODULE_PARM_DESC(check_for_chip, "Check for QNAP IT8528 E.C. chip");

// Define maximum number of possible channels
// Note: number of channels has to be multiples of 8 and less than 256 and is based on the switch
//       statements in the ec_sys_get_fan_status, ec_sys_get_fan_speed, ec_sys_get_fan_pwm, and
//       ec_sys_get_temperature functions in the libuLinux_hal.so library as decompiled by IDA and
//       rounded up to the nearest multiple of 32 to allow for future additions of channels in
//       those functions
#define QNAP_EC_NUMBER_OF_FAN_CHANNELS 64
#define QNAP_EC_NUMBER_OF_PWM_CHANNELS QNAP_EC_NUMBER_OF_FAN_CHANNELS
#define QNAP_EC_NUMBER_OF_TEMP_CHANNELS 64

// Define the devices structure
// Note: in order to use the container_of macro in the qnap_ec_misc_dev_open and 
//       qnap_ec_misc_dev_ioctl functions we need to make the misc_device member not a pointer
//       and in order to use the platform_device_alloc function (see note in the qnap_ec_init
//       function) we need to make the plat_device member a pointer
struct qnap_ec_devices {
  struct mutex misc_device_mutex;
  bool open_misc_device;
  struct miscdevice misc_device;
  struct platform_device* plat_device;
};

// Define the I/O control data structure
struct qnap_ec_data {
  struct mutex mutex;
  struct qnap_ec_devices* devices;
  struct qnap_ec_ioctl_command ioctl_command;
  uint8_t fan_channel_checked_field[QNAP_EC_NUMBER_OF_FAN_CHANNELS / 8];
  uint8_t fan_channel_valid_field[QNAP_EC_NUMBER_OF_FAN_CHANNELS / 8];
  uint8_t pwm_channel_checked_field[QNAP_EC_NUMBER_OF_PWM_CHANNELS / 8];
  uint8_t pwm_channel_valid_field[QNAP_EC_NUMBER_OF_PWM_CHANNELS / 8];
  uint8_t pwm_enable_value_field[QNAP_EC_NUMBER_OF_PWM_CHANNELS / 8];
  uint8_t temp_channel_checked_field[QNAP_EC_NUMBER_OF_TEMP_CHANNELS / 8];
  uint8_t temp_channel_valid_field[QNAP_EC_NUMBER_OF_TEMP_CHANNELS / 8];
};

// Declare functions
static int __init qnap_ec_init(void);
static int __init qnap_ec_is_chip_present(void);
static int qnap_ec_probe(struct platform_device* platform_dev);
static umode_t qnap_ec_hwmon_is_visible(const void* const_data, enum hwmon_sensor_types type,
                                        u32 attribute, int channel);
static int qnap_ec_hwmon_read(struct device* dev, enum hwmon_sensor_types type, u32 attribute,
                              int channel, long* value);
static int qnap_ec_hwmon_write(struct device* dev, enum hwmon_sensor_types type, u32 attribute,
                               int channel, long value);
static bool qnap_ec_is_fan_channel_valid(struct qnap_ec_data* data, uint8_t channel);
static bool qnap_ec_is_pwm_channel_valid(struct qnap_ec_data* data, uint8_t channel);
static bool qnap_ec_is_temp_channel_valid(struct qnap_ec_data* data, uint8_t channel);
static int qnap_ec_is_pwm_channel_valid_read_fan_pwms(struct qnap_ec_data* data, uint8_t channel,
                                                      uint8_t initial_pwm_values[],
                                                      uint8_t changed_pwm_values[]);
static int qnap_ec_call_lib_function(bool use_mutex, struct qnap_ec_data* data,
                                     enum qnap_ec_ioctl_function_type function_type,
                                     char* function_name, uint8_t argument1_uint8,
                                     uint8_t* argument2_uint8, uint32_t* argument2_uint32,
                                     int64_t* argument2_int64, bool log_return_error);
static int qnap_ec_misc_device_open(struct inode* inode, struct file* file);
static long int qnap_ec_misc_device_ioctl(struct file* file, unsigned int command,
                                          unsigned long argument);
static int qnap_ec_misc_device_release(struct inode* inode, struct file* file);
static void __exit qnap_ec_exit(void);

// Specifiy the initialization and exit functions
module_init(qnap_ec_init);
module_exit(qnap_ec_exit);

// Define the module parameters
static bool qnap_ec_val_pwm_channels = true;
static bool qnap_ec_sim_pwm_enable = false;
static bool qnap_ec_check_for_chip = true;
module_param_named(val_pwm_channels, qnap_ec_val_pwm_channels, bool, 0);
module_param_named(sim_pwm_enable, qnap_ec_sim_pwm_enable, bool, 0);
module_param_named(check_for_chip, qnap_ec_check_for_chip, bool, 0);

// Declare the platform driver structure pointer
static struct platform_driver* qnap_ec_plat_driver;

// Declare the devices structure pointer
static struct qnap_ec_devices* qnap_ec_devices;

// Function called to initialize the driver
static int __init qnap_ec_init(void)
{
  // Define static constant data consisting of the miscellaneous device file operations structure
  static const struct file_operations misc_device_file_ops = {
    .owner = THIS_MODULE,
    .open = &qnap_ec_misc_device_open,
    .unlocked_ioctl = &qnap_ec_misc_device_ioctl,
    .release = &qnap_ec_misc_device_release
  };

  // Declare needed variables
  int error;

  // Check if the embedded controll chip isn't present
  error = qnap_ec_is_chip_present();
  if (error)
    return error;

  // Allocate memory for the platform driver structure and populate various fields
  qnap_ec_plat_driver = kzalloc(sizeof(struct platform_driver), GFP_KERNEL);
  if (qnap_ec_plat_driver == NULL)
    return -ENOMEM;
  qnap_ec_plat_driver->driver.owner = THIS_MODULE;
  qnap_ec_plat_driver->driver.name = "qnap-ec";
  qnap_ec_plat_driver->probe = &qnap_ec_probe;

  // Register the driver
  error = platform_driver_register(qnap_ec_plat_driver);
  if (error)
  {
    // Free the platform driver structure memory
    kfree(qnap_ec_plat_driver);

    return error;
  }

  // Allocate memory for the devices structure
  qnap_ec_devices = kzalloc(sizeof(struct qnap_ec_devices), GFP_KERNEL);
  if (qnap_ec_devices == NULL)
  {
    // Unregister the driver
    platform_driver_unregister(qnap_ec_plat_driver);

    // Free the platform driver structure memory
    kfree(qnap_ec_plat_driver);

    return -ENOMEM;
  }

  // Initialize the miscellaneous device mutex
  mutex_init(&qnap_ec_devices->misc_device_mutex);

  // Populate various miscellaneous device structure fields
  qnap_ec_devices->misc_device.name = "qnap-ec";
  qnap_ec_devices->misc_device.minor = MISC_DYNAMIC_MINOR;
  qnap_ec_devices->misc_device.fops = &misc_device_file_ops;

  // Register the miscellaneous device
  // Note: we need to register the miscellaneous device before registering the platform device so
  //       that the miscellaneous device is available for use by the various functions called by
  //       the probe function which is called when the platform device is registered
  error = misc_register(&qnap_ec_devices->misc_device);
  if (error)
  {
    // Free the devices structure memory
    kfree(qnap_ec_devices);

    // Unregister the platform driver
    platform_driver_unregister(qnap_ec_plat_driver);

    // Free the platform driver structure memory
    kfree(qnap_ec_plat_driver);

    return error;
  }

  // Allocate memory for the platform device structure and populate various fields
  // Note: we are using the platform_device_alloc function in combination with the
  //       platform_device_add function instead of the platform_device_register function because
  //       this approach is recommended for legacy type drivers that use hardware probing, all
  //       other hwmon drivers use this approach, and it provides a device release function for us
  qnap_ec_devices->plat_device = platform_device_alloc("qnap-ec", 0);
  if (qnap_ec_devices->plat_device == NULL)
  {
    // Unregister the miscellaneous device
    misc_deregister(&qnap_ec_devices->misc_device);

    // Free the devices structure memory
    kfree(qnap_ec_devices);

    // Unregister the driver
    platform_driver_unregister(qnap_ec_plat_driver);

    // Free the platform driver structure memory
    kfree(qnap_ec_plat_driver);

    return -ENOMEM;
  }
  qnap_ec_devices->plat_device->name = "qnap-ec";
  qnap_ec_devices->plat_device->id = PLATFORM_DEVID_NONE;

  // "Register" the platform device
  error = platform_device_add(qnap_ec_devices->plat_device);
  if (error)
  {
    // Free the platform device structure memory
    platform_device_put(qnap_ec_devices->plat_device);

    // Unregister the miscellaneous device
    misc_deregister(&qnap_ec_devices->misc_device);

    // Free the devices structure memory
    kfree(qnap_ec_devices);

    // Unregister the platform driver
    platform_driver_unregister(qnap_ec_plat_driver);

    // Free the platform driver structure memory
    kfree(qnap_ec_plat_driver);

    return error;
  }

  return 0;
}

// Function called to check if the QNAP embedded controller chip is present
static int __init qnap_ec_is_chip_present(void)
{
  // Declare needed variables
  uint8_t byte1;
  uint8_t byte2;

  // Check if we should not check for the chip
  if (!qnap_ec_check_for_chip)
    return 0;

  // Request access to the input (0x2E) and output (0x2F) ports
  if (request_muxed_region(0x2E, 2, "qnap-ec") == NULL)
    return -EBUSY;

  // Write 0x20 to the input port
  outb(0x20, 0x2E);

  // Read the first identification byte from the output port
  byte1 = inb(0x2F);

  // Write 0x21 to the input port
  outb(0x21, 0x2E);

  // Read the second identification byte from the output port
  byte2 = inb(0x2F);

  // Check if the identification bytes do not match the expected values
  if (byte1 != 0x85 || byte2 != 0x28)
  {
    release_region(0x2E, 2);
    return -ENODEV;
  }

  // Release access to the input and output ports
  release_region(0x2E, 2);

  return 0;
}

// Function called to probe this driver
static int qnap_ec_probe(struct platform_device* platform_dev)
{
  // Define static non constant and constant data consisiting of mulitple configuration arrays,
  //   multiple hwmon channel info structures, the hwmon channel info structures array, and the
  //   hwmon chip information structure
  static u32 fan_config[QNAP_EC_NUMBER_OF_FAN_CHANNELS + 1];
  static u32 pwm_config[QNAP_EC_NUMBER_OF_PWM_CHANNELS + 1];
  static u32 temp_config[QNAP_EC_NUMBER_OF_TEMP_CHANNELS + 1];
  static const struct hwmon_channel_info fan_channel_info = {
    .type = hwmon_fan,
    .config = fan_config
  };
  static const struct hwmon_channel_info pwm_channel_info = {
    .type = hwmon_pwm,
    .config = pwm_config
  };
  static const struct hwmon_channel_info temp_channel_info = {
    .type = hwmon_temp,
    .config = temp_config
  };
  static const struct hwmon_channel_info* hwmon_channel_info[] = { &fan_channel_info,
    &pwm_channel_info, &temp_channel_info, NULL };
  static const struct hwmon_ops hwmon_ops = {
    .is_visible = &qnap_ec_hwmon_is_visible,
    .read = &qnap_ec_hwmon_read,
    .write = &qnap_ec_hwmon_write
  };
  static const struct hwmon_chip_info hwmon_chip_info = {
    .info = hwmon_channel_info,
    .ops = &hwmon_ops
  };

  // Declare needed variables
  uint8_t i;
  struct qnap_ec_data* data;
  struct device* device;

  // Allocate device managed memory for the data structure
  data = devm_kzalloc(&platform_dev->dev, sizeof(struct qnap_ec_data), GFP_KERNEL);
  if (data == NULL)
    return -ENOMEM;

  // Initialize the data mutex, set the devices pointer, and if we are simulating the PWM enable
  //   attribute set the PWM enable values
  mutex_init(&data->mutex);
  data->devices = qnap_ec_devices;
  if (qnap_ec_sim_pwm_enable)
    for (i = 0; i < QNAP_EC_NUMBER_OF_PWM_CHANNELS / 8; ++i)
      data->pwm_enable_value_field[i] = 0xFF;

  // Set the custom device data to the data structure
  // Note: this needs to be done before registering the hwmon device so that the data is accessible
  //       in the qnap_ec_is_visible function which is called when the hwmon device is registered
  dev_set_drvdata(&platform_dev->dev, data);

  // Populate the fan configuration array
  for (i = 0; i < QNAP_EC_NUMBER_OF_FAN_CHANNELS; ++i)
    fan_config[i] = HWMON_F_INPUT;
  fan_config[i] = 0;

  // Populate the PWM configuration array
  if (qnap_ec_sim_pwm_enable)
    for (i = 0; i < QNAP_EC_NUMBER_OF_PWM_CHANNELS; ++i)
      pwm_config[i] = HWMON_PWM_INPUT | HWMON_PWM_ENABLE;
  else
    for (i = 0; i < QNAP_EC_NUMBER_OF_PWM_CHANNELS; ++i)
      pwm_config[i] = HWMON_PWM_INPUT;
  pwm_config[i] = 0;

  // Populate the temperature configuration array
  for (i = 0; i < QNAP_EC_NUMBER_OF_TEMP_CHANNELS; ++i)
    temp_config[i] = HWMON_T_INPUT;
  temp_config[i] = 0;

  // Register the hwmon device and pass in the data structure
  // Note: hwmon device name cannot contain dashes
  device = devm_hwmon_device_register_with_info(&platform_dev->dev, "qnap_ec", data,
    &hwmon_chip_info, NULL);
  if (device == NULL)
    return -ENOMEM;

  return 0;
}

// Function called to check if a hwmon attribute is visible
static umode_t qnap_ec_hwmon_is_visible(const void* const_data, enum hwmon_sensor_types type,
                                        u32 attribute, int channel)
{
  // Declare and/or define needed variables
  // Note: we are using the dev_get_drvdata function to get access to the data since we need a
  //       pointer to non constant data
  struct qnap_ec_data* data = dev_get_drvdata(&((const struct qnap_ec_data*)const_data)->devices->
    plat_device->dev);

  // Switch based on the sensor type
  // Note: we are using a switch statements to simplify possible future expansion
  switch (type)
  {
    case hwmon_fan:
      // Switch based on the sensor attribute
      switch (attribute)
      {
        case hwmon_fan_input:
          // Check if this fan channel is valid and make the input attribute read only
          if (qnap_ec_is_fan_channel_valid(data, channel))
            return S_IRUGO;

          break;
      }
      break;
    case hwmon_pwm:
      // Switch based on the sensor attribute
      switch (attribute)
      {
        case hwmon_pwm_enable:
          // Check if we are simulating the enable attribute and if this PWM channel is valid and
          //   make the enable attribute read/write
          if (qnap_ec_sim_pwm_enable && qnap_ec_is_pwm_channel_valid(data, channel))
            return S_IRUGO | S_IWUSR;

          break;
        case hwmon_pwm_input:
          // Check if this PWM channel is valid and make the input attribute read/write
          if (qnap_ec_is_pwm_channel_valid(data, channel))
            return S_IRUGO | S_IWUSR;

          break;
      }
      break;
    case hwmon_temp:
      // Switch based on the sensor attribute
      switch (attribute)
      {
        case hwmon_temp_input:
          // Check if this temperature input channel is valid and make it read only
          if (qnap_ec_is_temp_channel_valid(data, channel))
            return S_IRUGO;

          break;
      }
      break;
    // Dummy default cause to silence compiler warnings about not including all enums in switch
    //   statement
    default:
      break;
  }

  return 0;
}

// Function called to read from a hwmon attribute
static int qnap_ec_hwmon_read(struct device* device, enum hwmon_sensor_types type, u32 attribute,
                              int channel, long* value)
{
  // Declare and/or define needed variables
  uint32_t fan_speed;
  uint32_t fan_pwm;
  int64_t temperature;
  struct qnap_ec_data* data = dev_get_drvdata(device);

  // Switch based on the sensor type
  // Note: we are using a switch statements to simplify possible future expansion
  switch (type)
  {
    case hwmon_fan:
      // Switch based on the sensor attribute
      switch (attribute)
      {
        case hwmon_fan_input:
          // Check if this fan channel is invalid
          if (!qnap_ec_is_fan_channel_valid(data, channel))
            return -EOPNOTSUPP;

          // Call the ec_sys_get_fan_speed function in the libuLinux_hal library
          if (qnap_ec_call_lib_function(true, data, int8_func_uint8_uint32pointer,
              "ec_sys_get_fan_speed", channel, NULL, &fan_speed, NULL, true) != 0)
            return -ENODATA;
  
          // Set the value to the returned fan speed
          *value = fan_speed;

          break;
        default:
          return -EOPNOTSUPP;
      }

      break;
    case hwmon_pwm:
      // Switch based on the sensor attribute
      switch (attribute)
      {
        case hwmon_pwm_enable:
          // Check if we are not simulating the PWM enable attribute or this PWM channel is invalid
          if (!qnap_ec_sim_pwm_enable || !qnap_ec_is_pwm_channel_valid(data, channel))
            return -EOPNOTSUPP;

          // Set the value to the PWM enable value
          *value = (data->pwm_enable_value_field[channel / 8] >> (channel % 8)) & 0x01;

          break;
        case hwmon_pwm_input:
          // Check if this PWM channel is invalid
          if (!qnap_ec_is_pwm_channel_valid(data, channel))
            return -EOPNOTSUPP;

          // Call the ec_sys_get_fan_pwm function in the libuLinux_hal library
          if (qnap_ec_call_lib_function(true, data, int8_func_uint8_uint32pointer,
              "ec_sys_get_fan_pwm", channel, NULL, &fan_pwm, NULL, true) != 0)
            return -ENODATA;

          // Set the value to the returned fan PWM
          *value = fan_pwm;

          break;
        default:
          return -EOPNOTSUPP;
      }

      break;
    case hwmon_temp:
      // Switch based on the sensor attribute
      switch (attribute)
      {
        case hwmon_temp_input:
          // Check if this temperature channel is invalid
          if (!qnap_ec_is_temp_channel_valid(data, channel))
            return -EOPNOTSUPP;

          // Call the ec_sys_get_temperature function in the libuLinux_hal library
          // Note: we are using an int64 variable instead of a double variable because floating
          //       point math (including casting) is not possible in kernel space
          if (qnap_ec_call_lib_function(true, data, int8_func_uint8_doublepointer,
              "ec_sys_get_temperature", channel, NULL, NULL, &temperature, true) != 0)
            return -ENODATA;

          // Set the value to the returned temperature
          // Note: because an int64 variable can hold a 19 digit integer while a double variable can
          //       hold a 16 digit integer without loosing precision in the helper program we
          //       multiple the double value by 1000 to move three digits after the decimal point to
          //       before the decimal point and still fit the value in an int64 variable and
          //       preserve three digits after the decimal point however because we need to return a
          //       millidegree value there is no need to divide by 1000
          *value = temperature;

          break;
        default:
          return -EOPNOTSUPP;
      }

      break;
    default:
      return -EOPNOTSUPP;
  }

  return 0;
}

// Function called to write to a hwmon atrribute
static int qnap_ec_hwmon_write(struct device* device, enum hwmon_sensor_types type, u32 attribute,
                               int channel, long value)
{
  // Declare and/or define needed variables
  uint8_t fan_pwm = value;
  struct qnap_ec_data* data = dev_get_drvdata(device);

  // Switch based on the sensor type
  // Note: we are using a switch statement to simplify possible future expansion
  switch (type)
  {
    case hwmon_pwm:
      // Switch based on the sensor attribute
      switch (attribute)
      {
        case hwmon_pwm_enable:
          // Check if we are not simulating the pwm enable attribute or this PWM channel is invalid
          if (!qnap_ec_sim_pwm_enable || !qnap_ec_is_pwm_channel_valid(data, channel))
            return -EOPNOTSUPP;

          // Check if the value is invalid
          if (value < 0 || value > 1)
            return -EOPNOTSUPP;

          // Check if the value is 0
          if (value == 0)
          {
            // Set the PWM enable value
            data->pwm_enable_value_field[channel / 8] &= ~(0x01 << (channel % 8));

            // Set the fan PWM to full and call the ec_sys_set_fan_speed function in the
            //   libuLinux_hal library
            fan_pwm = 255;
            if (qnap_ec_call_lib_function(true, data, int8_func_uint8_uint8,
                "ec_sys_set_fan_speed", channel, &fan_pwm, NULL, NULL, true) != 0)
              return -EOPNOTSUPP;
          }
          else // if (value == 1)
          {
            // Set the PWM enable value
            data->pwm_enable_value_field[channel / 8] |= (0x01 << (channel % 8));
          }

          break;
        case hwmon_pwm_input:
          // Check if this PWM channel is invalid or if we are simulating the PWM enable attribute
          //   and fan PWM is not enabled for this channel
          if (!qnap_ec_is_pwm_channel_valid(data, channel) || (qnap_ec_sim_pwm_enable &&
             ((data->pwm_enable_value_field[channel / 8] >> (channel % 8)) & 0x01) == 0))
            return -EOPNOTSUPP;

          // Check if the value is invalid
          if (value < 0 || value > 255)
            return -EOVERFLOW;

          // Call the ec_sys_set_fan_speed function in the libuLinux_hal library
          if (qnap_ec_call_lib_function(true, data, int8_func_uint8_uint8, "ec_sys_set_fan_speed",
              channel, &fan_pwm, NULL, NULL, true) != 0)
            return -EOPNOTSUPP;

          break;
        default:
          return -EOPNOTSUPP;
      }

      break;
    default:
      return -EOPNOTSUPP;
  }

  return 0;
}

// Function called to check if the fan channel number is valid
static bool qnap_ec_is_fan_channel_valid(struct qnap_ec_data* data, uint8_t channel)
{
  // Note: based on testing the logic to determining if a fan channel is valid is:
  //       - call the ec_sys_get_fan_status function in the libuLinux_hal library
  //       - if the function return value is non zero then the channel is invalid
  //       - if the returned fan status is non zero then the channel is invalid
  //       - call the ec_sys_get_fan_speed function in the libuLinux_hal library
  //       - if the function return value is non zero then the channel is invalid
  //       - if the returned fan speed is 65535 then the channel is invalid
  //       - call the ec_sys_get_fan_pwm function in the libuLinux_hal library
  //       - if the function return value is non zero then the channel is invalid
  //       - if the returned fan PWM is greater than 255 then the channel is invalid
  //       - mark the channel as valid

  // Declare needed variables
  uint32_t fan_status;
  uint32_t fan_speed;
  uint32_t fan_pwm;

  // Check if this channel has already been checked
  if (((data->fan_channel_checked_field[channel / 8] >> (channel % 8)) & 0x01) == 1)
  {
    // Check if this channel is invalid
    if (((data->fan_channel_valid_field[channel / 8] >> (channel % 8)) & 0x01) == 0)
      return false;

    return true;
  }

  // Get the data mutex lock
  mutex_lock(&data->mutex);

  // Set the fan status to an invalid value (to verify that the called function changed the value)
  //   and call the ec_sys_get_fan_status function in the libuLinux_hal library
  fan_status = 1;
  if (qnap_ec_call_lib_function(false, data, int8_func_uint8_uint32pointer, "ec_sys_get_fan_status",
      channel, NULL, &fan_status, NULL, false) != 0)
  {
    // Mark this channel as checked (and invalid by default)
    data->fan_channel_checked_field[channel / 8] |= (0x01 << (channel % 8));

    // Release the data mutex lock
    mutex_unlock(&data->mutex);

    return false;
  }

  // Check if the returned status is non zero
  if (fan_status != 0)
  {
    // Mark this channel as checked (and invalid by default)
    data->fan_channel_checked_field[channel / 8] |= (0x01 << (channel % 8));

    // Release the data mutex lock
    mutex_unlock(&data->mutex);

    return false;
  }

  // Set the fan speed to an invalid value (to verify that the called function changed the value)
  //   and call the ec_sys_get_fan_speed function in the libu_Linux_hal library
  fan_speed = 65535;
  if (qnap_ec_call_lib_function(false, data, int8_func_uint8_uint32pointer, "ec_sys_get_fan_speed",
      channel, NULL, &fan_speed, NULL, false) != 0)
  {
    // Mark this channel as checked (and invalid by default)
    data->fan_channel_checked_field[channel / 8] |= (0x01 << (channel % 8));

    // Release the data mutex lock
    mutex_unlock(&data->mutex);

    return false;
  }

  // Check if the returned fan speed is 65535
  if (fan_speed == 65535)
  {
    // Mark this channel as checked (and invalid by default)
    data->fan_channel_checked_field[channel / 8] |= (0x01 << (channel % 8));

    // Release the data mutex lock
    mutex_unlock(&data->mutex);

    return false;
  }

  // Set the fan PWM to an invalid value (to verify that the called function changed the value)
  //   and call the ec_sys_get_fan_pwm function in the libuLinux_hal library
  fan_pwm = 256;
  if (qnap_ec_call_lib_function(false, data, int8_func_uint8_uint32pointer, "ec_sys_get_fan_pwm",
      channel, NULL, &fan_pwm, NULL, false) != 0)
  {
    // Mark this channel as checked (and invalid by default)
    data->fan_channel_checked_field[channel / 8] |= (0x01 << (channel % 8));

    // Release the data mutex lock
    mutex_unlock(&data->mutex);

    return false;
  }

  // Check if the returned fan PWM is greater than 255
  if (fan_pwm > 255)
  {
    // Mark this channel as checked (and invalid by default)
    data->fan_channel_checked_field[channel / 8] |= (0x01 << (channel % 8));

    // Release the data mutex lock
    mutex_unlock(&data->mutex);

    return false;
  }

  // Mark this channel as checked and valid
  data->fan_channel_checked_field[channel / 8] |= (0x01 << (channel % 8));
  data->fan_channel_valid_field[channel / 8] |= (0x01 << (channel % 8));

  // Release the data mutex lock
  mutex_unlock(&data->mutex);

  return true;
}

// Function called to check if the PWM channel number is valid
static bool qnap_ec_is_pwm_channel_valid(struct qnap_ec_data* data, uint8_t channel)
{
  // Note: based on testing the logic to determine if a PWM channel is valid is:
  //       - loop through all channels that have not been checked and read the initial fan PWM by
  //         calling the ec_sys_get_fan_speed function in the libuLinux_hal library
  //       - if the function return value is non zero for a channel then the channel is invalid
  //       - if the returned fan PWM for a channel is greater than 255 then the channel is invalid
  //       - call the ec_sys_set_fan_speed function in the libuLinux_hal library to
  //         increase/decrease the fan PWM
  //       - if the function return value is non zero then the channel is invalid
  //       - loop through all channels that have not been checked and have the same initial fan PWM
  //         as the channel being validated and read the changed fan PWM by calling the
  //         ec_sys_get_fan_speed function in the libuLinux_hal library
  //       - if the function return value is non zero for a channel then the channel is invalid
  //       - if the returned fan PWM for a channel is greater than 255 then the channel is invalid
  //       - call the ec_sys_set_fan_speed function in the libuLinux_hal library to reset the fan
  //         PWM
  //       - if the function return value is non zero then the channel is invalid
  //       - loop through all channels that have not been checked and have the same initial fan PWM
  //         as the channel being validated and have the same changed fan PWM as the channel being
  //         validated and mark the lowest numerical channel as valid and mark the other
  //         channels as invalid

  // Declare and/or define needed variables
  uint8_t i;
  uint8_t fan_pwm;
  uint32_t fan_speed;
  bool valid_channel_marked = false;
  uint8_t initial_fan_pwms[QNAP_EC_NUMBER_OF_PWM_CHANNELS];
  uint8_t changed_fan_pwms[QNAP_EC_NUMBER_OF_PWM_CHANNELS];

  // Check if we should not be validating PWM channels and should mimic the fan channels
  if (!qnap_ec_val_pwm_channels)
    return qnap_ec_is_fan_channel_valid(data, channel);

  // Check if this channel has already been checked
  if (((data->pwm_channel_checked_field[channel / 8] >> (channel % 8)) & 0x01) == 1)
  {
    // Check if this channel is invalid
    if (((data->pwm_channel_valid_field[channel / 8] >> (channel % 8)) & 0x01) == 0)
      return false;

    return true;
  }

  // Get the data mutex lock
  mutex_lock(&data->mutex);

  // Read the intial fan PWMs
  if (qnap_ec_is_pwm_channel_valid_read_fan_pwms(data, channel, initial_fan_pwms, NULL) != 0)
  {
    // Mark this channel as checked (and invalid by default)
    data->pwm_channel_checked_field[channel / 8] |= (0x01 << (channel % 8));

    // Release the data mutex lock
    mutex_unlock(&data->mutex);

    return false;
  }

  // Check if the initial fan PWM is less than or equal to 250 and change the fan PWM accordingly
  if (initial_fan_pwms[channel] <= 250)
    fan_pwm = initial_fan_pwms[channel] + 5;
  else
    fan_pwm = initial_fan_pwms[channel] - 5;

  // Call the ec_sys_set_fan_speed function in the libuLinux_hal library
  if (qnap_ec_call_lib_function(false, data, int8_func_uint8_uint8, "ec_sys_set_fan_speed", channel,
      &fan_pwm, NULL, NULL, false) != 0)
  {
    // Mark this channel as checked (and invalid by default)
    data->pwm_channel_checked_field[channel / 8] |= (0x01 << (channel % 8));

    // Release the data mutex lock
    mutex_unlock(&data->mutex);

    return false;
  }

  // Read the changed fan PWMs
  if (qnap_ec_is_pwm_channel_valid_read_fan_pwms(data, channel, initial_fan_pwms,
      changed_fan_pwms) != 0)
  {
    // Mark this channel as checked (and invalid by default)
    data->pwm_channel_checked_field[channel / 8] |= (0x01 << (channel % 8));

    // Release the data mutex lock
    mutex_unlock(&data->mutex);

    return false;
  }

  // Check if the fan PWM didn't actually change
  if (initial_fan_pwms[channel] == changed_fan_pwms[channel])
  {
    // Mark this channel as checked (and invalid by default)
    data->pwm_channel_checked_field[channel / 8] |= (0x01 << (channel % 8));

    // Release the data mutex lock
    mutex_unlock(&data->mutex);

    return false;
  }

  // Set the fan PWM to the initial fan PWM and call the ec_sys_set_fan_speed function in the
  //   libuLinux_hal library
  fan_pwm = initial_fan_pwms[channel];
  if (qnap_ec_call_lib_function(false, data, int8_func_uint8_uint8, "ec_sys_set_fan_speed", channel,
      &fan_pwm, NULL, NULL, false) != 0)
  {
    // Mark this channel as checked (and invalid by default)
    data->pwm_channel_checked_field[channel / 8] |= (0x01 << (channel % 8));

    // Release the data mutex lock
    mutex_unlock(&data->mutex);

    return false;
  }

  // Loop through all the channels
  for (i = 0; i < QNAP_EC_NUMBER_OF_PWM_CHANNELS; ++i)
  {
    // Check if this channel has already been checked
    if (((data->pwm_channel_checked_field[i / 8] >> (i % 8)) & 0x01) == 1)
      continue;

    // Check if this channel does not have the same intial fan PWM as the channel being validated
    if (initial_fan_pwms[i] != initial_fan_pwms[channel])
      continue;


    // Check if this channel does not have the same changed fan PWM as the channel being validated
    if (changed_fan_pwms[i] != changed_fan_pwms[channel])
      continue;

    // Mark this channel as checked
    data->pwm_channel_checked_field[i / 8] |= (0x01 << (i % 8));

    // Check if a valid channel has not yet been marked in this group
    if (!valid_channel_marked)
    {
      // Set the fan speed to an invalid value (to verify that the called function changed the
      //   value) and call the ec_sys_get_fan_speed function in the libu_Linux_hal library
      fan_speed = 65535;
      if (qnap_ec_call_lib_function(false, data, int8_func_uint8_uint32pointer,
          "ec_sys_get_fan_speed", i, NULL, &fan_speed, NULL, false) != 0)
        continue;

      // Check if the returned fan speed is 65535
      if (fan_speed == 65535)
        continue;

      // Mark this channel as valid
      data->pwm_channel_valid_field[i / 8] |= (0x01 << (i % 8));

      // Set the valid channel marked flag
      valid_channel_marked = true;
    }
  }

  // Release the data mutex lock
  mutex_unlock(&data->mutex);

  // Check if this channel is invalid
  if (((data->pwm_channel_valid_field[channel / 8] >> (channel % 8)) & 0x01) == 0)
    return false;

  return true;
}

// Function called by the qnap_ec_is_pwm_channel_valid function to read the fan PWMs
static int qnap_ec_is_pwm_channel_valid_read_fan_pwms(struct qnap_ec_data* data, uint8_t channel,
                                                      uint8_t initial_fan_pwms[],
                                                      uint8_t changed_fan_pwms[])
{
  // Declare needed variables
  uint8_t i;
  uint8_t j;
  uint32_t fan_pwm;

  // Loop through all the channels starting at the channel being validated
  for (i = 0, j = channel; i < QNAP_EC_NUMBER_OF_PWM_CHANNELS; ++i, j = (j + 1) %
       QNAP_EC_NUMBER_OF_PWM_CHANNELS)
  {
    // Check if this channel has already been checked
    if (((data->pwm_channel_checked_field[j / 8] >> (j % 8)) & 0x01) == 1)
      continue;

    // Check if the changed fan PWMs pointer is not NULL (ie: this is the second read) and if
    //   this channel does not have the same initial fan PWM as the channel being validated
    if (changed_fan_pwms != NULL && initial_fan_pwms[j] != initial_fan_pwms[channel])
      continue;

    // Set the fan PWM to an invalid value (to verify that the called function changed the value)
    //   and call the ec_sys_get_fan_pwm function in the libuLinux_hal library
    fan_pwm = 256;
    if (qnap_ec_call_lib_function(false, data, int8_func_uint8_uint32pointer, "ec_sys_get_fan_pwm",
        j, NULL, &fan_pwm, NULL, false) != 0)
    {
      // Check if this is the channel that is being validated and we should return instead of
      //   continuing in the loop
      if (j == channel)
        return -ENODATA;

      // Mark this channel as checked (and invalid by default)
      data->pwm_channel_checked_field[j / 8] |= (0x01 << (j % 8));
      
      continue;
    }

    // Check if the returned fan PWM is greater than 255
    if (fan_pwm > 255)
    {
      // Check if this is the channel that is being validated and we should return instead of
      //   continuing in the loop
      if (j == channel)
        return -ENODATA;

      // Mark this channel as checked (and invalid by default)
      data->pwm_channel_checked_field[j / 8] |= (0x01 << (j % 8));
      
      continue;
    }

    // Check if the changed fan PWMs pointer is NULL (ie: this is the first read) and use the
    //   appropriate array for this channel
    if (changed_fan_pwms == NULL)
      initial_fan_pwms[j] = fan_pwm;
    else
      changed_fan_pwms[j] = fan_pwm;
  }

  return 0;
}

// Function called to check if the temperature channel number is valid
static bool qnap_ec_is_temp_channel_valid(struct qnap_ec_data* data, uint8_t channel)
{
  // Note: based on testing the logic to determining if a temperature channel is valid is:
  //       - call the ec_sys_get_temperature function in the libuLinux_hal library
  //       - if the function return value is non zero then the channel is invalid
  //       - if the returned temperature is negative then the channel is invalid
  //       - mark the channel as valid

  // Declare any needed variables
  int64_t temperature;

  // Check if this channel has already been checked
  if (((data->temp_channel_checked_field[channel / 8] >> (channel % 8)) & 0x01) == 1)
  {
    // Check if this channel is invalid
    if (((data->temp_channel_valid_field[channel / 8] >> (channel % 8)) & 0x01) == 0)
      return false;

    return true;
  }

  // Get the data mutex lock
  mutex_lock(&data->mutex);

  // Set the temperature to an invalid value (to verify that the called function changed the value)
  //   and call the ec_sys_get_temperature function in the libuLinux_hal library
  temperature = -1;
  if (qnap_ec_call_lib_function(false, data, int8_func_uint8_doublepointer,
      "ec_sys_get_temperature", channel, NULL, NULL, &temperature, false) != 0)
  {
    // Mark this channel as checked (and invalid by default)
    data->temp_channel_checked_field[channel / 8] |= (0x01 << (channel % 8));

    // Release the data mutex lock
    mutex_unlock(&data->mutex);

    return false;
  }

  // Check if the returned temperature is negative
  if (temperature < 0)
  {
    // Mark this channel as checked (and invalid by default)
    data->temp_channel_checked_field[channel / 8] |= (0x01 << (channel % 8));

    // Release the data mutex lock
    mutex_unlock(&data->mutex);

    return false;
  }

  // Mark this channel as checked and valid
  data->temp_channel_checked_field[channel / 8] |= (0x01 << (channel % 8));
  data->temp_channel_valid_field[channel / 8] |= (0x01 << (channel % 8));

  // Release the data mutex lock
  mutex_unlock(&data->mutex);

  return true;
}

// Function called to call a function in the libuLinux_hal library via the user space helper program
// Note: the return value is the call_usermodehelper function's error code if an error code was
//       returned or if successful the return value is the user space helper program's error code
//       if an error code was returned or is the return value is the libuLinux_hal library
//       function's error code if an error code was return or if successful the return value is zero
// Note: we are using an int64 function argument in place of a double since floating point math
//       (including casting) is not possible in kernel space
static int qnap_ec_call_lib_function(bool use_mutex, struct qnap_ec_data* data,
                                     enum qnap_ec_ioctl_function_type function_type,
                                     char* function_name, uint8_t argument1_uint8,
                                     uint8_t* argument2_uint8, uint32_t* argument2_uint32,
                                     int64_t* argument2_int64, bool log_return_error)
{
  // Declare and/or define needed variables
  uint8_t i;
  int return_value;
#ifdef PACKAGE
  char* paths[] = { "/usr/sbin/qnap-ec", "/usr/bin/qnap-ec", "/sbin/qnap-ec", "/bin/qnap-ec" };
#else
  char* paths[] = { "/usr/local/sbin/qnap-ec", "/usr/local/bin/qnap-ec", "/usr/sbin/qnap-ec",
    "/usr/bin/qnap-ec", "/sbin/qnap-ec", "/bin/qnap-ec" };
#endif

  // Check if we should use the mutex and get the data mutex lock
  if (use_mutex)
    mutex_lock(&data->mutex);

  // Set the I/O control command structure fields for calling the function in the libuLinux_hal
  //   library via the helper program
  // Note: "sizeof(((struct qnap_ec_ioctl_command*)0)->function_name)" statement is based on the
  //       FIELD_SIZEOF macro which was removed from the kernel
  data->ioctl_command.function_type = function_type;
  strncpy(data->ioctl_command.function_name, function_name,
    sizeof(((struct qnap_ec_ioctl_command*)0)->function_name) - 1);
  data->ioctl_command.argument1_uint8 = argument1_uint8;
  if (argument2_uint8 != NULL)
    data->ioctl_command.argument2_uint8 = *argument2_uint8;
  if (argument2_uint32 != NULL)
    data->ioctl_command.argument2_uint32 = *argument2_uint32;
  if (argument2_int64 != NULL)
    data->ioctl_command.argument2_int64 = *argument2_int64;

  // Set the open device flag to allow return communication by the helper program
  data->devices->open_misc_device = true;

  // Call the user space helper program and loop through the paths while the first 8 bits of the
  //   return value contain any error codes
  i = 0;
  do
    return_value = call_usermodehelper(paths[i], (char*[]){ paths[i], NULL }, NULL, UMH_WAIT_PROC);
  while ((return_value & 0xFF) != 0 && ++i < sizeof(paths) / sizeof(char*));

  // Check if the first 8 bits of the return value contain any error codes
  if ((return_value & 0xFF) != 0)
  {
    // Log the error
#ifdef PACKAGE
    pr_err("qnap-ec helper program not found at the expected path (%s) or any of the fall back "
      "paths (%s, %s, %s)", paths[0], paths[1], paths[2], paths[3]);
#else
    pr_err("qnap-ec helper program not found at the expected path (%s) or any of the fall back "
      "paths (%s, %s, %s, %s, %s)", paths[0], paths[1], paths[2], paths[3], paths[4], paths[5]);
#endif

    // Clear the open device flag
    data->devices->open_misc_device = false;

    // Check if we are using the mutex and release the data mutex lock
    if (use_mutex)
      mutex_unlock(&data->mutex);

    // Return the call_usermodehelper function's error code
    return return_value & 0xFF;
  }

  // Check if the user space helper program's return value stored in the second 8 bits of the
  //   return value contain any error codes
  if (((return_value >> 8) & 0xFF) != 0)
  {
    // Log the error
    // Note: the sign (+/-) of the user space helper program's error code is not returned by the
    //       call_usermodehelper function
    pr_err("qnap-ec helper program exited with a non zero exit code (+/-%i)",
      ((return_value >> 8) & 0xFF));

    // Clear the open device flag
    data->devices->open_misc_device = false;

    // Check if we are using the mutex and release the data mutex lock
    if (use_mutex)
      mutex_unlock(&data->mutex);

    // Return the user space helper program's error code
    return (return_value >> 8) & 0xFF;
  }

  // Clear the open device flag
  data->devices->open_misc_device = false;

  // Check if the called function returned any errors
  if (data->ioctl_command.return_value_int8 != 0)
  {
    // Check if we should log the function return error code error and log the error
    if (log_return_error)
      pr_err("libuLinux_hal library %s function called by qnap-ec helper program returned a non "
        "zero value (%i)", data->ioctl_command.function_name,
        data->ioctl_command.return_value_int8);

    // Check if we are using the mutex and release the data mutex lock
    if (use_mutex)
      mutex_unlock(&data->mutex);

    // Return the function's error code
    return data->ioctl_command.return_value_int8;
  }

  // Save any changes to the various arguments
  if (argument2_uint32 != NULL)
    *argument2_uint32 = data->ioctl_command.argument2_uint32;
  if (argument2_int64 != NULL)
    *argument2_int64 = data->ioctl_command.argument2_int64;

  // Check if we are using the mutex and release the data mutex lock
  if (use_mutex)
    mutex_unlock(&data->mutex);

  return 0;
}

// Function called when the miscellaneous device is openeded
static int qnap_ec_misc_device_open(struct inode* inode, struct file* file)
{
  // Declare and/or define needed variables
  struct qnap_ec_devices* devices = container_of(file->private_data, struct qnap_ec_devices,
    misc_device);

  // Check if the open device flag is not set which means we are not expecting any communications
  if (devices->open_misc_device == false)
    return -EBUSY;

  // Try to lock the miscellaneous device mutex if it's currently unlocked
  // Note: if the mutex is currently locked it means we are already communicating and this is an
  //       unexpected communication
  if (mutex_trylock(&devices->misc_device_mutex) == 0)
    return -EBUSY;

  return 0;
}

// Function called when the miscellaneous device receives a I/O control command
static long int qnap_ec_misc_device_ioctl(struct file* file, unsigned int command,
                                          unsigned long argument)
{
  // Declare and/or define needed variables
  // Note: the following statement is a combination of the following two statements:
  //       struct qnap_ec_devices* devices = container_of(file->private_data,
  //         struct qnap_ec_devices, misc_device);
  //       struct qnap_ec_data* data = dev_get_drvdata(&devices->plat_device->dev);
  struct qnap_ec_data* data = dev_get_drvdata(&container_of(file->private_data,
    struct qnap_ec_devices, misc_device)->plat_device->dev);

  // Swtich based on the command
  switch (command)
  {
    case QNAP_EC_IOCTL_CALL:
      // Make sure we can write the data to user space
      if (access_ok(argument, sizeof(struct qnap_ec_ioctl_command)) == 0)
        return -EFAULT;

      // Copy the I/O control command data from the data structure to the user space
      if (copy_to_user((void*)argument, &data->ioctl_command,
          sizeof(struct qnap_ec_ioctl_command)) != 0)
        return -EFAULT;
  
      break;
    case QNAP_EC_IOCTL_RETURN:
      // Make sure we can read the data from user space
      if (access_ok(argument, sizeof(struct qnap_ec_ioctl_command)) == 0)
        return -EFAULT;
  
      // Copy the I/O control command data from the user space to the data structure
      if (copy_from_user(&data->ioctl_command, (void*)argument,
          sizeof(struct qnap_ec_ioctl_command)) != 0)
        return -EFAULT;

      break;
    default:
      return -EINVAL;
  }

  return 0;
}

// Function called when the miscellaneous device is released
static int qnap_ec_misc_device_release(struct inode* inode, struct file* file)
{
  // Declare and/or define needed variables
  struct qnap_ec_devices* devices = container_of(file->private_data, struct qnap_ec_devices,
    misc_device);

  // Release the miscellaneous device mutex lock
  mutex_unlock(&devices->misc_device_mutex);

  return 0;
}

// Function called to exit the driver
static void __exit qnap_ec_exit(void)
{
  // Unregister the platform device and free the platform device structure memory
  // Note: we are using the platform_device_unregister function instead of the platform_device_put
  //       function used in the qnap_ec_init function because all other hwmon drivers take this
  //       approach
  platform_device_unregister(qnap_ec_devices->plat_device);

  // Unregister the miscellaneous device
  misc_deregister(&qnap_ec_devices->misc_device);

  // Free the devices structure memory
  kfree(qnap_ec_devices);

  // Unregister the platform driver
  platform_driver_unregister(qnap_ec_plat_driver);

  // Free the platform driver structure memory
  kfree(qnap_ec_plat_driver);
}