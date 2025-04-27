#ifndef _LED_H_
#define _LED_H_

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

// kernel includes
#include <linux/slab.h>

// local includes
#include "log.h"

/**************************************************************************************
 * DATA STRUCTS
 **************************************************************************************/
typedef struct led_node {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} led_node;

/**************************************************************************************
 * MACROS/DEFINES
 **************************************************************************************/
#define MAX_LEDS 100

/**************************************************************************************
 * FUNCTION PROTOTYPES
 **************************************************************************************/

/**
 * add_led()
 * 
 * Adds an LED to the list
 */
int add_led(int red, int green, int blue, struct led_node **head);

/**
 * remove_led()
 * 
 * Removes an LED from the list
 */
void remove_led(struct led_node **head);

/**
 * size_led()
 * 
 * Returns the size of the list
 */
size_t size_led(struct led_node *head);

/**
 * colorof_led()
 * 
 * Returns the hex code of an LED (0x00RRGGBB)
 */
unsigned int colorof_led(struct led_node *led);

/**
 * print_led()
 * 
 * Prints the LEDs in the list
 */
void print_led(struct led_node *head);

/**
 * free_led()
 * 
 * Frees all the nodes in the list
 */
void free_led(struct led_node *head);


#endif /* _LED_H_ */