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

/**************************************************************************************
 * MACROS/DEFINES
 **************************************************************************************/
// define a debug flag
#define DEBUG 1

// logging/debugging prints
#undef LOG
#undef LOGW
#ifdef DEBUG
    #ifdef __KERNEL__
        #define LOG(fmt, args...) \
            printk(KERN_DEBUG "ws2812 [D]: [%s():%d] " fmt "\n", __func__, __LINE__, ## args)
        #define LOGW(fmt, args...) \
            printk(KERN_WARNING "ws2812 [W]: [%s():%d] " fmt "\n", __func__, __LINE__, ## args)
        #define LOGE(fmt, args...) \
            printk(KERN_ERR "ws2812 [E]: [%s():%d] " fmt "\n", __func__, __LINE__, ## args)
    #else
        #define LOG(fmt, args...) \
            printf("ws2812 [D]: [%s():%d] " fmt "\n", __func__, __LINE__, ## args)
        #define LOGW(fmt, args...) \
            printf("ws2812 [W]: [%s():%d] " fmt "\n", __func__, __LINE__, ## args)
        #define LOGE(fmt, args...) \
            printf("ws2812 [E]: [%s():%d] " fmt "\n", __func__, __LINE__, ## args)
    #endif
#else
    #define LOG(fmt, args...)
    #define LOGW(fmt, args...)
    #define LOGE(fmt, args...)
#endif

// define module name
#define WS2812_MODULE_NAME "ws2812"

/**************************************************************************************
 * STRUCTS
 **************************************************************************************/

// device struct
struct ws2812_dev {
    struct cdev cdev;
};

/**************************************************************************************
 * MODULE LOAD/UNLOAD FUNCTION PROTOTYPES
 **************************************************************************************/
static int ws2812_setup_cdev(struct ws2812_dev *dev);

# endif /* _WS2812_H_ */