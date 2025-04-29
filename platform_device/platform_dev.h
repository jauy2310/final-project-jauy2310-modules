#ifndef _PLATFORM_DEV_H_
#define _PLATFORM_DEV_H_

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>

// local includes
#include "log.h"

// define an led node
typedef struct led_node {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} led_node;

// module definitions
#define DEVICE_NAME "led"
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

static char led_state = 0; // 0 = OFF, 1 = ON

// function prototypes
static int ws2812_open(struct inode *inode, struct file *file);
static int ws2812_release(struct inode *inode, struct file *file);
static ssize_t ws2812_write(struct file *file, const char __user *buf, size_t len, loff_t *offset);
static int ws2812_probe(struct platform_device *pdev);
static int ws2812_remove(struct platform_device *pdev);

#endif /* _PLATFORM_DEV_H_ */