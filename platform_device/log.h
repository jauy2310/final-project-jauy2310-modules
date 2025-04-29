#ifndef _LOG_H_
#define _LOG_H_

// include printk
#include <linux/printk.h>

// define a DEBUG flag
#define DEBUG 1

// define a set of LOG functions
// uses printk for kernel and printf for user
// if DEBUG is not set, ignore LOG statements
#undef LOG
#undef LOGW
#ifdef DEBUG
    #ifdef __KERNEL__
        #define LOG(fmt, args...) \
            printk(KERN_DEBUG "ws2812 [D]: [%s():%d] " fmt "\n", __func__, __LINE__, ## args)
        #define LOGW(fmt, args...) \
            printk(KERN_WARNING "ws2812 [W]: [%s():%d] " fmt "\n", __func__, __LINE__, ## args)
        #define LOGE(fmt, args...) \
            printk(KERN_ERR "ws2812 [E]: [%s():%d] " fmt "\n", __func__, __LINE__, ## args)
    #else
        #define LOG(fmt, args...) \
            printf("ws2812 [D]: [%s():%d] " fmt "\n", __func__, __LINE__, ## args)
        #define LOGW(fmt, args...) \
            printf("ws2812 [W]: [%s():%d] " fmt "\n", __func__, __LINE__, ## args)
        #define LOGE(fmt, args...) \
            printf("ws2812 [E]: [%s():%d] " fmt "\n", __func__, __LINE__, ## args)
    #endif
#else
    #define LOG(fmt, args...)
    #define LOGW(fmt, args...)
    #define LOGE(fmt, args...)
#endif

#endif /* _LOG_H_ */