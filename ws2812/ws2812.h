#ifndef _WS2812_H_
#define _WS2812_H_

/**************************************************************************************
 * INCLUDES
 **************************************************************************************/
#ifdef __KERNEL__
#include <linux/types.h>
#else
#include <stddef.h> // size_t
#include <stdint.h> // uintx_t
#include <stdbool.h>
#endif

/**************************************************************************************
 * MACROS/DEFINES
 **************************************************************************************/
#define DEBUG 1

// logging/debugging prints
#undef LOG
#ifdef DEBUG
    #ifdef __KERNEL__
        #define LOG(fmt, args...) \
            printk(KERN_DEBUG "ws2812: [%s():%d] " fmt "\n", __func__, __LINE__, ## args)
    #else
        #define LOG(fmt, args...) \
            printf("ws2812: [%s():%d] " fmt "\n", __func__, __LINE__, ## args)
    #endif
#else
    #define LOG(fmt, args...) /* not debugging: nothing */
#endif

# endif /* _WS2812_H_ */