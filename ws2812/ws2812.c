#include "ws2812.h"

/**************************************************************************************
 * KERNEL MODULE DECLARATIONS/DEFINITIONS
 **************************************************************************************/

// kernel module includes
#include <linux/module.h>
#include <linux/init.h>
#include <linux/printk.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/fs.h>

// module info
MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Jake Uyechi");

// module numbers
int ws2812_major = 0; // dynamically allocate
int ws2812_minor = 0;

/**************************************************************************************
 * MODULE IMPLEMENTATION
 **************************************************************************************/
struct file_operations fops = {
    .owner      = THIS_MODULE,
};

/**************************************************************************************
 * MODULE LOAD/UNLOAD FUNCTIONS
 **************************************************************************************/

/**
 * ws2812_init()
 * 
 * Loading the module
 */
static int ws2812_init(void) {
    // start driver load
    LOG("Starting WS2812B LED Kernel Module Load.");

    // register this module as a char device


    // return
    LOG("WS2812B LED Kernel Module Loaded!");
    return 0;
}

/**
 * ws2812_exit()
 * 
 * Unloading the module
 */
static void ws2812_exit(void) {
    // start driver cleanup
    LOG("Cleaning up WS2812B LED Kernel Module.");
    
    // function complete
    LOG("WS2812B LED Kernel Module Cleaned Up!");
}

/**
 * module init/exit functions
 */
module_init(ws2812_init);
module_exit(ws2812_exit);
