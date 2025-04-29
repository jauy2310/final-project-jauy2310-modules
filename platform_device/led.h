#ifndef _PLATFORM_DEV_H_
#define _PLATFORM_DEV_H_

#include <linux/fs.h>                   // linux file system
#include <linux/kernel.h>               // kernel
#include <linux/module.h>               // generic kernel module
#include <linux/platform_device.h>      // platform devices
#include <linux/init.h>                 // init/exit
#include <linux/cdev.h>                 // cdev
#include <linux/mod_devicetable.h>      // device tables (associate module to device)
#include <linux/device.h>               // device definitions

#include <linux/miscdevice.h>
#include <linux/uaccess.h>

// local includes
#include "log.h"
#include "registers.h"

// module definitions
#define DEVICE_NAME "led"

// device info
static char led_state = 0; // 0 = OFF, 1 = ON

#endif /* _PLATFORM_DEV_H_ */