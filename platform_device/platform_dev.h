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
#include <linux/module.h>

// local includes
#include "log.h"

// define an led node
typedef struct led_node {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} led_node;

// module definitions
#define MODULE_NAME "ws2812"
#define MAX_LEDS 100

// platform driver/char device definition
struct ws2812_dev {
    // generic device/driver
    struct cdev cdev;
    struct class *class;
    struct device *device;

    // driver-specific
    led_node led_strip[MAX_LEDS];
};

// function prototypes
static int ws2812_open(struct inode *inode, struct file *file);
static int ws2812_release(struct inode *inode, struct file *file);
static ssize_t ws2812_write(struct file *file, const char __user *buf, size_t len, loff_t *offset);
static int ws2812_probe(struct platform_device *pdev);
static int ws2812_remove(struct platform_device *pdev);

#endif /* _PLATFORM_DEV_H_ */