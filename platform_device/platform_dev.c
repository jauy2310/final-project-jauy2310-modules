// create file operations table
static const struct file_operations ws2812_fops = {
    .owner = THIS_MODULE,
    .open = ws2812_open,
    .release = ws2812_release,
    .write = ws2812_write
};

// open firmware match table
static struct of_device_id ws2812_match_table[] = {
    {
        .compatible = serial,
    },
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

// register the platform module
module_platform_driver(ws2812_pdriver);

// module information
MODULE_AUTHOR("Jake Uyechi");
MODULE_LICENSE("GPL");