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

#include <linux/acpi.h>
#include <linux/hwmon.h>
#include <linux/module.h>
#include <linux/platform_device.h>

#define QNAP_EC_ID_PORT_1 0x2E
#define QNAP_EC_ID_PORT_2 0x2F
#define QNAP_EC_COMM_PORT_1 0x68
#define QNAP_EC_COMM_PORT_2 0x6C

// Declare and/or define needed variables for overriding the detected device ID
static unsigned short qnap_ec_force_id;
module_param(qnap_ec_force_id, ushort, 0);
MODULE_PARM_DESC(qnap_ec_force_id, "Override the detected device ID");

uint8_t qnap_ec_call_helper(char* arguments)
{
  return 0;
}

umode_t qnap_ec_is_visible(const void* data, enum hwmon_sensor_types type, u32 attribute,
                           int channel)
{
  printk(KERN_INFO "qnap_ec_is_visible - %i - %i - %i", type, attribute, channel);
 
  // Switch based on the sensor type
  switch (type)
  {
    case hwmon_fan:
      // Switch based on the sensor attribute
      switch (attribute)
      {
        case hwmon_fan_input:
          // Make the fan input read only
          return S_IRUGO;
        default:
          return 0;
      }
    case hwmon_pwm:
      // Switch based on the sensor attribute
      switch (attribute)
      {
        case hwmon_pwm_input:
          // Make the PWM input read/write
          return S_IRUGO | S_IWUSR;
        default:
          return 0;
      }
    case hwmon_temp:
      // Switch based on the sensor attribute
      switch (attribute)
      {
        case hwmon_temp_input:
          // Make the temperature input read only
          return S_IRUGO;
        default:
          return 0;
      }
    default:
      return 0;
  }
}

int qnap_ec_read(struct device* dev, enum hwmon_sensor_types type, u32 attribute, int channel,
                 long* value)
{
  // Declare needed variables
  int return_value;
  char* arguments[3];
  char argument2[11];
  char* environment[] = {
    "PATH=/usr/local/sbin;/usr/local/bin;/usr/sbin;/usr/bin;/sbin;/bin"
  };

  // Fill in the first argument and third argument
  arguments[0] = "/usr/local/bin/qnap-ec";
  arguments[2] = NULL;

  printk(KERN_INFO "qnap_ec_read - %i - %i - %i", type, attribute, channel);

  // Switch based on the sensor type
  switch (type)
  {
    case hwmon_fan:
      // Switch based on the sensor attribute
      switch (attribute)
      {
        case hwmon_fan_input:
          // Create the second argument and add it to the arguments array
          if (scnprintf(argument2, 10, "--fan=%u", channel) < 7)
          {
            return -1;
          }
          arguments[1] = argument2;

          // Call the user space helper program and check if the first 8 bytes of the return value
          //   container any error codes
          return_value = call_usermodehelper(arguments[0], arguments, environment, UMH_WAIT_PROC);
          if (return_value & 0xFF)
          {
            return -1;
          }

          // Get the helper program return value
          return_value = (return_value >> 8) & 0xFF;

          // Process the return value

          // Set the value
          *value = return_value;

          return 0;
        default:
          return -EOPNOTSUPP;
      }
    case hwmon_pwm:
      // Switch based on the sensor attribute
      switch (attribute)
      {
        case hwmon_pwm_input:
          // Prepare the second argument and add it to the arguments array
          if (scnprintf(argument2, 10, "--pwm=%u", channel) < 7)
          {
            return 0;
          }
          arguments[1] = argument2;

          // Call the user space helper program and check if the first 8 bytes of the return value
          //   container any error codes
          return_value = call_usermodehelper(arguments[0], arguments, environment, UMH_WAIT_PROC);
          if (return_value & 0xFF)
          {
            return -1;
          }

          // Get the helper program return value
          return_value = (return_value >> 8) & 0xFF;

          // Process the return value

          // Set the value
          *value = return_value;

          return 0;

        default:
          return -EOPNOTSUPP;
      }
    case hwmon_temp:
      // Switch based on the sensor attribute
      switch (attribute)
      {
        case hwmon_temp_input:
          // Prepare the second argument and add it to the arguments array
          if (scnprintf(argument2, 10, "--temp=%u", channel) < 8)
          {
            return 0;
          }
          arguments[1] = argument2;

          // Call the user space helper program and check if the first 8 bytes of the return value
          //   container any error codes
          return_value = call_usermodehelper(arguments[0], arguments, environment, UMH_WAIT_PROC);
          if (return_value & 0xFF)
          {
            return -1;
          }

          // Get the helper program return value
          return_value = (return_value >> 8) & 0xFF;

          // Process the return value

          // Set the value
          *value = return_value;

          return 0;

        default:
          return -EOPNOTSUPP;
      }
    default:
      return -EOPNOTSUPP;
  }

  return 0;
}

int qnap_ec_write(struct device* dev, enum hwmon_sensor_types type, u32 attribute,
                        int channel, long value)
{
  printk(KERN_INFO "qnap_ec_write - %i - %i - %i", type, attribute, channel);

  // Switch based on the sensor type
  switch (type)
  {
    case hwmon_pwm:
      // Switch based on the sensor attribute
      switch (attribute)
      {
        case hwmon_pwm_input:
          return 0;
        default:
          return -EOPNOTSUPP;
      }
    default:
      return -EOPNOTSUPP;
  }
}

// Define the hwmon operations
static const struct hwmon_ops qnap_ec_ops = {
  .is_visible = qnap_ec_is_visible,
  .read = qnap_ec_read,
  .write = qnap_ec_write
};

// Define the hwmon channel info
static const struct hwmon_channel_info* qnap_ec_channel_info[] = {
  // Note: the number of HWMON_F_INPUT entries must match the number of fans sensors readable by
  //       the qnap-ec program
  HWMON_CHANNEL_INFO(fan, HWMON_F_INPUT, HWMON_F_INPUT, HWMON_F_INPUT, HWMON_F_INPUT, HWMON_F_INPUT,
    HWMON_F_INPUT),
  // Note: the number of HWMON_PWM_INPUT entries must match the number of PWM sensors readable by
  //       the qnap-ec program
  HWMON_CHANNEL_INFO(pwm, HWMON_PWM_INPUT, HWMON_PWM_INPUT, HWMON_PWM_INPUT, HWMON_PWM_INPUT),
  // Note: the number of HWMON_T_INPUT entries must match the number of PWM sensors readable by
  //       the qnap-ec program
  HWMON_CHANNEL_INFO(temp, HWMON_T_INPUT, HWMON_T_INPUT, HWMON_T_INPUT, HWMON_T_INPUT,
    HWMON_T_INPUT),
  NULL
};

// Define the hwmon chip information
static const struct hwmon_chip_info qnap_ec_chip_info = {
  .ops = &qnap_ec_ops,
  .info = qnap_ec_channel_info
};

// Function called to probe this driver
static int qnap_ec_probe(struct platform_device* platform_dev)
{
  // Declare needed variables
  struct device* dev;

  // Register the hwmon device
  dev = devm_hwmon_device_register_with_info(&platform_dev->dev, "qnap_ec", NULL,
    &qnap_ec_chip_info, NULL);

  return PTR_ERR_OR_ZERO(dev);
}

// Declare the global platform device
static struct platform_device* qnap_ec_platform_dev;

// Define the global platform driver
// Note: dashes are not allowed for driver names
static struct platform_driver qnap_ec_platform_drv = {
  .driver = {
    .name = "qnap_ec"
  },
  .probe = qnap_ec_probe
};

// Function called during driver initialization to add the device
static int __init qnap_ec_add_device(void)
{
  // Declare and/or define needed variables
  int error;
  struct resource resources[2] = {
    {
      .start = QNAP_EC_COMM_PORT_1,
      .end = QNAP_EC_COMM_PORT_1,
      .flags  = IORESOURCE_IO,
    },
    {
      .start = QNAP_EC_COMM_PORT_2,
      .end = QNAP_EC_COMM_PORT_2,
      .flags  = IORESOURCE_IO,
    }
  };

  // Create the platform device
  qnap_ec_platform_dev = platform_device_alloc("qnap_ec", 0);
  if (qnap_ec_platform_dev == 0)
  {
    return -ENOMEM;
  }

  // Set the resource structures name fields
  resources[0].name = qnap_ec_platform_dev->name;
  resources[1].name = qnap_ec_platform_dev->name;

  // Check if there is any resource conflict
  error = acpi_check_resource_conflict(resources);
  if (error)
  {
    platform_device_put(qnap_ec_platform_dev);
    return error;
  }

  // Add the device resources
  error = platform_device_add_resources(qnap_ec_platform_dev, resources, 2);
  if (error)
  {
    platform_device_put(qnap_ec_platform_dev);
    return error;
  }

  // Add the device
  error = platform_device_add(qnap_ec_platform_dev);
  if (error)
  {
    platform_device_put(qnap_ec_platform_dev);
    return error;
  }

  return 0;
}

// Function called during driver initialization to find the QNAP embedded controller
static int __init qnap_ec_find(void)
{
  return 0;
}

// Function called to initialize the driver
static int __init qnap_ec_init(void)
{
  // Declare needed variables
  int error;

  // Find the chip
  if (qnap_ec_find() != 0)
  {
    return -ENODEV;
  }

  // Register the driver
  error = platform_driver_register(&qnap_ec_platform_drv);
  if (error)
  {
    return error;
  }

  // Add the device
  error = qnap_ec_add_device();
  if (error)
  {
    platform_driver_unregister(&qnap_ec_platform_drv);
    return error;
  }

  return 0;
}

// Function called to exit the driver
static void __exit qnap_ec_exit(void)
{
  platform_device_unregister(qnap_ec_platform_dev);
  platform_driver_unregister(&qnap_ec_platform_drv);
}

// Specifiy the init and exit functions
module_init(qnap_ec_init);
module_exit(qnap_ec_exit);

MODULE_DESCRIPTION("QNAP EC Driver");
MODULE_AUTHOR("Stonyx - https://www.stonyx.com/");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:qnap-ec");