#include <driver/gpio.h>
#include <stdbool.h>

#define RELAY_PIN CONFIG_RELAY_PIN
#define LED_PIN 2

static bool lights_are_on;

bool are_lights_on()
{
    return lights_are_on;
}

void lights_on()
{
    gpio_set_level(RELAY_PIN, 0);
    lights_are_on = true;
}

void lights_off()
{
    gpio_set_level(RELAY_PIN, 1);
    lights_are_on = false;
}

void led_on()
{
    gpio_set_level(LED_PIN, 0);
}

void led_off()
{
    gpio_set_level(LED_PIN, 1);
}

esp_err_t lights_gpio_init()
{
    gpio_config_t io_conf = {
        .pin_bit_mask = BIT(RELAY_PIN) | BIT(LED_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .intr_type = GPIO_INTR_DISABLE,
        .pull_down_en = 0,
        .pull_up_en = 0,
    };
    esp_err_t err = gpio_config(&io_conf);
    if ( err == ESP_OK ) {
        lights_off();
        led_off();
    }
    return err;
}
