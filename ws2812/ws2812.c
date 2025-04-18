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
    // test - modify this to verify the latest load of the module
    printk(KERN_INFO "Hello, world (jauy2310 - final project)\n");

    // register this module as a char device


    // return
    return 0;
}

/**
 * ws2812_exit()
 * 
 * Unloading the module
 */
static void ws2812_exit(void) {
    printk(KERN_INFO "Goodbye, cruel world (jauy2310 - final project)\n");
}

/**
 * module init/exit functions
 */
module_init(ws2812_init);
module_exit(ws2812_exit);
