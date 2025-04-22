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
 * HELPER FUNCTIONS
 **************************************************************************************/

/**
 * gpio_configure()
 * 
 * Configure the GPFSEL register in the GPIO peripheral
 */
static int gpio_configure(unsigned int pin, gpfsel_mode_t mode) {
    // function setup
    unsigned int *gpio_gpfseli;

    // check that pins are within bounds
    if (pin < 0 || pin > BCM_NUM_GPIO_PINS) {
        LOGE("Error; cannot use GPIO pin outside of [0, %d]", BCM_NUM_GPIO_PINS);
        return -EINVAL;
    }

    // create a pointer to the selected pin's GPIO register
    // gpfsel registers each correspond to 10 pins each, using an offset at (pin / 10)
    gpio_gpfseli = gpio_registers + (pin / 10);

    // momentarily disable the GPIO, set the mode, and enable it again
    *gpio_gpfseli &= ~(BCM_GPIO_GPFSELN_MASK(pin));
    *gpio_gpfseli |= (BCM_GPIO_GPFSELN_SET(mode, pin));

    // return success
    return 0;
}

/**
 * gpio_set()
 * 
 * Set a GPIO pin
 */
static int gpio_set(unsigned int pin) {
    // check that pins are within bounds
    if (pin < 0 || pin > BCM_NUM_GPIO_PINS) {
        LOGE("Error; cannot use GPIO pin outside of [0, %d]", BCM_NUM_GPIO_PINS);
        return -EINVAL;
    }

    // create a pointer to the selected pin's GPSET register
    if (pin < 32) {
        *BCM_GPIO_REG(BCM_GPIO_GPSET0) = BCM_GPIO_GPSETN_SET(1, pin);
    } else {
        *BCM_GPIO_REG(BCM_GPIO_GPSET1) = BCM_GPIO_GPSETN_SET(1, pin);
    }

    // return success
    return 0;
}

/**
 * gpio_clear()
 * 
 * Clear a GPIO pin
 */
static int gpio_clear(unsigned int pin) {
    // check that pins are within bounds
    if (pin < 0 || pin > BCM_NUM_GPIO_PINS) {
        LOGE("Error; cannot use GPIO pin outside of [0, %d]", BCM_NUM_GPIO_PINS);
        return -EINVAL;
    }

    // create a pointer to the selected pin's GPSET register
    if (pin < 32) {
        *BCM_GPIO_REG(BCM_GPIO_GPCLR0) = BCM_GPIO_GPSETN_SET(1, pin);
    } else {
        *BCM_GPIO_REG(BCM_GPIO_GPCLR1) = BCM_GPIO_GPSETN_SET(1, pin);
    }

    // return success
    return 0;
}

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
        iounmap(gpio_registers);
        return -ENOMEM;
    } else {
        LOG("> Entry created for module at /proc/%s", WS2812_MODULE_NAME);
    }

    /*****************************
     * POST-INIT ACTIONS
     *****************************/
    // configure GPIO and turn on an LED
    gpio_configure(WS2812_GPIO_PIN, GPFSEL_OUTPUT);
    gpio_set(WS2812_GPIO_PIN);

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
     * PRE-EXIT ACTIONS
     *****************************/
    // turn off an LED and configure GPIO to default
    gpio_clear(WS2812_GPIO_PIN);
    gpio_configure(WS2812_GPIO_PIN, GPFSEL_INPUT);

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
