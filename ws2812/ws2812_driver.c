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
static const struct proc_ops fops = {
    // file operations
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
    /*****************************
     * START MODULE ENTRY
     *****************************/
    // initialization setup
    int result = 0;
    
    // start driver load
    LOG("Starting WS2812B LED Kernel Module Load.");

    /*****************************
     * INITIALIZE
     *****************************/
    // remap the GPIO peripheral's physical address to a driver-usable one
    gpio_registers = (int *)ioremap(BCM_GPIO_BASE_ADDRESS, PAGE_SIZE);
    if (gpio_registers == NULL) {
        LOGE("> GPIO peripheral cannot be remapped.");
        return -ENOMEM;
    } else {
        LOG("> GPIO peripheral mapped in memory at %p.", gpio_registers);
    }

    /*****************************
     * REGISTER MODULE
     *****************************/
    // create an entry in procfs
    ws2812_proc = proc_create(WS2812_MODULE_NAME, 0666, NULL, &fops);
    if (ws2812_proc == NULL) {
        LOGE("> Unable to create entry in procfs.");
        return -ENOMEM;
    } else {
        LOG("> Entry created for module at /proc/%s", WS2812_MODULE_NAME);
    }

    
    /*****************************
     * RETURN
     *****************************/
    LOG("WS2812B LED Kernel Module Loaded!");
    return result;
}

/**
 * ws2812_exit()
 * 
 * Unloading the module
 */
static void ws2812_exit(void) {
    /*****************************
     * START MODULE EXIT
     *****************************/
    // start driver cleanup
    LOG("Cleaning up WS2812B LED Kernel Module.");

    /*****************************
     * UNREGISTER MODULE
     *****************************/
    // remove entry in procfs; proc_remove() checks input param
    if (ws2812_proc != NULL) {
        LOG("> Unregistering module from procfs.");
        proc_remove(ws2812_proc);
    } else {
        LOGW("> Module not registered in procfs; skipping.");
    }

    /*****************************
     * DE-INITIALIZE
     *****************************/
    // unmap the GPIO peripheral from memory
    if (gpio_registers != NULL) {
        LOG("> Unmapping GPIO peripheral.");
        iounmap(gpio_registers);
    }
    
    /*****************************
     * RETURN
     *****************************/
    LOG("WS2812B LED Kernel Module Cleaned Up!");
}

/**
 * module init/exit functions
 */
module_init(ws2812_init);
module_exit(ws2812_exit);
