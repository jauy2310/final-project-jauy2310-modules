#include "ws2812.h"

// kernel module definitions/macros
#include <linux/init.h>
#include <linux/module.h>
MODULE_LICENSE("Dual BSD/GPL");

/**
 * hello_init()
 * 
 * Testing the load/unload of a module
 */
static int hello_init(void) {
    printk(KERN_ALERT "Hello, world (jauy2310 - final project)\n");
    return 0;
}

/**
 * hello_exit()
 * 
 * Testing the unload of a module
 */
static void hello_exit(void) {
    printk(KERN_ALERT "Goodbye, cruel world (jauy2310 - final project)\n");
}

/**
 * module init/exit functions
 */
module_init(hello_init);
module_exit(hello_exit);
