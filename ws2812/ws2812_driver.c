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
    if (pin > NUM_GPIO_PINS) {
        LOGE("Error; cannot use GPIO pin outside of [0, %d]", NUM_GPIO_PINS);
        return -EINVAL;
    }

    // create a pointer to the selected pin's GPIO register
    // gpfsel registers each correspond to 10 pins each, using an offset at (pin / 10)
    gpio_gpfseli = gpio_registers + (pin / 10);

    // clear the target register bits and set the mode
    *gpio_gpfseli &= ~(GPIO_GPFSEL_MASK(pin));
    *gpio_gpfseli |= (GPIO_GPFSEL(pin, mode));

    // return success
    return 0;
}

/**
 * gpio_set()
 * 
 * Set a GPIO pin
 */
static int gpio_set(unsigned int pin) {
    // function setup
    unsigned int *gpio_gpsetn;

    // check that pins are within bounds
    if (pin > NUM_GPIO_PINS) {
        LOGE("Error; cannot use GPIO pin outside of [0, %d]", NUM_GPIO_PINS);
        return -EINVAL;
    }

    // create a pointer to the selected pin's GPSET register
    if (pin < 32) {
        gpio_gpsetn = GPIO_REG(GPIO_GPSET0_OFFSET);
    } else {
        gpio_gpsetn = GPIO_REG(GPIO_GPSET1_OFFSET);
    }

    // write a 1 to the set bit corresponding to the pin
    *gpio_gpsetn |= GPIO_GPSETN(pin);

    // return success
    return 0;
}

/**
 * gpio_clear()
 * 
 * Clear a GPIO pin
 */
static int gpio_clear(unsigned int pin) {
    // function setup
    unsigned int *gpio_gpclrn;

    // check that pins are within bounds
    if (pin > NUM_GPIO_PINS) {
        LOGE("Error; cannot use GPIO pin outside of [0, %d]", NUM_GPIO_PINS);
        return -EINVAL;
    }

    // create a pointer to the selected pin's GPSET register
    if (pin < 32) {
        gpio_gpclrn = GPIO_REG(GPIO_GPCLR0_OFFSET);
    } else {
        gpio_gpclrn = GPIO_REG(GPIO_GPCLR1_OFFSET);
    }

    // write a 1 to the clear bit corresponding to the pin
    *gpio_gpclrn |= GPIO_GPCLRN(pin);

    // return success
    return 0;
}

/**
 * cm_configure()
 * 
 * Configure the clock manager peripheral
 */
static int cm_configure(void) {         

    // return
    return 0;
}

/**
 * pwm_configure()
 * 
 * Configure PWM peripheral using memory-mapped physical address
 */
static int pwm_configure(void) {

    // return
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
    gpio_registers = (int *)ioremap(GPIO_BASE_ADDRESS, PAGE_SIZE);
    if (gpio_registers == NULL) {
        LOGE("> GPIO peripheral cannot be remapped.");
        return -ENOMEM;
    } else {
        LOG("> GPIO peripheral mapped in memory at 0x%p.", gpio_registers);
    }

    // remap the PWM peripheral's physical address to a driver-usable one
    pwm_registers = (int *)ioremap(PWM_BASE_ADDRESS, PAGE_SIZE);
    if (pwm_registers == NULL) {
        LOGE("> PWM peripheral cannot be remapped.");
        iounmap(gpio_registers);
        return -ENOMEM;
    } else {
        LOG("> PWM peripheral mapped in memory at 0x%p.", pwm_registers);
    }

    // remap the CM peripheral's physical address to a driver-usable one
    cm_registers = (int *)ioremap(CM_BASE_ADDRESS, PAGE_SIZE);
    if (cm_registers == NULL) {
        LOGE("> CM peripheral cannot be remapped.");
        iounmap(pwm_registers);
        iounmap(gpio_registers);
        return -ENOMEM;
    } else {
        LOG("> CM peripheral mapped in memory at 0x%p.", cm_registers);
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
    // unmap the CM peripheral from memory
    if (cm_registers != NULL) {
        LOG("> Unmapping CM peripheral.");
        iounmap(cm_registers);
    }
    // unmap the PWM peripheral from memory
    if (pwm_registers != NULL) {
        LOG("> Unmapping PWM peripheral.");
        iounmap(pwm_registers);
    }

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
