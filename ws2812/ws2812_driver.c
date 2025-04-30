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
// define a global device struct
struct ws2812_dev ws2812_device;
static struct platform_device *ws2812_platform_device;

static struct platform_driver ws2812_platform_driver = {
    .driver = {
        .name = WS2812_MODULE_NAME,
        .owner = THIS_MODULE,
    },
    .probe = ws2812_probe,
    .remove = ws2812_remove,
};

// file operations table
static const struct file_operations ws2812_fops = {
    // file operations
    .owner = THIS_MODULE,
    .open = ws2812_open,
    .write = ws2812_write,
};

// open function
static int ws2812_open(struct inode *inode, struct file *file) {
    file->private_data = &ws2812_device;
    return 0;
}

// write function
static ssize_t ws2812_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos) {
    char color_str[8] = {0};
    uint8_t first, second, third;

    if (count < 7) {
        return -EINVAL;
    }

    if (copy_from_user(color_str, buf, 7)) {
        return -EFAULT;
    }

    if (color_str[0] != '#') {
        return -EINVAL;
    }

    // parse hex RGB values
    if (sscanf(color_str + 1, "%02hhx%02hhx%02hhx", &first, &second, &third) != 3) {
        return -EINVAL;
    }

    LOG("Set LED strip to #%02X%02X%02X", first, second, third);

    // set all LEDs to same color
    for (int i = 0; i < WS2812_MAX_LEDS; i++) {
        ws2812_device.leds[i].red   = second;
        ws2812_device.leds[i].green = first;
        ws2812_device.leds[i].blue  = third;
    }

    // encode LED array to DMA buffer
    encode_leds_to_dma(&ws2812_device);

    // start DMA transfer
    restart_dma_transfer();

    return count;
}

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
static int cm_configure(pwmctl_src_t src, uint32_t div, pwmctl_mash_t mash) {         
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
    *cm_pwmdiv = (CM_PASSWD) | (*cm_pwmdiv & ~CM_PWMDIV_MASK) | (CM_PWMDIV(div));
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
    udelay(DELAY_SHORT);

    // configure the CTL register
    LOG("+ Configuring CTL register.");
    *pwm_ctl &= ~(PWM_CTL_MODE1_MASK);          // set to PWM mode
    *pwm_ctl &= ~(PWM_CTL_SBIT1_MASK);          // pull LOW between transfers
    *pwm_ctl |= PWM_CTL_USEF1(1);               // enable FIFO
    *pwm_ctl |= PWM_CTL_MSEN1(1);               // enable Mark-Space (M/S) mode
    LOG("+ PWM_CTL [%p]: 0x%08X", pwm_ctl, *pwm_ctl);

    // configure the DMAC register
    LOG("+ Configuring DMAC register.");
    *pwm_dmac |= PWM_DMAC_ENAB(1);         // enable DMA
    LOG("+ PWM_DMAC [%p]: 0x%08X", pwm_dmac, *pwm_dmac);
    
    // configure the RNG1 register
    LOG("+ Configuring RNG1 register.");
    *pwm_rng1 = PWM_RNG1(100);                  // set the range to 100 (percentage-based duty cycle)
    LOG("+ PWM_RNG1 [%p]: 0x%08X", pwm_rng1, *pwm_rng1);
    udelay(DELAY_SHORT);

    // configure the DAT1 register
    LOG("+ Configuring DAT1 register.");
    *pwm_dat1 = PWM_DAT1(25);                   // set the duty cycle
    LOG("+ PWM_DAT1 [%p]: 0x%08X", pwm_dat1, *pwm_dat1);
    udelay(DELAY_SHORT);

    // configuration complete; enable PWM
    LOG("+ PWM Configuration Complete! Enabling peripheral.");
    *pwm_ctl |= PWM_CTL_PWEN1(1);
    LOG("+ PWM_CTL after enabling: 0x%08X", *pwm_ctl);

    // return
    return 0;
}

/**
 * pwm_setduty()
 * 
 * Sets the duty cycle of the PWM module
 */
static int pwm_setduty(int duty) {
    // function setup
    volatile unsigned int *pwm_ctl = PWM_REG(PWM_CTL_OFFSET);
    volatile unsigned int *pwm_dat1 = PWM_REG(PWM_DAT1_OFFSET);

    // disable PWM for changing duty cycle
    LOG("+ Disabling PWM to change duty cycle.");
    *pwm_ctl &= ~(PWM_CTL_PWEN1_MASK);

    // modify the duty cycle (DAT1)
    LOG("+ Changing duty cycle to %d", duty);
    *pwm_dat1 &= ~(PWM_DAT1_MASK);
    *pwm_dat1 |= PWM_DAT1(duty);

    // enable PWM
    LOG("+ Duty cycle changed! Enabling PWM.");
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
    udelay(DELAY_SHORT);

    // allocate a DMA-accessible buffer for DMA transfers
    if (!ws2812_device.dma_buffer) {
        LOG("+ Allocating DMA-accessible memory buffer (device: %p).", ws2812_device.mdev.this_device);
        ws2812_device.dma_buffer = dma_alloc_coherent(
            ws2812_device.device,
            WS2812_DMA_BUFFER_LEN * sizeof(uint32_t),
            &ws2812_device.dma_buffer_phys,
            GFP_KERNEL
        );
        if (!ws2812_device.dma_buffer) {
            LOGE("- Failed to allocate DMA buffer.");
            return -ENOMEM;
        }
    }
    
    // create a control block structure
    LOG("+ Allocating DMA-accessible control block.");
    ws2812_device.dma_cb = dma_alloc_coherent(ws2812_device.device, sizeof(dma_cb_t), &ws2812_device.cb_phys, GFP_KERNEL);
    if (!ws2812_device.dma_cb) {
        LOGE("- Error allocating memory for DMA handle.");
        return -ENOMEM;
    }

    // fill the control block
    // DMA controller uses the bus addresses, not the virtually-mapped addresses, so dest_ad = bus address
    LOG("+ Configuring DMA control block structure.");
    ws2812_device.dma_cb->ti = DMA_TI_SRCINC(1) | DMA_TI_DESTDREQ(1) | DMA_TI_PERMAP(DMA_PERMAP_PWM);
    ws2812_device.dma_cb->source_ad = ws2812_device.dma_buffer_phys;
    ws2812_device.dma_cb->dest_ad = PWM_BUS_BASE_ADDRESS + PWM_FIF1_OFFSET;
    ws2812_device.dma_cb->txfr_len = WS2812_DMA_BUFFER_LEN * sizeof(uint32_t);
    ws2812_device.dma_cb->stride = 0;
    ws2812_device.dma_cb->nextconbk = 0; // TODO: test if we want to repeat or not
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

static void restart_dma_transfer(void) {
    // log
    LOG("+ Restarting DMA transfer.");

    // get a reference to each of the control registers for DMA and PWM
    volatile unsigned int *dma_cs = DMA_REG(DMA_CS_OFFSET);
    volatile unsigned int *dma_conblkad = DMA_REG(DMA_CONBLKAD_OFFSET);
    volatile unsigned int *pwm_ctl = PWM_REG(PWM_CTL_OFFSET);
    volatile unsigned int *pwm_dmac = PWM_REG(PWM_DMAC_OFFSET);

    // stop the DMA channel
    *dma_cs = DMA_CS_RESET(1);
    udelay(DELAY_SHORT);

    // disable PWM temporarily
    *pwm_ctl &= ~(PWM_CTL_PWEN1_MASK);
    udelay(DELAY_SHORT);

    // clear the FIFO
    *pwm_ctl |= PWM_CTL_CLRF1(1);
    udelay(DELAY_SHORT);

    // stop pwm dmac
    *pwm_dmac &= ~(PWM_DMAC_ENAB_MASK);
    *pwm_dmac |= PWM_DMAC_ENAB(1);

    // set up the control block pointer again
    *dma_conblkad = ws2812_device.cb_phys;

    // re-enable DMA
    *dma_cs = DMA_CS_ACTIVE(1);
    udelay(DELAY_LONG);

    // re-enable PWM
    *pwm_ctl |= PWM_CTL_USEF1(1) | PWM_CTL_MSEN1(1);
    *pwm_ctl |= PWM_CTL_PWEN1(1);

    // log register state
    LOG("+ [Post-DMA restart] DMA CS: 0x%08X", *dma_cs);
    LOG("+ [Post-DMA restart] PWM CTL: 0x%08X", *pwm_ctl);
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
        dma_free_coherent(
            ws2812_device.device,
            sizeof(dma_cb_t),
            ws2812_device.dma_cb,
            ws2812_device.cb_phys
        );
        ws2812_device.dma_cb = NULL;
        ws2812_device.cb_phys = 0;
    }

    // free DMA buffer
    if (ws2812_device.dma_buffer != NULL) {
        dma_free_coherent(
            ws2812_device.device,
            WS2812_DMA_BUFFER_LEN * sizeof(uint32_t),
            ws2812_device.dma_buffer,
            ws2812_device.dma_buffer_phys
        );
        ws2812_device.dma_buffer = NULL;
    }

    // dma cleaned up
    LOG("DMA deconfiguration complete.");
}

/**
 * encode_leds_to_dma()
 * 
 * Encodes the LEDs represented using an array of led_t into duty cycle pulses in the DMA buffer
 */
void encode_leds_to_dma(struct ws2812_dev *dev) {
    // log
    LOG("+ Encoding LEDs to DMA buffer.");

    // get a pointer to the start of the dma buffer
    uint32_t *dma_buf = dev->dma_buffer;

    // process a single LED
    for (int i = 0; i < WS2812_MAX_LEDS; i++) {
        // if i is under the num_leds turn on
        if (i < dev->num_leds) {
            uint32_t color = (
                (dev->leds[i].green << 16) | \
                (dev->leds[i].red   << 8)  | \
                (dev->leds[i].blue)
            );
            
            for (int b = 23; b >= 0; b--) {
                if (color & (1 << b)) {
                    *dma_buf++ = PULSE_BIT_1;
                } else {
                    *dma_buf++ = PULSE_BIT_0;
                }
            }
        } else {
            for (int b = 23; b >= 0; b--) {
                *dma_buf++ = PULSE_BIT_0;
            }
        }
    }

    for (int i = 0; i < WS2812_RESET_LATCH_BITS; i++) {
        *dma_buf++ = 0;
    }
}

static void log_dma_buffer_for_all_leds(struct ws2812_dev *dev) {
    if (!dev->dma_buffer) {
        LOGE("DMA buffer is NULL.");
        return;
    }

    printk("=== DMA BUFFER ENCODING FOR ALL %d LEDS ===\n", dev->num_leds);

    for (int led = 0; led < dev->num_leds; led++) {
        printk("--- LED %d ---\n", led);
        for (int bit = 0; bit < 24; bit++) {
            int index = led * 24 + bit;
            uint32_t val = dev->dma_buffer[index];
            const char *bit_type = (val == PULSE_BIT_1) ? "1" :
                                   (val == PULSE_BIT_0) ? "0" : "INVALID";
            printk("Bit %3d: %3u [%s]\n", index, val, bit_type);
        }
    }

    printk("===========================================\n");
}

/**************************************************************************************
 * MODULE LOAD/UNLOAD FUNCTIONS
 **************************************************************************************/

/**
 * ws2812_probe()
 * 
 * Probes the module; main entry point
 */
static int ws2812_probe(struct platform_device *pdev) {
    // function setup
    int retval;

    // log
    LOG("> Probing WS2812 Module.");

    // store reference to device in the overarching device struct
    ws2812_device.device = &pdev->dev;

    // initialize the misc device
    ws2812_device.mdev.minor = MISC_DYNAMIC_MINOR;
    ws2812_device.mdev.name = WS2812_MODULE_NAME;
    ws2812_device.mdev.fops = &ws2812_fops;

    // register misc device
    retval = misc_register(&ws2812_device.mdev);
    if (retval) {
        LOGE("- Error registering misc device");
        return retval;
    }

    // configure peripherals
    LOG("> Configuring GPIO.");
    gpio_configure(WS2812_GPIO_PIN, GPFSEL_ALT5);

    LOG("> Configuring CM.");
    cm_configure(PWMCTL_PLLD, PWMDIV_REGISTER, PWMCTL_MASH1STAGE);

    LOG("> Configuring PWM.");
    pwm_configure();

    LOG("> Configuring DMA.");
    dma_configure();

    // peripherals configured; turn on led strip
    // pre-populate the LED buf with some LEDs
    for (int i = 0; i < WS2812_MAX_LEDS; i++) {
        ws2812_device.leds[i].red = 0xFF;
        ws2812_device.leds[i].green = 0x00;
        ws2812_device.leds[i].blue = 0x00;
    }

    // set the max leds
    ws2812_device.num_leds = WS2812_MAX_LEDS;
    for (int i = 0; i < 5; i++) {
        ws2812_device.leds[i].green = test_colors[i][1];
        ws2812_device.leds[i].red   = test_colors[i][0];
        ws2812_device.leds[i].blue  = test_colors[i][2];
    }
    encode_leds_to_dma(&ws2812_device);
    log_dma_buffer_for_all_leds(&ws2812_device);
    restart_dma_transfer();

    // success
    return 0;
}

/**
 * ws2812_remove()
 * 
 * Removes the module; main exit point
 */
static int ws2812_remove(struct platform_device *pdev) {
    // log
    LOG("> Removing WS2812 Module.");

    // deconfigure DMA
    dma_cleanup();

    // turn off an LED and configure GPIO to default
    gpio_clear(WS2812_GPIO_PIN);
    gpio_configure(WS2812_GPIO_PIN, GPFSEL_INPUT);

    // de-register device
    misc_deregister(&ws2812_device.mdev);

    // return
    return 0;
}

/**
 * ws2812_init()
 * 
 * Module init function
 */
static int __init ws2812_init(void) {

    /*****************************
     * START MODULE ENTRY
     *****************************/
    // initialization setup
    int retval = 0;
    
    // function setup
    LOG("> Initializing WS2812 Module.");

    /*****************************
     * INITIALIZE
     *****************************/
    // remap the GPIO peripheral's physical address to a driver-usable one
    gpio_registers = (volatile unsigned int *)ioremap(GPIO_BASE_ADDRESS, PAGE_SIZE);
    if (gpio_registers == NULL) {
        LOGE("- GPIO peripheral cannot be remapped.");
        return -ENOMEM;
    } else {
        LOG("> GPIO peripheral mapped in memory at 0x%p.", gpio_registers);
    }

    // remap the PWM peripheral's physical address to a driver-usable one
    pwm_registers = (volatile unsigned int *)ioremap(PWM_BASE_ADDRESS, PAGE_SIZE);
    if (pwm_registers == NULL) {
        LOGE("- PWM peripheral cannot be remapped.");
        iounmap(gpio_registers);
        return -ENOMEM;
    } else {
        LOG("> PWM peripheral mapped in memory at 0x%p.", pwm_registers);
    }

    // remap the CM peripheral's physical address to a driver-usable one
    cm_registers = (volatile unsigned int *)ioremap(CM_BASE_ADDRESS, PAGE_SIZE);
    if (cm_registers == NULL) {
        LOGE("- CM peripheral cannot be remapped.");
        iounmap(pwm_registers);
        iounmap(gpio_registers);
        return -ENOMEM;
    } else {
        LOG("> CM peripheral mapped in memory at 0x%p.", cm_registers);
    }

    dma_registers = (volatile unsigned int *)ioremap(DMA_CHANNEL_BASE_ADDRESS, PAGE_SIZE);
    if (dma_registers == NULL) {
        LOGE("- DMA peripheral cannot be remapped.");
        iounmap(cm_registers);
        iounmap(pwm_registers);
        iounmap(gpio_registers);
        return -ENOMEM;
    } else {
        LOG("> DMA peripheral mapped in memory at 0x%p.", dma_registers);
    }

    /*****************************
     * DEVICE REGISTRATION
     *****************************/
    LOG("> Registering platform driver.");
    retval = platform_driver_register(&ws2812_platform_driver);
    if (retval) {
        return retval;
    }

    // Register platform device manually (if no device tree)
    LOG("> Registering platform device.");
    ws2812_platform_device = platform_device_register_simple(WS2812_MODULE_NAME, -1, NULL, 0);
    if (IS_ERR(ws2812_platform_device)) {
        platform_driver_unregister(&ws2812_platform_driver);
        return PTR_ERR(ws2812_platform_device);
    }

    /*****************************
     * POST-INIT ACTIONS
     *****************************/

    return 0;
}

/**
 * ws2812_exit()
 * 
 * Unloading the module
 */
static void __exit ws2812_exit(void) {
    /*****************************
     * START MODULE EXIT
     *****************************/
    // start driver cleanup
    LOG("Cleaning up WS2812B LED Kernel Module.");

    /*****************************
     * PRE-EXIT ACTIONS
     *****************************/

    /*****************************
     * UNREGISTER MODULE
     *****************************/
    platform_device_unregister(ws2812_platform_device);
    platform_driver_unregister(&ws2812_platform_driver);

    /*****************************
     * DE-INITIALIZE
     *****************************/
    // free the DMA-accessible memory for the dma_buffer
    if (ws2812_device.dma_buffer != NULL) {
        LOG("> Freeing DMA-accessible memory for the DMA buffer.");
        dma_free_coherent(
            ws2812_device.device,
            WS2812_DMA_BUFFER_LEN * sizeof(uint32_t),
            ws2812_device.dma_buffer,
            ws2812_device.dma_buffer_phys
        );
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
