#include <FreeRTOS.h>
#include <timers.h>
#include "config.h"

static TimerHandle_t timer;
static uint8_t       mask;

static void on_blink_timer(TimerHandle_t timer)
{
    static uint8_t counter;

    if (++counter & mask) {
        GPIO.OUT_SET = BIT(LED_PIN);
    } else {
        GPIO.OUT_CLEAR = BIT(LED_PIN);
    }
}

void blink_stop(void)
{
    if (timer) {
        xTimerStop(timer, 0);
    }
    // and turn the led off
    GPIO.OUT_SET = BIT(LED_PIN);
}

void blink_restart(void)
{
    if (timer) {
        xTimerStart(timer, 0);
    }
}

void blink_set_mask(uint8_t new_mask)
{
    mask = new_mask;
}

#define AUTO_RELOAD pdTRUE

void blink_init(void)
{
    gpio_enable(LED_PIN, GPIO_OUTPUT);

    mask = 0x01;
    timer = xTimerCreate("blink", pdMS_TO_TICKS(100), AUTO_RELOAD, NULL, on_blink_timer);
    if (timer) {
        xTimerStart(timer, 0);
    }
}
