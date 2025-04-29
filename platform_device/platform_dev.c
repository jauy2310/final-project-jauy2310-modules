#include "platform_dev.h"

// module information
MODULE_AUTHOR("Jake Uyechi");
MODULE_LICENSE("GPL");

/**************************************************************************************
 * MODULE GLOBALS
 **************************************************************************************/

// define device major/minor numbers
static int ws2812_major = 0;
static int ws2812_minor = 0;

// define a global reference to device struct
struct ws2812_dev ws2812_device;
static dev_t ws2812_devno;

// create file operations table
static const struct file_operations ws2812_fops = {
    .owner = THIS_MODULE,
    .open = ws2812_open,
    .release = ws2812_release,
    .write = ws2812_write
};

// open firmware match table
static const struct of_device_id ws2812_match_table[] = {
    { .compatible = "ws2812" },
    {},
};

// platform driver structure
static struct platform_driver ws2812_pdriver = {
    .driver = {
        .name = MODULE_NAME,
        .owner = THIS_MODULE,
        .of_match_table = ws2812_match_table,
    },
    .probe = ws2812_probe,
    .remove = ws2812_remove,
};

/**************************************************************************************
 * MODULE IMPLEMENTATION
 **************************************************************************************/

static int ws2812_open(struct inode *inode, struct file *file) {
    LOG("Device opened.");
    return 0;
}

static int ws2812_release(struct inode *inode, struct file *file) {
    LOG("Device released.");
    return 0;
}

static ssize_t ws2812_write(struct file *file, const char __user *buf, size_t len, loff_t *offset) {
    LOG("WS2812 Write (size: %zu)", len);
    return len;
}

/**************************************************************************************
 * MODULE INIT/EXIT/PROBE/REMOVE FUNCTIONS
 **************************************************************************************/

// module probe (for platform driver)
static int ws2812_probe(struct platform_device *pdev) {
    // function setup
    int retval = 0;
    LOG("Probing Test Platform Driver.");

    // create area in memory for the char device
    retval = alloc_chrdev_region(&ws2812_devno, ws2812_minor, 1, MODULE_NAME);
    if (retval != 0) {
        LOGE("Error allocating character device region.");
        goto cleanup;
    }
    ws2812_major = MAJOR(ws2812_devno);
    memset(&ws2812_device, 0, sizeof(struct ws2812_dev));

    // initialize char device
    cdev_init(&ws2812_device.cdev, &ws2812_fops);
    ws2812_device.cdev.owner = THIS_MODULE;

    // add cdev to kernel
    retval = cdev_add(&ws2812_device.cdev, ws2812_devno, 1);
    if (retval != 0) {
        LOGE("Error adding cdev.");
        goto unregister_chrdev;
    }

    // create device class
    ws2812_device.class = class_create(MODULE_NAME);
    if (IS_ERR(ws2812_device.class)) {
        retval = PTR_ERR(ws2812_device.class);
        LOGE("Error creating class.");
        goto del_cdev;
    }

    // create device
    ws2812_device.device = device_create(ws2812_device.class, NULL, ws2812_devno, NULL, MODULE_NAME);
    if (IS_ERR(ws2812_device.device)) {
        retval = PTR_ERR(ws2812_device.device);
        LOGE("Error creating device.");
        goto destroy_class;
    }

    // return success
    return 0;

    // cleanup labels 
destroy_class:
    class_destroy(ws2812_device.class);
del_cdev:
    cdev_del(&ws2812_device.cdev);
unregister_chrdev:
    unregister_chrdev_region(ws2812_devno, 1);
cleanup:
    return retval;
}

// module remove (for platform driver)
static int ws2812_remove(struct platform_device *pdev) {
    // function setup
    LOG("Removing Test Platform Driver.");

    // destroy the device registration
    device_destroy(ws2812_device.class, ws2812_devno);

    // destroy the class registration
    class_destroy(ws2812_device.class);

    // delete the cdev
    cdev_del(&ws2812_device.cdev);

    // unregister the memory region for the char device
    unregister_chrdev_region(ws2812_devno, 1);
    
    // return
    return 0;
}


// module init
static int __init ws2812_init(void) {
    LOG("Loading Test Platform Driver.");
    return platform_driver_register(&ws2812_pdriver);
}

// module exit
static void __exit ws2812_exit(void) {
    LOG("Unloading Test Platform Driver.");
    platform_driver_unregister(&ws2812_pdriver);
}

// register module entry/exit points
module_init(ws2812_init);
module_exit(ws2812_exit);