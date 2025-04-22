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
        *BCM_GPIO_REG(BCM_GPIO_GPCLR0) = BCM_GPIO_GPCLRN_SET(1, pin);
    } else {
        *BCM_GPIO_REG(BCM_GPIO_GPCLR1) = BCM_GPIO_GPCLRN_SET(1, pin);
    }

    // return success
    return 0;
}

/**
 * cm_configure()
 * 
 * Configure the clock manager peripheral
 */
static int cm_configure(void) {
    // function setup
    unsigned int *cm_pwmctl = BCM_CM_REG(BCM_CM_PWMCTL);
    unsigned int *cm_pwmdiv = BCM_CM_REG(BCM_CM_PWMDIV);

    // disable clocks and wait until ready
    REG_WRITE_FIELD(cm_pwmctl, BCM_CM_PASSWD_MASK, 0 | BCM_CM_PASSWD);
    while(*cm_pwmctl & BCM_CM_PWMCTL_BUSY_MASK);
    
    // set divisor
    REG_WRITE_FIELD(cm_pwmdiv, BCM_CM_PASSWD_MASK | BCM_CM_PWMDIV_MASK,
                                BCM_CM_PASSWD | (WS2812_CLK_DIV << 12) | WS2812_CLK_DIV_FRAC);

    // enable clock with PLLD (source = 6)
    REG_WRITE_FIELD(cm_pwmctl, BCM_CM_PASSWD_MASK | BCM_CM_PWMCTL_SRC_MASK | BCM_CM_PWMCTL_ENAB_MASK,
                                BCM_CM_PASSWD | 6 | 1 << 4);

    // return
    return 0;
}

/**
 * pwm_configure()
 * 
 * Configure PWM peripheral using memory-mapped physical address
 */
static int pwm_configure(void) {
    // function setup
    unsigned int *pwm_ctl = BCM_PWM_REG(BCM_PWM_CTL);
    unsigned int *pwm_dmac = BCM_PWM_REG(BCM_PWM_DMAC);
    unsigned int *pwm_rng1 = BCM_PWM_REG(BCM_PWM_RNG1);
    unsigned int *pwm_dat1 = BCM_PWM_REG(BCM_PWM_DAT1);
    // unsigned int *pwm_fif1 = BCM_PWM_REG(BCM_PWM_FIF1);

    // disable PWM for configuration
    REG_WRITE_FIELD(pwm_ctl, BCM_PWM_CTL_PWEN1_MASK, 0);
    
    // configure CTL register
    REG_WRITE_FIELD(pwm_ctl, BCM_PWM_CTL_MODE1_MASK, 0);        // PWM mode
    REG_WRITE_FIELD(pwm_ctl, BCM_PWM_CTL_SBIT1_MASK, 1);        // Pull HIGH between transfers
    REG_WRITE_FIELD(pwm_ctl, BCM_PWM_CTL_USEF1_MASK, 0);        // Disable FIFO (TODO: change after testing)
    REG_WRITE_FIELD(pwm_ctl, BCM_PWM_CTL_MSEN1_MASK, 1);        // Enable Mark-Space mode

    // configure DMAC register
    REG_WRITE_FIELD(pwm_dmac, BCM_PWM_DMAC_ENAB_MASK, 0);       // Disable DMA (TODO: change after testing)

    // configure RNG1 register
    REG_WRITE_FIELD(pwm_rng1, BCM_PWM_RNG1_MASK, 0x0000000A);   // Set the RNG1 register to 0xA (=10)
    
    // configure DAT1 register
    REG_WRITE_FIELD(pwm_dat1, BCM_PWM_DAT1_MASK, 0x00000007);   // Set the DAT1 register to 0x7 (=7)

    // configuration complete; enable PWM
    REG_WRITE_FIELD(pwm_ctl, BCM_PWM_CTL_PWEN1_MASK, 1);

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
    gpio_registers = (int *)ioremap(BCM_GPIO_BASE_ADDRESS, PAGE_SIZE);
    if (gpio_registers == NULL) {
        LOGE("> GPIO peripheral cannot be remapped.");
        return -ENOMEM;
    } else {
        LOG("> GPIO peripheral mapped in memory at 0x%p.", gpio_registers);
    }

    // remap the PWM peripheral's physical address to a driver-usable one
    pwm_registers = (int *)ioremap(BCM_PWM_BASE_ADDRESS, PAGE_SIZE);
    if (pwm_registers == NULL) {
        LOGE("> PWM peripheral cannot be remapped.");
        iounmap(gpio_registers);
        return -ENOMEM;
    } else {
        LOG("> PWM peripheral mapped in memory at 0x%p.", pwm_registers);
    }

    // remap the CM peripheral's physical address to a driver-usable one
    cm_registers = (int *)ioremap(BCM_CM_BASE_ADDRESS, PAGE_SIZE);
    if (cm_registers == NULL) {
        LOGE("> CM peripheral cannot be remapped.");
        iounmap(pwm_registers);
        iounmap(gpio_registers);
        return -ENOMEM;
    } else {
        LOG("> CM peripheral mapped in memory at 0x%p.", pwm_registers);
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
    cm_configure();
    pwm_configure();
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
