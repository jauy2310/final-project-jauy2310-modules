#include "ws2812_driver.h"

/**************************************************************************************
 * KERNEL MODULE DECLARATIONS/DEFINITIONS
 **************************************************************************************/

// module info
MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Jake Uyechi");

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

    // initialization setup
    int result = 0;

    /*****************************
     * INITIALIZE
     *****************************/


    /*****************************
     * REGISTER DEVICE
     *****************************/
    // return
    LOG("WS2812B LED Kernel Module Loaded!");
    return result;
}

/**
 * ws2812_exit()
 * 
 * Unloading the module
 */
static void ws2812_exit(void) {
    // start driver cleanup
    LOG("Cleaning up WS2812B LED Kernel Module.");

    /*****************************
     * DEINITIALIZE
     *****************************/

    /*****************************
     * UNREGISTER DEVICE
     *****************************/

    
    // function complete
    LOG("WS2812B LED Kernel Module Cleaned Up!");
}

/**
 * module init/exit functions
 */
module_init(ws2812_init);
module_exit(ws2812_exit);
