#ifndef __GPIO_H
#define __GPIO_H

#include <esp_err.h>
#include <stdbool.h>

esp_err_t lights_gpio_init();
void lights_on();
void lights_off();
bool are_lights_on();
void led_on();
void led_off();


#endif