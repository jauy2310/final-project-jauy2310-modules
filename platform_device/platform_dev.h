#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/init.h>

// local includes
#include "log.h"

// define an led node
typedef struct led_node {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} led_node;

#define MODULE_NAME "ws2812"

struct ws2812_dev {
    led_node led_strip[MAX_LEDS];
}