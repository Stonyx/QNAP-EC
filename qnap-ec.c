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

#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/hwmon.h>
#include <linux/ioctl.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/platform_device.h>
#include "qnap-ec.h"

// Define constants
#define QNAP_EC_ID_PORT_1 0x2E
#define QNAP_EC_ID_PORT_2 0x2F
#define QNAP_EC_COMM_PORT_1 0x68
#define QNAP_EC_COMM_PORT_2 0x6C

// Specify module details
MODULE_DESCRIPTION("QNAP EC Driver");
MODULE_AUTHOR("Stonyx - https://www.stonyx.com/");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:qnap-ec");

// Declare functions
static int __init qnap_ec_init(void);
static int __init qnap_ec_find(void);
static int __init qnap_ec_add_devices(void);
static int qnap_ec_probe(struct platform_device* platform_dev);
static umode_t qnap_ec_is_visible(const void* data, enum hwmon_sensor_types type, u32 attribute,
                                  int channel);
static int qnap_ec_read(struct device* dev, enum hwmon_sensor_types type, u32 attribute,
                        int channel, long* value);
static int qnap_ec_write(struct device* dev, enum hwmon_sensor_types type, u32 attribute,
                         int channel, long value);
static uint8_t qnap_ec_call_helper(void);
static long qnap_ec_ioctl(struct file* file, unsigned int command, unsigned long argument);
static void __exit qnap_ec_exit(void);

// Declare and/or define needed variables for overriding the detected device ID
static unsigned short qnap_ec_force_id;
module_param(qnap_ec_force_id, ushort, 0);
MODULE_PARM_DESC(qnap_ec_force_id, "Override the detected device ID");

// Define the mutex
DEFINE_MUTEX(qnap_ec_mutex);

// Define the IO control command

// Define the platform driver structure
static struct platform_driver qnap_ec_platform_drv = {
  .driver = {
    .name = "qnap_ec" // Dashes are not allowed for driver names
  },
  .probe = qnap_ec_probe
};

// Declare the character device major and minor numbers
static dev_t qnap_ec_char_dev_major_minor;

// Declare the character device structure pointer
static struct cdev* qnap_ec_char_dev;

// Declare the character device class structure pointer
static struct class* qnap_ec_char_dev_class;

// Define the character device file operations structure
static const struct file_operations qnap_ec_char_dev_ops = {
  .owner = THIS_MODULE,
  .unlocked_ioctl = qnap_ec_ioctl
};

// Declare the platform device structure pointer
static struct platform_device* qnap_ec_platform_dev;

// Define the hwmon channel info structures array pointer
static const struct hwmon_channel_info* qnap_ec_hwmon_channel_info[] = {
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

// Define the hwmon operations structure
static const struct hwmon_ops qnap_ec_hwmon_ops = {
  .is_visible = qnap_ec_is_visible,
  .read = qnap_ec_read,
  .write = qnap_ec_write
};

// Define the hwmon chip information structure
static const struct hwmon_chip_info qnap_ec_hwmon_chip_info = {
  .info = qnap_ec_hwmon_channel_info,
  .ops = &qnap_ec_hwmon_ops
};

// Specifiy the init and exit functions
module_init(qnap_ec_init);
module_exit(qnap_ec_exit);

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

  // Add the devices
  error = qnap_ec_add_devices();
  if (error)
  {
    platform_driver_unregister(&qnap_ec_platform_drv);
    return error;
  }

  return 0;
}

// Function called during driver initialization to find the QNAP embedded controller
static int __init qnap_ec_find(void)
{
  return 0;
}

// Function called during driver initialization to add the devices
// TODO: Possibly create qnap_ec_add_char_device and qnap_ec_add_hwmon_device functions
// TODO: Possibly move creating the character device class and device to the probe function
static int __init qnap_ec_add_devices(void)
{
  // Declare and/or define needed variables
  int error;
  struct device* device;

  // Allocate the character device major and minor numbers
  error = alloc_chrdev_region(&qnap_ec_char_dev_major_minor, 0, 1, "qnap-ec");
  if (error)
  {
    return error;
  }

  // Allocate the character device and populate required fields
  qnap_ec_char_dev = cdev_alloc();
  if (qnap_ec_char_dev == NULL)
  {
    // Unregister the character device major and minor numbers
    unregister_chrdev_region(qnap_ec_char_dev_major_minor, 1);

    return -ENOMEM;
  }
  qnap_ec_char_dev->owner = THIS_MODULE;
  qnap_ec_char_dev->ops = &qnap_ec_char_dev_ops;

  // Add the character device
  error = cdev_add(qnap_ec_char_dev, qnap_ec_char_dev_major_minor, 1);
  if (error)
  {
    // Delete the character device
    cdev_del(qnap_ec_char_dev);

    // Unregister the character device major and minor numbers
    unregister_chrdev_region(qnap_ec_char_dev_major_minor, 1);

    return error;
  }

  // Create the character device class
  qnap_ec_char_dev_class = class_create(THIS_MODULE, "qnap-ec");
  if (qnap_ec_char_dev_class == NULL)
  {
    // Delete the character device
    cdev_del(qnap_ec_char_dev);

    // Unregister the character device major and minor numbers
    unregister_chrdev_region(qnap_ec_char_dev_major_minor, 1);

    return -ENOMEM;
  }

  // Create the character device
  device = device_create(qnap_ec_char_dev_class, NULL, qnap_ec_char_dev_major_minor, NULL,
    "qnap-ec");
  if (device == NULL)
  {
    // Destroy the character device class
    class_destroy(qnap_ec_char_dev_class);

    // Delete the character device
    cdev_del(qnap_ec_char_dev);

    // Unregister the character device major and minor numbers
    unregister_chrdev_region(qnap_ec_char_dev_major_minor, 1);

    return -ENOMEM;
  }

  // Allocate the platform device
  // Note: name match the platform driver name which doesn't allow dashes
  qnap_ec_platform_dev = platform_device_alloc("qnap_ec", 0);
  if (qnap_ec_platform_dev == NULL)
  {
    // Destory the character device
    device_destroy(qnap_ec_char_dev_class, qnap_ec_char_dev_major_minor);

    // Destroy the character device class
    class_destroy(qnap_ec_char_dev_class);

    // Delete the character device
    cdev_del(qnap_ec_char_dev);

    // Unregister the character device major and minor numbers
    unregister_chrdev_region(qnap_ec_char_dev_major_minor, 1);

    return -ENOMEM;
  }

  // Add the platform device
  error = platform_device_add(qnap_ec_platform_dev);
  if (error)
  {
    // Put the platform device
    platform_device_put(qnap_ec_platform_dev);

    // Destory the character device
    device_destroy(qnap_ec_char_dev_class, qnap_ec_char_dev_major_minor);

    // Destroy the character device class
    class_destroy(qnap_ec_char_dev_class);

    // Delete the character device
    cdev_del(qnap_ec_char_dev);

    // Unregister the character device major and minor numbers
    unregister_chrdev_region(qnap_ec_char_dev_major_minor, 1);

    return error;
  }

  return 0;
}

// Function called to probe this driver
static int qnap_ec_probe(struct platform_device* platform_dev)
{
  // Declare needed variables
  struct device* device;

  // Register the hwmon device
  // Note: name matches the platform driver name which doesn't allow dashes
  device = devm_hwmon_device_register_with_info(&platform_dev->dev, "qnap_ec", NULL,
    &qnap_ec_hwmon_chip_info, NULL);
  if (device == NULL)
  {
    return -ENOMEM;
  }

  return 0;
}

static umode_t qnap_ec_is_visible(const void* data, enum hwmon_sensor_types type, u32 attribute,
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

static int qnap_ec_read(struct device* dev, enum hwmon_sensor_types type, u32 attribute,
                        int channel, long* value)
{
  printk(KERN_INFO "qnap_ec_read - %i - %i - %i", type, attribute, channel);

  // Switch based on the sensor type
  switch (type)
  {
    case hwmon_fan:
      // Switch based on the sensor attribute
      switch (attribute)
      {
        case hwmon_fan_input:
          // Get the mutex lock
          mutex_lock(&qnap_ec_mutex);

          // Call the helper program
          qnap_ec_call_helper();

          // Release the mutex lock
          mutex_unlock(&qnap_ec_mutex);

          return 0;
        default:
          return -EOPNOTSUPP;
      }
    case hwmon_pwm:
      // Switch based on the sensor attribute
      switch (attribute)
      {
        case hwmon_pwm_input:
          return 0;
        default:
          return -EOPNOTSUPP;
      }
    case hwmon_temp:
      // Switch based on the sensor attribute
      switch (attribute)
      {
        case hwmon_temp_input:
          return 0;
        default:
          return -EOPNOTSUPP;
      }
    default:
      return -EOPNOTSUPP;
  }
}

static int qnap_ec_write(struct device* dev, enum hwmon_sensor_types type, u32 attribute,
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

// Function called to call the user space helper program
// Note: the return value contains the call_usermodehelper error code (if any) in the first
//       8 bytes of the return value and the user space helper program error code (if any)
//       in the next 8 bytes
static uint8_t qnap_ec_call_helper()
{
  // Declare and/or define needed variables
  int return_value;
  char* arguments[] = {
    "/usr/local/bin/qnap-ec-helper",
    NULL };
  char* environment[] = {
    "PATH=/usr/local/sbin;/usr/local/bin;/usr/sbin;/usr/bin;/sbin;/bin",
    NULL
  };

  // Call the user space helper program and check if the first 8 bytes of the return value
  //   container any error codes
  return_value = call_usermodehelper(arguments[0], arguments, environment, UMH_WAIT_PROC);
  if ((return_value & 0xFF) != 0)
  {
    // Try calling the user space helper program by name only and rely on the path to resolve
    //   its location and check if the first 8 bytes of the return value contain any error codes
    arguments[0] = "qnap-ec-helper";
    return_value = call_usermodehelper(arguments[0], arguments, environment, UMH_WAIT_PROC);
    if ((return_value & 0xFF) != 0)
    {
      return return_value;
    }    
  }

  // Check if the helper program return value stored in the next 8 bytes of the return value
  //   contain any error codes
  if (((return_value >> 8) & 0xFF) != 0)
  {
    return return_value;
  }
  
  return 0;
}

// Function called ...
static long qnap_ec_ioctl(struct file* file, unsigned int command, unsigned long argument)
{
  return 0;
}

// Function called to exit the driver
static void __exit qnap_ec_exit(void)
{
  // Unregister the platform device
  platform_device_unregister(qnap_ec_platform_dev);

  // Destory the character device
  device_destroy(qnap_ec_char_dev_class, qnap_ec_char_dev_major_minor);

  // Destroy the character device class
  class_destroy(qnap_ec_char_dev_class);

  // Delete the character device
  cdev_del(qnap_ec_char_dev);

  // Unregister the character device major and minor numbers
  unregister_chrdev_region(qnap_ec_char_dev_major_minor, 1);

  // Unregister the platform driver
  platform_driver_unregister(&qnap_ec_platform_drv);
}