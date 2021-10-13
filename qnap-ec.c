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
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/types.h>
#include "qnap-ec.h"

// Define constants
#define QNAP_EC_ID_PORT_1 0x2E
#define QNAP_EC_ID_PORT_2 0x2F

// Specify module details
MODULE_DESCRIPTION("QNAP EC Driver");
MODULE_AUTHOR("Stonyx - https://www.stonyx.com/");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:qnap-ec");

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
static int qnap_ec_misc_dev_open(struct inode* inode, struct file* file);
static long int qnap_ec_misc_dev_ioctl(struct file* file, unsigned int command,
                                       unsigned long argument);
static int qnap_ec_misc_dev_release(struct inode* inode, struct file* file);
static void qnap_ec_plat_device_release(struct device *device);
static int qnap_ec_remove(struct platform_device* platform_dev);
static void __exit qnap_ec_remove_char_dev(void);
static void __exit qnap_ec_exit(void);

// Declare and/or define needed variables for overriding the detected device ID
static unsigned short qnap_ec_force_id;
module_param(qnap_ec_force_id, ushort, 0);
MODULE_PARM_DESC(qnap_ec_force_id, "Override the detected device ID");

// Define I/O control data structure
struct qnap_ec_ioctl_data {
  struct qnap_ec_ioctl_call_func_data call_func_data;
  struct qnap_ec_ioctl_return_data return_data;
  atomic_t open_device;
};

// Define the devices structure
struct qnap_ec_devices {
  struct platform_device* plat_device;
  struct miscdevice* misc_device;
};

// Define the mutexes
DEFINE_MUTEX(qnap_ec_helper_mutex);
DEFINE_MUTEX(qnap_ec_char_dev_mutex);

// Specifiy the init and exit functions
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

  // Define static constant data consisting of the miscellaneous driver file operations structure
  static const struct file_operations misc_device_file_ops = {
    .owner = THIS_MODULE,
    .open = &qnap_ec_misc_dev_open,
    .unlocked_ioctl = &qnap_ec_misc_dev_ioctl,
    .release = &qnap_ec_misc_dev_release
  };

  // Check if we can't find the chip
  if (qnap_ec_find() != 0)
  {
    return -ENODEV;
  }

  // Allocate memory for the platform driver structure and populate various fields
  qnap_ec_plat_driver = kzalloc(sizeof(struct platform_driver), GFP_KERNEL);
  if (qnap_ec_plat_driver == NULL)
  {
    return -ENOMEM;
  }
  qnap_ec_plat_driver->driver.owner = THIS_MODULE;
  qnap_ec_plat_driver->driver.name = "qnap_ec"; // Dashes are not allowed in driver names
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
  qnap_ec_devices->plat_device = kzalloc(sizeof(struct platform_device), GFP_KERNEL);
  if (qnap_ec_devices->plat_device == NULL)
  {
    // Free the devices structure memory
    kfree(qnap_ec_devices);

    // Unregister the platform driver
    platform_driver_unregister(qnap_ec_plat_driver);

    // Free the platform driver structure memory
    kfree(qnap_ec_plat_driver);

    return -ENOMEM;
  }
  qnap_ec_devices->plat_device->name = "qnap_ec"; // Name matches the platform driver name
  qnap_ec_devices->plat_device->id = PLATFORM_DEVID_NONE;
  qnap_ec_devices->plat_device->dev.release = &qnap_ec_plat_device_release;

  // Register the platform device
  error = platform_device_register(qnap_ec_devices->plat_device);
  if (error)
  {
    // Free the platform device structure memory
    kfree(qnap_ec_devices->plat_device);

    // Free the devices structure memory
    kfree(qnap_ec_devices);

    // Unregister the platform driver
    platform_driver_unregister(qnap_ec_plat_driver);

    // Free the platform driver structure memory
    kfree(qnap_ec_plat_driver);

    return error;
  }

  // Allocate memory for the miscellaneous device structure and populate various fields
  qnap_ec_devices->misc_device = kzalloc(sizeof(struct miscdevice), GFP_KERNEL);
  if (qnap_ec_devices->misc_device == NULL)
  {
    // Unregister the platform device
    platform_device_unregister(qnap_ec_devices->plat_device);

    // Free the platform device structure memory
    kfree(qnap_ec_devices->plat_device);

    // Free the devices structure memory
    kfree(qnap_ec_devices);

    // Unregister the platform driver
    platform_driver_unregister(qnap_ec_plat_driver);

    // Free the platform driver structure memory
    kfree(qnap_ec_plat_driver);

    return -ENOMEM;
  }
  qnap_ec_devices->misc_device->name = "qnap-ec";
  qnap_ec_devices->misc_device->minor = MISC_DYNAMIC_MINOR;
  qnap_ec_devices->misc_device->fops = &misc_device_file_ops;

  // Register the miscellaneous device
  error = misc_register(qnap_ec_devices->misc_device);
  if (error)
  {
    // Free the miscellaneous device structure memory
    kfree(qnap_ec_devices->misc_device); 

    // Unregister the platform device
    platform_device_unregister(qnap_ec_devices->plat_device);

    // Free the platform device structure memory
    kfree(qnap_ec_devices->plat_device);

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
  return 0;
}

// Function called to probe this driver
static int qnap_ec_probe(struct platform_device* platform_dev)
{
  // Declare needed variables
  struct qnap_ec_ioctl_data* ioctl_data;
  struct device* device;

  // Define static constant data consisiting of mulitple configuration arrays, multiple hwmon
  //   channel info structures, the hwmon channel info structures array, and the hwmon chip
  //   information structure
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
    .type = hwmon_pwm,
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

  // Allocate device managed memory for the ioctl data structure
  ioctl_data = devm_kzalloc(&platform_dev->dev, sizeof(struct qnap_ec_ioctl_data), GFP_KERNEL);
  if (ioctl_data == NULL)
  {
    return -ENOMEM;
  }

  // Register the hwmon device and include the ioctl data structure
  // Note: name matches the platform driver name which doesn't allow dashes
  device = devm_hwmon_device_register_with_info(&platform_dev->dev, "qnap_ec", ioctl_data,
    &hwmon_chip_info, NULL);
  if (device == NULL)
  {
    return -ENOMEM;
  }

  return 0;
}

static umode_t qnap_ec_hwmon_is_visible(const void* data, enum hwmon_sensor_types type,
                                        u32 attribute, int channel)
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

static int qnap_ec_hwmon_read(struct device* device, enum hwmon_sensor_types type, u32 attribute,
                              int channel, long* value)
{
  // Declare and/or define needed variables
  struct qnap_ec_ioctl_data* ioctl_data = dev_get_drvdata(device);

  printk(KERN_INFO "qnap_ec_hwmon_read - %i - %i - %i", type, attribute, channel);

  // Switch based on the sensor type
  switch (type)
  {
    case hwmon_fan:
      // Switch based on the sensor attribute
      switch (attribute)
      {
        case hwmon_fan_input:
          // Get the helper mutex lock
          // mutex_lock(&qnap_ec_helper_mutex);

          // Set the ioctl data fields to call the correct function with the right arguments
          ioctl_data->call_func_data.function = ec_sys_get_fan_speed;
          ioctl_data->call_func_data.argument1 = channel;

          // Set the open device flag to allow communication
          atomic_set(&ioctl_data->open_device, 1);

          // Call the helper program
          if (qnap_ec_call_helper() != 0)
          {
            // Release the helper mutex lock
            // mutex_unlock(&qnap_ec_helper_mutex);

            return -1;
          }

          // Clear the open device flag
          atomic_set(&ioctl_data->open_device, 0);

          // Set the value to the return data value
          *value = ioctl_data->return_data.value;

          // Release the helper mutex lock
          // mutex_unlock(&qnap_ec_helper_mutex);

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

static int qnap_ec_hwmon_write(struct device* device, enum hwmon_sensor_types type, u32 attribute,
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
static int qnap_ec_call_helper()
{
  // Declare and/or define needed variables
  int return_value;
  char* arguments[] = {
    "/usr/local/bin/qnap-ec-helper",
    NULL 
  };
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

// Function called when the miscellaneous device is openeded
static int qnap_ec_misc_dev_open(struct inode* inode, struct file* file)
{
  // Declare and/or define needed variables
  struct qnap_ec_devices* devices = container_of(file->private_data,
    struct qnap_ec_devices, misc_device);
  struct qnap_ec_ioctl_data* ioctl_data = dev_get_drvdata(&devices->plat_device->dev);

  printk(KERN_INFO "qnap_ec_char_dev_open");

  // Check if the open device flag is not set which means we are not expecting any communications
  if (atomic_read(&ioctl_data->open_device) == 0)
  {
    return -EBUSY;
  }

  // Try to lock the character device mutex if it's currently unlocked
  // Note: if the mutex is currently locked it means we are already communicating and this is an
  //       unexpected communication
  if (mutex_trylock(&qnap_ec_char_dev_mutex) == 0)
  {
    return -EBUSY;
  }

  return 0;
}

// Function called when the miscellaneous device receives a I/O control command
static long int qnap_ec_misc_dev_ioctl(struct file* file, unsigned int command,
                                       unsigned long argument)
{
  printk(KERN_INFO "qnap_ec_char_dev_ioctl - %i", command);

  /*
  // Declare and/or define needed variables
  struct qnap_ec_ioctl_data* ioctl_data = dev_get_drvdata(file->private_data);

  // Swtich based on the command
  switch (command)
  {
    case QNAP_EC_IOCTL_CALL_FUNC:
      // Make sure we can write the data to user space
      if (access_ok(argument, sizeof(struct qnap_ec_ioctl_call_func_data)) == 0)
      {
        return -EFAULT;
      }

      // Copy the data from the platform data structure to the user space
      if (copy_to_user((int32_t*)argument, &ioctl_data->call_func_data,
        sizeof(struct qnap_ec_ioctl_call_func_data) != 0))
      {
        return -EFAULT;
      }

      return 0;
    case QNAP_EC_IOCTL_RETURN:
      // Make sure we can read the data from user space
      if (access_ok(argument, sizeof(struct qnap_ec_ioctl_return_data)) == 0)
      {
        return -EFAULT;
      }

      // Copy the data from the user space to the platform data structure
      if (copy_from_user(&ioctl_data->return_data, (int32_t*)argument,
        sizeof(struct qnap_ec_ioctl_return_data) != 0))
      {
        return -EFAULT;
      }

      return 0;
    default:
      return -EINVAL;
  }
  */

  return 0;
}

// Function called when the miscellaneous device is released
static int qnap_ec_misc_dev_release(struct inode* inode, struct file* file)
{
  printk(KERN_INFO "qnap_ec_char_dev_release");

  // Release the character device mutex lock
  mutex_unlock(&qnap_ec_char_dev_mutex);

  return 0;
}

// Function called when the device is release
// Note: since we are using the platform_device_register function instead of a combination of the
//       platform_device_allow and platform_device_add functions in our init function we need to
//       create a device release function of our own and are using an exact duplicate (with the
//       exception of variable names) of the static platform_device_release function as defined in
//       the linux/drivers/base/platform.c file
static void qnap_ec_plat_device_release(struct device *device)
{
  /*
  struct platform_object* plat_object = container_of(device, struct platform_object,
    qnap_ec_devices->plat_device->dev);

  of_node_put(plat_object->pdev.dev.of_node);
  kfree(plat_object->pdev.dev.platform_data);
  kfree(plat_object->pdev.mfd_cell);
  kfree(plat_object->pdev.resource);
  kfree(plat_object->pdev.driver_override);
  kfree(plat_object);
  */
}

// Function called to exit the driver
static void __exit qnap_ec_exit(void)
{
  // Unregister the miscellaneous device
  misc_deregister(qnap_ec_devices->misc_device);

  // Free the miscellaneous device structure memory
  kfree(qnap_ec_devices->misc_device); 

  // Unregister the platform device
  platform_device_unregister(qnap_ec_devices->plat_device);

  // Free the platform device structure memory
  kfree(qnap_ec_devices->plat_device);

  // Free the devices structure memory
  kfree(qnap_ec_devices);

  // Unregister the platform driver
  platform_driver_unregister(qnap_ec_plat_driver);

  // Free the platform driver structure memory
  kfree(qnap_ec_plat_driver);
}