#ifndef _WS2812_H_
#define _WS2812_H_

/**************************************************************************************
 * INCLUDES
 **************************************************************************************/
#ifdef __KERNEL__
#include <linux/types.h>
#else
#include <stddef.h> // size_t
#include <stdint.h> // uintx_t
#include <stdbool.h>
#endif

// kernel module includes
#include <linux/module.h>
#include <linux/init.h>
#include <linux/printk.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/gpio/consumer.h>

// local includes
#include "log.h"
#include "led.h"

/**************************************************************************************
 * MACROS/DEFINES
 **************************************************************************************/
// define a debug flag
#define DEBUG 1

// define module name
#define WS2812_MODULE_NAME "ws2812"
#define WS2812_GPIO_PIN 12

/**************************************************************************************
 * STRUCTS
 **************************************************************************************/

// device struct
struct ws2812_dev {
    struct led_node *strip;
    struct cdev cdev;
};

/**************************************************************************************
 * MODULE LOAD/UNLOAD FUNCTION PROTOTYPES
 **************************************************************************************/
static int ws2812_setup_cdev(struct ws2812_dev *dev);

# endif /* _WS2812_H_ */