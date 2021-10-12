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

// Define the mutexes
DEFINE_MUTEX(qnap_ec_helper_mutex);
DEFINE_MUTEX(qnap_ec_char_dev_mutex);

// Define I/O control data structure
struct qnap_ec_ioctl_data {
  struct qnap_ec_ioctl_call_func_data call_func_data;
  struct qnap_ec_ioctl_return_data return_data;
  atomic_t open_device;
};

// Declare functions
static int __init qnap_ec_init(void);
static int __init qnap_ec_find(void);
static int qnap_ec_probe(struct platform_device* platform_dev);
static int qnap_ec_add_char_dev(struct qnap_ec_ioctl_data* ioctl_data);
static umode_t qnap_ec_hwmon_is_visible(const void* data, enum hwmon_sensor_types type,
                                        u32 attribute, int channel);
static int qnap_ec_hwmon_read(struct device* dev, enum hwmon_sensor_types type, u32 attribute,
                              int channel, long* value);
static int qnap_ec_hwmon_write(struct device* dev, enum hwmon_sensor_types type, u32 attribute,
                               int channel, long value);
static int qnap_ec_call_helper(void);
static int qnap_ec_char_dev_open(struct inode* inode, struct file* file);
static long int qnap_ec_char_dev_ioctl(struct file* file, unsigned int command,
                                       unsigned long argument);
static int qnap_ec_char_dev_release(struct inode* inode, struct file* file);
static int qnap_ec_remove(struct platform_device* platform_dev);
static void __exit qnap_ec_remove_char_dev(void);
static void __exit qnap_ec_exit(void);

// Declare and/or define needed variables for overriding the detected device ID
static unsigned short qnap_ec_force_id;
module_param(qnap_ec_force_id, ushort, 0);
MODULE_PARM_DESC(qnap_ec_force_id, "Override the detected device ID");

// Specifiy the init function
module_init(qnap_ec_init);

// Define the platform driver structure
static struct platform_driver qnap_ec_platform_drv = {
  .driver = {
    .name = "qnap_ec" // Dashes are not allowed in driver names
  },
  .probe = qnap_ec_probe,
  .remove = qnap_ec_remove
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
  .is_visible = qnap_ec_hwmon_is_visible,
  .read = qnap_ec_hwmon_read,
  .write = qnap_ec_hwmon_write
};

// Define the hwmon chip information structure
static const struct hwmon_chip_info qnap_ec_hwmon_chip_info = {
  .info = qnap_ec_hwmon_channel_info,
  .ops = &qnap_ec_hwmon_ops
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
  .open = qnap_ec_char_dev_open,
  .unlocked_ioctl = qnap_ec_char_dev_ioctl,
  .release = qnap_ec_char_dev_release
};

// Specify the module exit function
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

  // Allocate the platform device
  // Note: name matches the platform driver name which doesn't allow dashes
  qnap_ec_platform_dev = platform_device_alloc("qnap_ec", 0);
  if (qnap_ec_platform_dev == NULL)
  {
    // Unregister the platform driver
    platform_driver_unregister(&qnap_ec_platform_drv);

    return -ENOMEM;
  }

  // Add the platform device
  error = platform_device_add(qnap_ec_platform_dev);
  if (error)
  {
    // Unallocate the platform device
    platform_device_put(qnap_ec_platform_dev);

    // Unregister the platform driver
    platform_driver_unregister(&qnap_ec_platform_drv);

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
  int error;

  // Allocate memory for the ioctl data and associate with the platform device
  ioctl_data = devm_kzalloc(&platform_dev->dev, sizeof(*ioctl_data), GFP_KERNEL);
  if (ioctl_data == NULL)
  {
    return -ENOMEM;
  }

  // Register the platform device
  // Note: name matches the platform driver name which doesn't allow dashes
  device = devm_hwmon_device_register_with_info(&platform_dev->dev, "qnap_ec", ioctl_data,
    &qnap_ec_hwmon_chip_info, NULL);
  if (device == NULL)
  {
    return -ENOMEM;
  }

  // Add the character device
  error = qnap_ec_add_char_dev(ioctl_data);
  if (error)
  {
    // Unregister the platform device
    platform_device_unregister(platform_dev);

    return error;
  }

  return 0;
}

// Function called to add the character device
static int qnap_ec_add_char_dev(struct qnap_ec_ioctl_data* ioctl_data)
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
  device = device_create(qnap_ec_char_dev_class, NULL, qnap_ec_char_dev_major_minor, ioctl_data,
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

static int qnap_ec_hwmon_read(struct device* dev, enum hwmon_sensor_types type, u32 attribute,
                             int channel, long* value)
{
  // Declare and/or define needed variables
  struct qnap_ec_ioctl_data* ioctl_data = dev_get_drvdata(dev);

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
          //mutex_lock(&qnap_ec_helper_mutex);

          // Set the ioctl data fields to call the correct function with the right arguments
          ioctl_data->call_func_data.function = ec_sys_get_fan_speed;
          ioctl_data->call_func_data.argument1 = channel;

          // Set the open device flag to allow communication
          atomic_set(&ioctl_data->open_device, 1);

          // Call the helper program
          if (qnap_ec_call_helper() != 0)
          {
            // Release the helper mutex lock
            mutex_unlock(&qnap_ec_helper_mutex);

            return -1;
          }

          // Clear the open device flag
          atomic_set(&ioctl_data->open_device, 0);

          // Set the value to the return data value
          *value = ioctl_data->return_data.value;

          // Release the helper mutex lock
          //mutex_unlock(&qnap_ec_helper_mutex);

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

static int qnap_ec_hwmon_write(struct device* dev, enum hwmon_sensor_types type, u32 attribute,
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

// Function called when the character device is openeded
static int qnap_ec_char_dev_open(struct inode* inode, struct file* file)
{
  // Declare and/or define needed variables
  struct qnap_ec_ioctl_data* ioctl_data = dev_get_drvdata(file->private_data);

  printk(KERN_INFO "qnap_ec_char_dev_open");

  // Check if the open device flag is not set which means we are not expecting any communications
  if (atomic_read(&ioctl_data->open_device) == 0)
  {
    return -EBUSY;
  }

  printk(KERN_INFO "qnap_ec_char_dev_open - #1");

  // Try to lock the character device mutex if it's currently unlocked
  // Note: if the mutex is currently locked it means we are already communicating and this is an
  //       unexpected communication
  if (mutex_trylock(&qnap_ec_char_dev_mutex) == 0)
  {
    return -EBUSY;
  }

  printk(KERN_INFO "qnap_ec_char_dev_open - #2");

  return 0;
}

// Function called when the character device receives a I/O control command
static long int qnap_ec_char_dev_ioctl(struct file* file, unsigned int command,
                                       unsigned long argument)
{
  // Declare and/or define needed variables
  struct qnap_ec_ioctl_data* ioctl_data = dev_get_drvdata(file->private_data);

  printk(KERN_INFO "qnap_ec_char_dev_ioctl - %i", command);

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
}

// Function called when the character device is released
static int qnap_ec_char_dev_release(struct inode* inode, struct file* file)
{
  printk(KERN_INFO "qnap_ec_char_dev_release");

  // Release the character device mutex lock
  mutex_unlock(&qnap_ec_char_dev_mutex);

  return 0;
}

// Function called to remove this driver
static int qnap_ec_remove(struct platform_device* platform_dev)
{
  // Remove the character device
  qnap_ec_remove_char_dev();

  return 0;
}

// Function called to remove the character device
static void qnap_ec_remove_char_dev(void)
{
  // Destory the character device
  device_destroy(qnap_ec_char_dev_class, qnap_ec_char_dev_major_minor);

  // Destroy the character device class
  class_destroy(qnap_ec_char_dev_class);

  // Delete the character device
  cdev_del(qnap_ec_char_dev);

  // Unregister the character device major and minor numbers
  unregister_chrdev_region(qnap_ec_char_dev_major_minor, 1);
}

// Function called to exit the driver
static void __exit qnap_ec_exit(void)
{
  // Unregister the platform device
  platform_device_unregister(qnap_ec_platform_dev);

  // Unregister the platform driver
  platform_driver_unregister(&qnap_ec_platform_drv);
}