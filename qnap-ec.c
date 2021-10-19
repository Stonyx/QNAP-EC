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
#include <linux/io.h>
#include <linux/ioctl.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/types.h>
#include "qnap-ec-ioctl.h"

// Define the printk prefix
#undef pr_fmt
#define pr_fmt(fmt) "%s @ %s: " fmt, "qnap-ec", __FUNCTION__

// Specify module details
MODULE_DESCRIPTION("QNAP EC Driver");
MODULE_VERSION("1.0.0");
MODULE_AUTHOR("Stonyx - https://www.stonyx.com/");
MODULE_LICENSE("GPL");

// Declare functions
static int __init qnap_ec_init(void);
static int __init qnap_ec_find(void);
static int qnap_ec_probe(struct platform_device* platform_dev);
static umode_t qnap_ec_hwmon_is_visible(const void* data, enum hwmon_sensor_types type,
                                        u32 attribute, int channel);
static int qnap_ec_hwmon_read(struct device* dev, enum hwmon_sensor_types type, u32 attribute,
                              int channel, long* value);
static int qnap_ec_hwmon_write(struct device* dev, enum hwmon_sensor_types type, u32 attribute,
                               int channel, long value);
static int qnap_ec_call_helper(void);
static int qnap_ec_misc_device_open(struct inode* inode, struct file* file);
static long int qnap_ec_misc_device_ioctl(struct file* file, unsigned int command,
                                          unsigned long argument);
static int qnap_ec_misc_device_release(struct inode* inode, struct file* file);
static void __exit qnap_ec_exit(void);

// Declare and/or define needed variables for overriding the detected device ID
static unsigned short qnap_ec_force_id;
module_param(qnap_ec_force_id, ushort, 0);
MODULE_PARM_DESC(qnap_ec_force_id, "Override the detected device ID");

// Define I/O control data structure
struct qnap_ec_data {
  uint8_t open_device;
  struct qnap_ec_ioctl_command ioctl_command;
  const uint16_t* fan_ids;
  const uint16_t* pwm_ids;
  const uint16_t* temp_ids;
};

// Define the devices structure
// Note: in order to use the platform_device_alloc function (see note in the qnap_ec_init 
//       function) we need to make the plat_device member a pointer and in order to use the
//       container_of macro in the qnap_ec_misc_dev_open and qnap_ec_misc_dev_ioctl functions
//       we need to make the misc_device member not a pointer
struct qnap_ec_devices {
  struct platform_device* plat_device;
  struct miscdevice misc_device;
};

// Define the mutexes
DEFINE_MUTEX(qnap_ec_helper_mutex);
DEFINE_MUTEX(qnap_ec_misc_device_mutex);

// Specifiy the initialization and exit functions
module_init(qnap_ec_init);
module_exit(qnap_ec_exit);

// Declare the platform driver structure pointer
static struct platform_driver* qnap_ec_plat_driver;

// Declare the devices structure pointer
static struct qnap_ec_devices* qnap_ec_devices;

// Function called to initialize the driver
static int __init qnap_ec_init(void)
{
  // Declare needed variables
  int error;

  // Define static constant data consisting of the miscellaneous device file operations structure
  static const struct file_operations misc_device_file_ops = {
    .owner = THIS_MODULE,
    .open = &qnap_ec_misc_device_open,
    .unlocked_ioctl = &qnap_ec_misc_device_ioctl,
    .release = &qnap_ec_misc_device_release
  };

  // Check if we can't find the chip
  error = qnap_ec_find();
  if (error)
  {
    return error;
  }

  // Allocate memory for the platform driver structure and populate various fields
  qnap_ec_plat_driver = kzalloc(sizeof(struct platform_driver), GFP_KERNEL);
  if (qnap_ec_plat_driver == NULL)
  {
    return -ENOMEM;
  }
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

  // Allocate memory for the platform device structure and populate various fields
  // Note: we are using the platform_device_alloc function in combination with the
  //       platform_device_add function instead of the platform_device_register function because
  //       this approach is recommended for legacy type drivers that use hardware probing, all
  //       other hwmon drivers use this approach, and it provides a device release function for us
  qnap_ec_devices->plat_device = platform_device_alloc("qnap-ec", 0);
  if (qnap_ec_devices->plat_device == NULL)
  {
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

    // Free the devices structure memory
    kfree(qnap_ec_devices);

    // Unregister the platform driver
    platform_driver_unregister(qnap_ec_plat_driver);

    // Free the platform driver structure memory
    kfree(qnap_ec_plat_driver);

    return error;
  }

  // Populate various miscellaneous device structure fields
  qnap_ec_devices->misc_device.name = "qnap-ec";
  qnap_ec_devices->misc_device.minor = MISC_DYNAMIC_MINOR;
  qnap_ec_devices->misc_device.fops = &misc_device_file_ops;

  // Register the miscellaneous device
  error = misc_register(&qnap_ec_devices->misc_device);
  if (error)
  {
    // "Unregister" the platform device and free the platform device structure memory
    platform_device_put(qnap_ec_devices->plat_device);

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

// Function called to find the QNAP embedded controller
static int __init qnap_ec_find(void)
{
  // Declare needed variables
  uint8_t byte1;
  uint8_t byte2;

  // Request access to the input (0x2E) and output (0x2F) ports
  if (request_muxed_region(0x2E, 2, "qnap-ec") == NULL)
  {
    return -EBUSY;
  }

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
  // Declare needed variables
  struct qnap_ec_data* data;
  struct device* device;

  // Define static constant data consisiting of fanIDs, PWM Ids, temperature sensor IDs, mulitple
  //   configuration arrays, multiple hwmon channel info structures, the hwmon channel info
  //   structures array, and the hwmon chip information structure
  // Note: IDs are based on the switch statements in the ec_sys_get_fan_speed, ec_sys_get_fan_pwm,
  //       and ec_sys_get_temperature functions in the libuLinux_hal.so library as decompiled by
  //       IDA
  // Note: the entries in the configuration arrays need to match the corresponding ID arrays
  static const uint16_t fan_ids[] = { 5, 7, 10, 11, 25, 35 };
  static const uint16_t pwm_ids[] = { 5, 7, 25, 35 };
  static const uint16_t temp_ids[] = { 1, 7, 10, 11, 38 };
  static const u32 fan_config[] = { HWMON_F_INPUT, HWMON_F_INPUT, HWMON_F_INPUT, HWMON_F_INPUT,
    HWMON_F_INPUT, HWMON_F_INPUT, 0 };
  static const u32 pwm_config[] = { HWMON_PWM_INPUT, HWMON_PWM_INPUT, HWMON_PWM_INPUT,
    HWMON_PWM_INPUT, 0 };
  static const u32 temp_config[] = { HWMON_T_INPUT, HWMON_T_INPUT, HWMON_T_INPUT, HWMON_T_INPUT,
    HWMON_T_INPUT, 0 };
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

  // Allocate device managed memory for the data structure and set various fileds
  data = devm_kzalloc(&platform_dev->dev, sizeof(struct qnap_ec_data), GFP_KERNEL);
  if (data == NULL)
  {
    return -ENOMEM;
  }
  data->fan_ids = fan_ids;
  data->pwm_ids = pwm_ids;
  data->temp_ids = temp_ids;

  // Register the hwmon device and include the data structure
  // Note: hwmon device name cannot contain dashes
  device = devm_hwmon_device_register_with_info(&platform_dev->dev, "qnap_ec", data,
    &hwmon_chip_info, NULL);
  if (device == NULL)
  {
    return -ENOMEM;
  }

  // Set the custom device data to the data structure
  dev_set_drvdata(&platform_dev->dev, data);

  return 0;
}

static umode_t qnap_ec_hwmon_is_visible(const void* data, enum hwmon_sensor_types type,
                                        u32 attribute, int channel)
{
  // Switch based on the sensor type
  // Note: we are using a switch statement to simplify possible future expansion
  switch (type)
  {
    case hwmon_fan:
      // Switch based on the sensor attribute
      switch (attribute)
      {
        case hwmon_fan_input:
          // Check if the fan ID for this channel is not valid
          // Note: fan IDs 10 and 11 are considered not valid because they cause the libuLinux_hal
          //       library functions to fail to execute when retrieving data for these IDs and
          //       based on decompiled code these IDs seem to be only valid in systems with
          //       redundant power supplies
          if (((struct qnap_ec_data*)data)->fan_ids[channel] == 10 ||
            ((struct qnap_ec_data*)data)->fan_ids[channel] == 11)
          {
            return 0;
          }

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
          // Check if the temperature sensors ID for this channel is not valid
          // Note: temperature sensors IDs 10 and 11 are considered not valid because they cause
          //       the libuLinux_hal library functions to fail to execute when retrieving data for
          //       these IDs
          if (((struct qnap_ec_data*)data)->temp_ids[channel] == 10 ||
            ((struct qnap_ec_data*)data)->temp_ids[channel] == 11)
          {
            return 0;
          }

          // Make the temperature input read only
          return S_IRUGO;
        default:
          return 0;
      }
    default:
      return 0;
  }
}

static int qnap_ec_hwmon_read(struct device* device, enum hwmon_sensor_types type, u32 attribute,
                              int channel, long* value)
{
  // Declare and/or define needed variables
  struct qnap_ec_data* data = dev_get_drvdata(device);

  // Get the helper mutex lock
  mutex_lock(&qnap_ec_helper_mutex);

  // Switch based on the sensor type
  // Note: we are using a switch statement to simplify possible future expansion
  switch (type)
  {
    case hwmon_fan:
      // Switch based on the sensor attribute
      switch (attribute)
      {
        case hwmon_fan_input:
          // Set the I/O control command structure fields
          // Note: "sizeof(((struct qnap_ec_ioctl_command*)0)->function_name)" statement is based
          //       on the FIELD_SIZEOF macro which was removed from the kernel
          data->ioctl_command.function_type = int16_func_uint16_uint16pointer;
          strncpy(data->ioctl_command.function_name, "ec_sys_get_fan_speed",
            sizeof(((struct qnap_ec_ioctl_command*)0)->function_name) - 1);
          data->ioctl_command.argument1_uint16 = data->fan_ids[channel];
          data->ioctl_command.argument2_uint16 = 0;

          break;
        default:
          return -EOPNOTSUPP;
      }
      break;
    case hwmon_pwm:
      // Switch based on the sensor attribute
      switch (attribute)
      {
        case hwmon_pwm_input:
          // Set the I/O control command structure fields
          // Note: "sizeof(((struct qnap_ec_ioctl_command*)0)->function_name)" statement is based
          //       on the FIELD_SIZEOF macro which was removed from the kernel
          data->ioctl_command.function_type = int16_func_uint16_uint16pointer;
          strncpy(data->ioctl_command.function_name, "ec_sys_get_fan_pwm",
            sizeof(((struct qnap_ec_ioctl_command*)0)->function_name) - 1);
          data->ioctl_command.argument1_uint16 = data->pwm_ids[channel];
          data->ioctl_command.argument2_uint16 = 0;

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
          // Set the I/O control command structure fields
          // Note: "sizeof(((struct qnap_ec_ioctl_command*)0)->function_name)" statement is based
          //       on the FIELD_SIZEOF macro which was removed from the kernel
          // Note: we are using an int64 field in place of a double field since floating point
          //       math is not possible in kernel space
          data->ioctl_command.function_type = int16_func_uint16_doublepointer;
          strncpy(data->ioctl_command.function_name, "ec_sys_get_temperature",
            sizeof(((struct qnap_ec_ioctl_command*)0)->function_name) - 1);
          data->ioctl_command.argument1_uint16 = data->temp_ids[channel];
          data->ioctl_command.argument2_int64 = 0;

          break;
        default:
          return -EOPNOTSUPP;
      }
      break;
    default:
      return -EOPNOTSUPP;
  }

  // Set the open device flag to allow communication
  data->open_device = 1;

  // Call the helper program
  if (qnap_ec_call_helper() != 0)
  {
    // Release the helper mutex lock
    mutex_unlock(&qnap_ec_helper_mutex);

    return -ENODATA;
  }

  // Clear the open device flag
  data->open_device = 0;

  // Check if the called function returned any errors
  if (data->ioctl_command.return_value_int16 != 0)
  {
    // Log the error
    printk(KERN_ERR "libuLinux_hal library %s function called by qnap-ec helper program returned "
      "a non zero value of %i", data->ioctl_command.function_name,
      data->ioctl_command.return_value_int16);

    // Release the helper mutex lock
    mutex_unlock(&qnap_ec_helper_mutex);

    return -ENODATA;
  }

  // Switch based on the sensor type
  // Note: we are using a switch statement to match the switch statement above with the exception
  //       of error cases
  switch (type)
  {
    case hwmon_fan:
      // Switch based on the sensor attribute
      switch (attribute)
      {
        case hwmon_fan_input:
          // Set the value to the correct returned value
          *value = data->ioctl_command.argument2_uint16;

          break;
      }
      break;
    case hwmon_pwm:
      // Switch based on the sensor attribute
      switch (attribute)
      {
        case hwmon_pwm_input:
          // Set the value to the correct returned value
          *value = data->ioctl_command.argument2_uint16;

          break;
      }
      break;
    case hwmon_temp:
      // Switch based on the sensor attribute
      switch (attribute)
      {
        case hwmon_temp_input:
          // Set the value to the correct returned value
          // Note: we are using an int64 field instead of a double field because floating point
          //       math is not possible in kernel space and because an int64 value can hold a 19
          //       digit integer while a double value can hold a 16 digit integer without loosing
          //       precision we can multiple the double value by 1000 to move three digits after
          //       the decimal point to before the decimal point and still fit the value in an
          //       int64 value and preserve three digits after the decimal point however because
          //       we need to return a millidegree value there is no need to divide by 1000
          *value = data->ioctl_command.argument2_int64;

          break;
      }
      break;
    // Dummy default cause to silence compiler warnings about not including all enums in switch
    //   statement
    default:
      break;
  }

  // Release the helper mutex lock
  mutex_unlock(&qnap_ec_helper_mutex);

  return 0;
}

static int qnap_ec_hwmon_write(struct device* device, enum hwmon_sensor_types type, u32 attribute,
                               int channel, long value)
{
  // Declare and/or define needed variables
  struct qnap_ec_data* data = dev_get_drvdata(device);

  // Get the helper mutex lock
  mutex_lock(&qnap_ec_helper_mutex);

  // Switch based on the sensor type
  // Note: we are using a switch statement to simplify possible future expansion
  switch (type)
  {
    case hwmon_pwm:
      // Switch based on the sensor attribute
      switch (attribute)
      {
        case hwmon_pwm_input:
          // Set the I/O control command structure fields
          // Note: "sizeof(((struct qnap_ec_ioctl_command*)0)->function_name)" statement is based
          //       on the FIELD_SIZEOF macro which was removed from the kernel
          data->ioctl_command.function_type = int16_func_uint16_uint16;
          strncpy(data->ioctl_command.function_name, "ec_sys_set_fan_speed",
            sizeof(((struct qnap_ec_ioctl_command*)0)->function_name) - 1);
          data->ioctl_command.argument1_uint16 = data->fan_ids[channel];
          data->ioctl_command.argument2_uint16 = value;

          return 0;
        default:
          return -EOPNOTSUPP;
      }
    default:
      return -EOPNOTSUPP;
  }

  // Set the open device flag to allow communication
  data->open_device = 1;

  // Call the helper program
  if (qnap_ec_call_helper() != 0)
  {
    // Release the helper mutex lock
    mutex_unlock(&qnap_ec_helper_mutex);

    return -ENODATA;
  }

  // Clear the open device flag
  data->open_device = 0;

  // Check if the called function returned any errors
  if (data->ioctl_command.return_value_int16 != 0)
  {
    // Log the error
    printk(KERN_ERR "libuLinux_hal library %s function called by qnap-ec helper program returned "
      "a non zero value of %i", data->ioctl_command.function_name,
      data->ioctl_command.return_value_int16);

    // Release the helper mutex lock
    mutex_unlock(&qnap_ec_helper_mutex);

    return -ENODATA;
  }

  // Release the helper mutex lock
  mutex_unlock(&qnap_ec_helper_mutex);
}

// Function called to call the user space helper program
// Note: the return value contains the call_usermodehelper error code (if any) in the first
//       8 bytes of the return value and the user space helper program error code (if any)
//       in the next 8 bytes
static int qnap_ec_call_helper()
{
  // Declare and/or define needed variables
  int return_value;
  char* arguments[] = {
    "/usr/local/sbin/qnap-ec",
    NULL 
  };
  char* environment[] = {
    "PATH=/usr/local/bin;/usr/sbin;/usr/bin;/sbin;/bin",
    NULL
  };

  // Call the user space helper program and check if the first 8 bytes of the return value
  //   container any error codes
  return_value = call_usermodehelper(arguments[0], arguments, environment, UMH_WAIT_PROC);
  if ((return_value & 0xFF) != 0)
  {
    // Try calling the user space helper program by name only and rely on the path to resolve
    //   its path and check if the first 8 bytes of the return value contain any error codes
    arguments[0] = "qnap-ec";
    return_value = call_usermodehelper(arguments[0], arguments, environment, UMH_WAIT_PROC);
    if ((return_value & 0xFF) != 0)
    {
      printk(KERN_ERR "qnap-ec helper program not found in the expected path (/usr/local/sbin) "
        "or any of the fall back paths (/usr/local/bin;/usr/sbin;/usr/bin;/sbin;/bin)");

      return return_value & 0xFF;
    }    
  }

  // Check if the helper program return value stored in the next 8 bytes of the return value
  //   contain any error codes
  if (((return_value >> 8) & 0xFF) != 0)
  {
    printk(KERN_ERR "qnap-ec helper program exited with a non zero exit code (%i)",
      ((return_value >> 8) & 0xFF));

    return (return_value >> 8) & 0xFF;
  }
  
  return 0;
}

// Function called when the miscellaneous device is openeded
static int qnap_ec_misc_device_open(struct inode* inode, struct file* file)
{
  // Declare and/or define needed variables
  // Note: the following statement is a combination of the following two statements
  //       struct qnap_ec_devices* devices = container_of(file->private_data,
  //         struct qnap_ec_devices, misc_device);
  //       struct qnap_ec_data* data = dev_get_drvdata(&devices->plat_device->dev);
  struct qnap_ec_data* data = dev_get_drvdata(&container_of(file->private_data,
    struct qnap_ec_devices, misc_device)->plat_device->dev);

  // Check if the open device flag is not set which means we are not expecting any communications
  if (data->open_device == 0)
  {
    return -EBUSY;
  }

  // Try to lock the character device mutex if it's currently unlocked
  // Note: if the mutex is currently locked it means we are already communicating and this is an
  //       unexpected communication
  if (mutex_trylock(&qnap_ec_misc_device_mutex) == 0)
  {
    return -EBUSY;
  }

  return 0;
}

// Function called when the miscellaneous device receives a I/O control command
static long int qnap_ec_misc_device_ioctl(struct file* file, unsigned int command,
                                          unsigned long argument)
{
  // Declare and/or define needed variables
  // Note: the following statement is a combination of the following two statements
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
      {
        return -EFAULT;
      }

      // Copy the I/O control command data from the data structure to the user space
      if (copy_to_user((void*)argument, &data->ioctl_command,
        sizeof(struct qnap_ec_ioctl_command)) != 0)
      {
        return -EFAULT;
      }

      return 0;
    case QNAP_EC_IOCTL_RETURN:
      // Make sure we can read the data from user space
      if (access_ok(argument, sizeof(struct qnap_ec_ioctl_command)) == 0)
      {
        return -EFAULT;
      }

      // Copy the I/O control command data from the user space to the data structure
      if (copy_from_user(&data->ioctl_command, (void*)argument,
        sizeof(struct qnap_ec_ioctl_command)) != 0)
      {
        return -EFAULT;
      }

      return 0;
    default:
      return -EINVAL;
  }

  return 0;
}

// Function called when the miscellaneous device is released
static int qnap_ec_misc_device_release(struct inode* inode, struct file* file)
{
  // Release the character device mutex lock
  mutex_unlock(&qnap_ec_misc_device_mutex);

  return 0;
}

// Function called to exit the driver
static void __exit qnap_ec_exit(void)
{
  // Unregister the miscellaneous device
  misc_deregister(&qnap_ec_devices->misc_device);

  // Unregister the platform device and free the platform device structure memory
  // Note: we are using the platform_device_unregister function instead of the platform_device_put
  //       function used in the qnap_ec_init function because all other hwmon drivers take this
  //       approach
  platform_device_unregister(qnap_ec_devices->plat_device);

  // Free the devices structure memory
  kfree(qnap_ec_devices);

  // Unregister the platform driver
  platform_driver_unregister(qnap_ec_plat_driver);

  // Free the platform driver structure memory
  kfree(qnap_ec_plat_driver);
}