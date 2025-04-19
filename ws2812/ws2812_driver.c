#include "ws2812_driver.h"

/**************************************************************************************
 * KERNEL MODULE DECLARATIONS/DEFINITIONS
 **************************************************************************************/

// module info
MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Jake Uyechi");

// module numbers
int ws2812_major = 0; // dynamically allocate
int ws2812_minor = 0;

// global device definition
struct ws2812_dev ws2812_dev;

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
 * ws2812_setup_cdev()
 * 
 * Sets up the character device
 */
static int ws2812_setup_cdev(struct ws2812_dev *dev) {
    // setup
    int err = 0;

    // create the device 
    int devno = MKDEV(ws2812_major, ws2812_minor);
    cdev_init(&dev->cdev, &fops);

    // set device fields
    dev->cdev.owner = THIS_MODULE;
    dev->cdev.ops = &fops;
    
    // add the device to the kernel
    cdev_add(&dev->cdev, devno, 1);

    // return
    if (err) {
        LOGE("Error adding ws2812 cdev. (%d)", err);
    }
    return err;
}

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
    dev_t dev = 0;

    /*****************************
     * INITIALIZE STRUCTS
     *****************************/

    /*****************************
     * REGISTER DEVICE
     *****************************/

    // register this module as a char device and clear device memory region
    result = alloc_chrdev_region(&dev, ws2812_minor, 1, WS2812_MODULE_NAME);
    ws2812_major = MAJOR(dev);
    if (result < 0) {
        LOGW("Can't get major %d\n", ws2812_major);
        return result;
    }
    memset(&ws2812_dev, 0, sizeof(struct ws2812_dev));

    // setup cdev
    result = ws2812_setup_cdev(&ws2812_dev);

    // return
    if (result) {
        unregister_chrdev_region(dev, 1);
        LOGE("WS2812B LED Kernel Module not loaded due to error. (%d)", result);
    } else {
        LOG("WS2812B LED Kernel Module Loaded!");
    }
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
     * FREE STRUCTS
     *****************************/
    LOG("> De-initializing LED list...");
    free_led(ws2812_dev.strip);
    ws2812_dev.strip = NULL;

    /*****************************
     * UNREGISTER DEVICE
     *****************************/

    // get device info
    dev_t devno = MKDEV(ws2812_major, ws2812_minor);

    // delete and unregister device
    cdev_del(&ws2812_dev.cdev);
    unregister_chrdev_region(devno, 1);
    
    // function complete
    LOG("WS2812B LED Kernel Module Cleaned Up!");
}

/**
 * module init/exit functions
 */
module_init(ws2812_init);
module_exit(ws2812_exit);
