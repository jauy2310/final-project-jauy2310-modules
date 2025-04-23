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
    volatile unsigned int *gpio_gpfseli;

    // check that pins are within bounds
    if (pin > NUM_GPIO_PINS) {
        LOGE("Error; cannot use GPIO pin outside of [0, %d]", NUM_GPIO_PINS);
        return -EINVAL;
    }

    // create a pointer to the selected pin's GPIO register
    // gpfsel registers each correspond to 10 pins each, using an offset at (pin / 10)
    gpio_gpfseli = (volatile unsigned int *)(gpio_registers + (pin / 10));

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
    volatile unsigned int *gpio_gpsetn;

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
    volatile unsigned int *gpio_gpclrn;

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
static int cm_configure(pwmctl_src_t src, pwmctl_mash_t mash) {         
    // function setup
    volatile unsigned int *cm_pwmctl = CM_REG(CM_PWMCTL_OFFSET);
    volatile unsigned int *cm_pwmdiv = CM_REG(CM_PWMDIV_OFFSET);

    // disable clocks and wait until the busy flag is cleared
    LOG("+ Disabling CM for configuration.");
    *cm_pwmctl = (CM_PASSWD) | (*cm_pwmctl & ~CM_PWMCTL_ENAB_MASK) | (CM_PWMCTL_ENAB(0));

    // waiting on busy flag
    LOG("+ Waiting for BUSY flag to go low...");
    int timeout = 100000;
    while((*cm_pwmctl & CM_PWMCTL_BUSY_MASK) && --timeout);
    if (timeout == 0) {
        LOGE("- BUSY flag never goes low.");
    }

    // configure the clock divider
    LOG("+ Configuring the clock divider.");
    *cm_pwmdiv = (CM_PASSWD) | (*cm_pwmdiv & ~CM_PWMDIV_MASK) | (CM_PWMDIV(PWMDIV_REGISTER));
    LOG("+ CM_PWMDIV: 0x%08X", *cm_pwmdiv);

    // configure the clock source and MASH
    LOG("+ Configuring PWMCTL register.");
    *cm_pwmctl = (CM_PASSWD) | (*cm_pwmctl & ~CM_PWMCTL_SRC_MASK);
    *cm_pwmctl |= (CM_PASSWD) | (CM_PWMCTL_SRC(src));
    *cm_pwmctl = (CM_PASSWD) | (*cm_pwmctl & ~CM_PWMCTL_MASH_MASK);
    *cm_pwmctl |= (CM_PASSWD) | (CM_PWMCTL_MASH(mash));
    LOG("+ CM_PWMCTL: 0x%08X", *cm_pwmctl);

    // enable clocks and wait until the busy flag turns on
    LOG("+ CM Configuration Complete! Enabling peripheral.");
    *cm_pwmctl |= (CM_PASSWD) | (CM_PWMCTL_ENAB(1));

    timeout = 100000;
    while((!(*cm_pwmctl & CM_PWMCTL_BUSY_MASK)) && --timeout);
    if (timeout == 0) {
        LOGE("- BUSY flag never goes high.");
    }

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
    volatile unsigned int *pwm_ctl = PWM_REG(PWM_CTL_OFFSET);
    volatile unsigned int *pwm_dmac = PWM_REG(PWM_DMAC_OFFSET);
    volatile unsigned int *pwm_rng1 = PWM_REG(PWM_RNG1_OFFSET);
    volatile unsigned int *pwm_dat1 = PWM_REG(PWM_DAT1_OFFSET);

    // disable PWM for configuration
    LOG("+ Disabling PWM for configuration.");
    *pwm_ctl &= ~(PWM_CTL_PWEN1_MASK);

    // configure the CTL register
    LOG("+ Configuring CTL register.");
    *pwm_ctl &= ~(PWM_CTL_MODE1_MASK);          // set to PWM mode
    *pwm_ctl &= ~(PWM_CTL_SBIT1_MASK);          // pull LOW between transfers (TODO: change after testing)
    *pwm_ctl &= ~(PWM_CTL_USEF1_MASK);          // disable FIFO (TODO: change after testing)
    *pwm_ctl &= ~(PWM_CTL_MSEN1_MASK);
    *pwm_ctl |= PWM_CTL_MSEN1(1);               // enable Mark-Space (M/S) mode

    // configure the DMAC register
    LOG("+ Configuring DMAC register.");
    *pwm_dmac &= ~(PWM_DMAC_ENAB_MASK);         // disable DMA (TODO: change after testing)
    
    // configure the RNG1 register
    LOG("+ Configuring RNG1 register.");
    *pwm_rng1 &= ~(PWM_RNG1_MASK);
    *pwm_rng1 |= PWM_RNG1(100);                 // set the range to 100 (percentage-based duty cycle)

    // configure the DAT1 register
    LOG("+ Configuring DAT1 register.");
    *pwm_dat1 &= ~(PWM_DAT1_MASK);
    *pwm_dat1 |= PWM_DAT1(25);                  // set the duty cycle

    // configuration complete; enable PWM
    LOG("+ PWM Configuration Complete! Enabling peripheral.");
    *pwm_ctl |= PWM_CTL_PWEN1(1);

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
    gpio_registers = (volatile unsigned int *)ioremap(GPIO_BASE_ADDRESS, PAGE_SIZE);
    if (gpio_registers == NULL) {
        LOGE("> GPIO peripheral cannot be remapped.");
        return -ENOMEM;
    } else {
        LOG("> GPIO peripheral mapped in memory at 0x%p.", gpio_registers);
    }

    // remap the PWM peripheral's physical address to a driver-usable one
    pwm_registers = (volatile unsigned int *)ioremap(PWM_BASE_ADDRESS, PAGE_SIZE);
    if (pwm_registers == NULL) {
        LOGE("> PWM peripheral cannot be remapped.");
        iounmap(gpio_registers);
        return -ENOMEM;
    } else {
        LOG("> PWM peripheral mapped in memory at 0x%p.", pwm_registers);
    }

    // remap the CM peripheral's physical address to a driver-usable one
    cm_registers = (volatile unsigned int *)ioremap(CM_BASE_ADDRESS, PAGE_SIZE);
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
    LOG("> Configuring GPIO.");
    gpio_configure(WS2812_GPIO_PIN, GPFSEL_OUTPUT);
    
    LOG("> Configuring CM.");
    cm_configure(PWMCTL_PLLD, PWMCTL_MASH1STAGE);

    LOG("> Configuring PWM.");
    pwm_configure();

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
