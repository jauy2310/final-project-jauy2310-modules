#include "led.h"

// register configuration functions
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

// File operations
static ssize_t led_read(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
    char state = led_state ? '1' : '0';
    if (*ppos > 0)
        return 0; // EOF

    if (copy_to_user(buf, &state, 1))
        return -EFAULT;

    *ppos += 1;
    return 1;
}

static ssize_t led_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos)
{
    char kbuf;

    if (count < 1)
        return -EINVAL;

    if (copy_from_user(&kbuf, buf, 1))
        return -EFAULT;

    if (kbuf == '1') {
        led_state = 1;
        pr_info("LED turned ON\n");
        gpio_set(LED_PIN);
    } else if (kbuf == '0') {
        led_state = 0;
        pr_info("LED turned OFF\n");
        gpio_clear(LED_PIN);
    } else {
        return -EINVAL;
    }

    return 1;
}

static int led_open(struct inode *inode, struct file *file)
{
    pr_info("LED device opened\n");
    return 0;
}

static int led_release(struct inode *inode, struct file *file)
{
    pr_info("LED device closed\n");
    return 0;
}

static const struct file_operations led_fops = {
    .owner = THIS_MODULE,
    .read = led_read,
    .write = led_write,
    .open = led_open,
    .release = led_release,
};

// Misc device structure
static struct miscdevice led_misc_device = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = DEVICE_NAME,
    .fops = &led_fops,
};

// Platform driver
static int led_probe(struct platform_device *pdev)
{
    int ret;

    pr_info("LED platform device probed\n");

    ret = misc_register(&led_misc_device);
    if (ret) {
        pr_err("Failed to register misc device\n");
        return ret;
    }

    return 0;
}

static int led_remove(struct platform_device *pdev)
{
    pr_info("LED platform device removed\n");
    misc_deregister(&led_misc_device);
    return 0;
}

static struct platform_driver led_platform_driver = {
    .driver = {
        .name = "led_device",
        .owner = THIS_MODULE,
    },
    .probe = led_probe,
    .remove = led_remove,
};

// Manual platform device (optional, for quick testing without DT)
static struct platform_device *led_platform_device;

// Module init and exit
static int __init led_init(void)
{
    int ret;

    pr_info("LED driver initializing\n");

    // remap the GPIO peripheral's physical address to a driver-usable one
    gpio_registers = (volatile unsigned int *)ioremap(GPIO_BASE_ADDRESS, PAGE_SIZE);
    if (gpio_registers == NULL) {
        LOGE("> GPIO peripheral cannot be remapped.");
        return -ENOMEM;
    } else {
        LOG("> GPIO peripheral mapped in memory at 0x%p.", gpio_registers);
    }

    gpio_configure(LED_PIN, GPFSEL_OUTPUT);
    gpio_set(LED_PIN);

    ret = platform_driver_register(&led_platform_driver);
    if (ret)
        return ret;

    // Register platform device manually (if no device tree)
    led_platform_device = platform_device_register_simple("led_device", -1, NULL, 0);
    if (IS_ERR(led_platform_device)) {
        platform_driver_unregister(&led_platform_driver);
        return PTR_ERR(led_platform_device);
    }

    return 0;
}

static void __exit led_exit(void)
{
    platform_device_unregister(led_platform_device);
    platform_driver_unregister(&led_platform_driver);

    gpio_clear(LED_PIN);
    
    // unmap the GPIO peripheral from memory
    if (gpio_registers != NULL) {
        LOG("> Unmapping GPIO peripheral.");
        iounmap(gpio_registers);
    }

    pr_info("LED driver exiting\n");
}

module_init(led_init);
module_exit(led_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("Simple Platform Driver with /dev/led");
