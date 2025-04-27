// open firmware match table
static struct of_device_id hw_match_table[] = {
    {
        .compatible = serial,
    },
};

// platform driver structure
static struct platform_driver hw_pdriver = {
    .driver = {
        .name = MODULE_NAME,
        .owner = THIS_MODULE,
        .of_match_table = hw_match_table,
    },
    .probe = hw_probe,
    .remove = hw_remove,
};

MODULE_AUTHOR("Alex Rhodes");
MODULE_LICENSE("GPL");