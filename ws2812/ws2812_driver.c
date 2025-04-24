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
struct file_operations ws2812_fops = {
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
    LOG("+ Configuring GPFSEL%d register.", pin / 10);
    *gpio_gpfseli &= ~(GPIO_GPFSEL_MASK(pin));
    *gpio_gpfseli |= (GPIO_GPFSEL(pin, mode));
    LOG("+ GPIO_GPFSEL%d [%p]: 0x%08X", pin / 10, gpio_gpfseli, *gpio_gpfseli);

    // return success
    LOG("GPIO Configuration Complete! Enabling peripheral.");
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
    int timeout;
    volatile unsigned int *cm_pwmctl = CM_REG(CM_PWMCTL_OFFSET);
    volatile unsigned int *cm_pwmdiv = CM_REG(CM_PWMDIV_OFFSET);

    // disable clocks and wait until the busy flag is cleared
    LOG("+ Disabling CM for configuration.");
    *cm_pwmctl = (CM_PASSWD) | (*cm_pwmctl & ~CM_PWMCTL_ENAB_MASK) | (CM_PWMCTL_ENAB(0));

    // waiting on busy flag
    LOG("+ Waiting for BUSY flag to go low...");
    timeout = 100000;
    while((*cm_pwmctl & CM_PWMCTL_BUSY_MASK) && --timeout);
    if (timeout == 0) {
        LOGE("- BUSY flag never goes low.");
    }

    // configure the clock divider
    LOG("+ Configuring the clock divider.");
    *cm_pwmdiv = (CM_PASSWD) | (*cm_pwmdiv & ~CM_PWMDIV_MASK) | (CM_PWMDIV(PWMDIV_REGISTER));
    LOG("+ CM_PWMDIV [%p]: 0x%08X", cm_pwmdiv, *cm_pwmdiv);

    // configure the clock source and MASH
    LOG("+ Configuring PWMCTL register.");
    *cm_pwmctl = (CM_PASSWD) | (*cm_pwmctl & ~CM_PWMCTL_SRC_MASK);
    *cm_pwmctl |= (CM_PASSWD) | (CM_PWMCTL_SRC(src));
    *cm_pwmctl = (CM_PASSWD) | (*cm_pwmctl & ~CM_PWMCTL_MASH_MASK);
    *cm_pwmctl |= (CM_PASSWD) | (CM_PWMCTL_MASH(mash));
    LOG("+ CM_PWMCTL [%p]: 0x%08X", cm_pwmctl, *cm_pwmctl);

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
    *pwm_ctl |= PWM_CTL_USEF1(1);               // enable FIFO
    *pwm_ctl |= PWM_CTL_MSEN1(1);               // enable Mark-Space (M/S) mode
    LOG("+ PWM_CTL [%p]: 0x%08X", pwm_ctl, *pwm_ctl);

    // configure the DMAC register
    LOG("+ Configuring DMAC register.");
    *pwm_dmac |= PWM_DMAC_ENAB(1);              // enable DMA
    LOG("+ PWM_DMAC [%p]: 0x%08X", pwm_dmac, *pwm_dmac);
    
    // configure the RNG1 register
    LOG("+ Configuring RNG1 register.");
    *pwm_rng1 &= ~(PWM_RNG1_MASK);
    *pwm_rng1 |= PWM_RNG1(100);                 // set the range to 100 (percentage-based duty cycle)
    LOG("+ PWM_RNG1 [%p]: 0x%08X", pwm_rng1, *pwm_rng1);

    // configure the DAT1 register
    LOG("+ Configuring DAT1 register.");
    *pwm_dat1 &= ~(PWM_DAT1_MASK);
    *pwm_dat1 |= PWM_DAT1(25);                  // set the duty cycle
    LOG("+ PWM_DAT1 [%p]: 0x%08X", pwm_dat1, *pwm_dat1);

    // configuration complete; enable PWM
    LOG("+ PWM Configuration Complete! Enabling peripheral.");
    *pwm_ctl |= PWM_CTL_PWEN1(1);

    // return
    return 0;
}

/**
 * dma_configure()
 * 
 * Configure DMA peripheral using memory-mapped physical address
 */
static int dma_configure(void) {
    // function setup
    volatile unsigned int *dma_cs = DMA_REG(DMA_CS_OFFSET);
    volatile unsigned int *dma_conblkad = DMA_REG(DMA_CONBLKAD_OFFSET);

    // disable DMA channel
    LOG("+ Disabling DMA for configuration.");
    *dma_cs &= ~(DMA_CS_ACTIVE_MASK);
    
    // create a control block structure
    LOG("+ Allocating DMA-accessible control block.");
    ws2812_device.dma_cb = dma_alloc_coherent(ws2812_device.device, sizeof(dma_cb_t), &ws2812_device.cb_phys, GFP_KERNEL);
    if (ws2812_device.dma_cb == NULL) {
        LOGE("- Error allocating memory for DMA handle.");
        return -ENOMEM;
    }

    // fill the control block
    // DMA controller uses the bus addresses, not the virtually-mapped addresses, so dest_ad = bus address
    LOG("+ Configuring DMA control block structure.");
    ws2812_device.dma_cb->ti = DMA_TI_SRCINC(1) | DMA_TI_DESTDREQ(1) | DMA_TI_PERMAP(DMA_PERMAP_PWM);
    ws2812_device.dma_cb->source_ad = ws2812_device.dma_buffer_phys;
    ws2812_device.dma_cb->dest_ad = PWM_BUS_BASE_ADDRESS + PWM_FIF1_OFFSET;
    ws2812_device.dma_cb->txfr_len = 200; // TODO: change this to the length of the LED buffer; adding/removing leds require this to update
    ws2812_device.dma_cb->stride = 0;
    ws2812_device.dma_cb->nextconbk = ws2812_device.cb_phys; // repeat the buffer
    LOG("+ DMA control block allocated at %p (phys: %pa)", ws2812_device.dma_cb, &ws2812_device.cb_phys);

    // set the control block address
    LOG("+ Setting the configured control block to the DMA's settings.");
    *dma_conblkad = ws2812_device.cb_phys;

    // enable DMA channel
    LOG("+ DMA Configuration Complete! Enabling peripheral.");
    *dma_cs |= DMA_CS_ACTIVE(1);

    // return 
    return 0;
}

/**
 * dma_cleanup()
 * 
 * Deconfigures the DMA
 */
static void dma_cleanup(void) {
    // function setup
    volatile unsigned int *dma_cs = DMA_REG(DMA_CS_OFFSET);
    volatile unsigned int *dma_conblkad = DMA_REG(DMA_CONBLKAD_OFFSET);

    // disable DMA channel
    LOG("+ Disabling DMA channel.");
    *dma_cs &= ~(DMA_CS_ACTIVE_MASK);  // Clear the ACTIVE bit to stop the DMA transfer

    // clear the control block address
    LOG("+ Clearing DMA control block address.");
    *dma_conblkad = 0;  // Clear the control block address

    // free any allocated DMA resources if necessary
    if (ws2812_device.dma_cb != NULL) {
        dma_free_coherent(NULL, sizeof(dma_cb_t), ws2812_device.dma_cb, ws2812_device.cb_phys);  // Free the control block
        ws2812_device.dma_cb = NULL;
        ws2812_device.cb_phys = 0;
    }

    // dma cleaned up
    LOG("DMA deconfiguration complete.");
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
    dev_t dev;
    
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

    dma_registers = (volatile unsigned int *)ioremap(DMA_CHANNEL_BASE_ADDRESS, PAGE_SIZE);
    if (dma_registers == NULL) {
        LOGE("> DMA peripheral cannot be remapped.");
        iounmap(cm_registers);
        iounmap(pwm_registers);
        iounmap(gpio_registers);
        return -ENOMEM;
    } else {
        LOG("> DMA peripheral mapped in memory at 0x%p.", dma_registers);
    }

    /*****************************
     * REGISTER MODULE
     *****************************/
    // allocate a device
    dev = 0;
    result = alloc_chrdev_region(&dev, ws2812_minor, 1, WS2812_MODULE_NAME);
    ws2812_major = MAJOR(dev);
    if (result < 0) {
        LOGE("> Error getting device major.");
        return result;
    }
    memset(&ws2812_device, 0, sizeof(struct ws2812_dev));
    ws2812_dev_no = dev;

    // create a class
    #if LINUX_VERSION_CODE < KERNEL_VERSION(6, 1, 67)
        ws2812_class = class_create(THIS_MODULE, WS2812_MODULE_NAME);
    #else
        ws2812_class = class_create(WS2812_MODULE_NAME);
    #endif
    if (IS_ERR(ws2812_class)) {
        LOGE("> Error setting up a device class.");
        unregister_chrdev_region(ws2812_dev_no, 1);
        return PTR_ERR(ws2812_class);
    }

    // create a platform device
    ws2812_device.pdev = platform_device_register_simple("ws2812-dma", -1, NULL, 0);
    if (IS_ERR(ws2812_device.pdev)) {
        LOGE("> Error creating a platform device.");
        return PTR_ERR(ws2812_device.pdev);
    }

    // creeate a device
    ws2812_device.device = device_create(ws2812_class, &ws2812_device.pdev->dev, ws2812_dev_no, NULL, WS2812_MODULE_NAME);
    if (IS_ERR(ws2812_device.device)) {
        LOGE("> Error creating a device.");
        class_destroy(ws2812_class);
        unregister_chrdev_region(ws2812_dev_no, 1);
        return PTR_ERR(ws2812_class);
    }

    // allocate DMA buffer
    ws2812_device.dma_buffer = dma_alloc_coherent(ws2812_device.device, 200 * sizeof(uint32_t), &ws2812_device.dma_buffer_phys, GFP_KERNEL);
    if (ws2812_device.dma_buffer == NULL) {
        LOGE("> No memory to allocate the DMA buffer.");
        device_destroy(ws2812_class, ws2812_dev_no);
        class_destroy(ws2812_class);
        unregister_chrdev_region(ws2812_dev_no, 1);
        return -ENOMEM;
    }

    // populate the DMA buffer (TODO: remove this when implementing ws2812 control)
    for (int i = 0; i < 200; i++) {
        ws2812_device.dma_buffer[i] = (i < 100) ? i : 200 - i;
    }

    // initialize and register the char dev
    cdev_init(&ws2812_device.cdev, &ws2812_fops);
    ws2812_device.cdev.owner = THIS_MODULE;
    result = cdev_add(&ws2812_device.cdev, dev, 1);
    if (result < 0) {
        LOGE("> Error registering the char device: %d", result);
        dma_free_coherent(ws2812_device.device, 200 * sizeof(uint32_t), ws2812_device.dma_buffer, ws2812_device.dma_buffer_phys);
        device_destroy(ws2812_class, ws2812_dev_no);
        class_destroy(ws2812_class);
        unregister_chrdev_region(ws2812_dev_no, 1);
        return result;
    }

    /*****************************
     * POST-INIT ACTIONS
     *****************************/
    // configure GPIO and turn on an LED
    LOG("> Configuring GPIO.");
    gpio_configure(WS2812_GPIO_PIN, GPFSEL_ALT5);
    // gpio_set(WS2812_GPIO_PIN);
    
    LOG("> Configuring CM.");
    cm_configure(PWMCTL_PLLD, PWMCTL_MASH1STAGE);

    LOG("> Configuring PWM.");
    pwm_configure();

    LOG("> Configuring DMA.");
    dma_configure();

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
    // deconfigure DMA
    dma_cleanup();

    // turn off an LED and configure GPIO to default
    gpio_clear(WS2812_GPIO_PIN);
    gpio_configure(WS2812_GPIO_PIN, GPFSEL_INPUT);

    /*****************************
     * UNREGISTER MODULE
     *****************************/
    cdev_del(&ws2812_device.cdev);
    dma_free_coherent(ws2812_device.device, 200 * sizeof(uint32_t), ws2812_device.dma_buffer, ws2812_device.dma_buffer_phys);
    device_destroy(ws2812_class, ws2812_dev_no);
    class_destroy(ws2812_class);
    unregister_chrdev_region(ws2812_dev_no, 1);

    /*****************************
     * DE-INITIALIZE
     *****************************/
    // free the DMA-accessible memory for the dma_buffer
    if (ws2812_device.dma_buffer != NULL) {
        LOG("> Freeing DMA-accessible memory for the DMA buffer.");
        dma_free_coherent(ws2812_device.device, 200 * sizeof(uint32_t), ws2812_device.dma_buffer, ws2812_device.dma_buffer_phys);
        ws2812_device. dma_buffer = NULL;
    }

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
