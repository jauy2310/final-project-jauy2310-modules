#include "led.h"

int add_led(int red, int green, int blue, struct led_node **head) {
    // function setup
    struct led_node *curr;

    // allocate memory for a new node
    struct led_node *new = kmalloc(sizeof(struct led_node), GFP_KERNEL);

    // assign values to new node
    new->r = red;
    new->g = green;
    new->b = blue;
    new->next = NULL;

    // check if the list is empty
    curr = *head;
    if (*head == NULL) {
        *head = new;
        return 1;
    }

    // list is not empty; traverse to end and add new node
    while (curr->next != NULL) {
        curr = curr->next;
    }
    curr->next = new;

    // return the size of the linked list
    return size_led(*head);
}

void remove_led(struct led_node **head) {
    // function setup
    struct led_node *curr = *head;
    // check if list is empty; return if it is
    if (*head == NULL) {
        return;
    }

    // check if list has one node; free, remove, and return the only node
    if ((*head)->next == NULL) {
        kfree(*head);
        *head = NULL;
        return;
    }

    // iterate through the list to get to the second to last node
    while (curr->next->next != NULL) {
        curr = curr->next;
    }

    // free the curr->next (last) node and update to NULL
    kfree(curr->next);
    curr->next = NULL;
}

size_t size_led(struct led_node *head) {
    // function setup
    size_t nodes = 0;
    struct led_node *curr;

    // check if list is empty
    if (head == NULL) {
        return 0;
    }

    // count nodes in list
    curr = head;
    while (curr != NULL) {
        nodes++;
        curr = curr->next;
    }

    // return list size
    return nodes;
}

unsigned int colorof_led(struct led_node *led) {
    // check if NULL
    if (led == NULL) {
        return 0;
    }

    // bitwise operate on the LEDs to get an integer value
    return (led->r << 16) | (led->g << 8) | (led->b);
}

void print_led(struct led_node *head) {
    // function setup
    struct led_node *curr;
    size_t index = 0;

    // print header
    LOG("===== [LEDS] =====");  
    
    // check if list is empty
    if (head == NULL) {
        return;
    }

    // loop and print all LEDs
    curr = head;
    while (curr->next != NULL) {
        LOG("[%zu] 0x%06X", index, colorof_led(curr));
        curr = curr->next;
        index++;
    }
}

void free_led(struct led_node *head) {
    // function setup
    struct led_node *curr;

    // check if NULL; return immediately if no nodes to free
    if (head == NULL) {
        return;
    }

    // iterate through, freeing each node
    curr = head;
    while (curr != NULL) {
        struct led_node *next = curr->next;
        kfree(curr);
        curr = next;
    }
}