#include <conf.h>
#include <gpio.h>
#include <sunsiret.h>
#include <esp_log.h>
#include <esp_system.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/timers.h>
#include <stdlib.h>
#include <string.h>

#define TAG "ctrl"

static const char * lights_ctrl_state = "Not running";
static time_t time_of_next_event = 0;

static void set_status(const char * description, time_t until)
{
    lights_ctrl_state = description;
    time_of_next_event = until;
}

size_t get_lights_ctlr_status(char * buf, size_t buf_len)
{
    size_t remaining = buf_len;
    struct tm tm;
    if ( buf_len >= 22 ) {
        time_t now;
        time(&now);
        size_t time_len = strftime(buf, remaining, "%Y-%m-%d %X: ", localtime_r(&now, &tm));
        remaining -= time_len;
        buf += time_len;
    }

    const char * src = lights_ctrl_state;
    while ( *src != '\0' && remaining > 1 ) {
        *buf++ = *src++;
        --remaining;
    }

    if ( time_of_next_event != 0 && remaining >= 27 ) {
        src = " until ";
        while ( *src != '\0' ) {
            *buf++ = *src++;
        }
        remaining -= 7;

        size_t time_len = strftime(buf, remaining, "%Y-%m-%d %X", localtime_r(&time_of_next_event, &tm));
        remaining -= time_len;
        buf += time_len;
    }
    *buf = '\0';
    return (buf_len - remaining);
}

static double latitude, longitude;

static void local_daylight(
    time_t              now,
    struct tm const *   local_time,
    time_t *            daylight_begins,
    time_t *            daylight_ends
) {
    double hour_daylight_begins, hour_daylight_ends;
    __sunriset__(
        local_time->tm_year + 1900, local_time->tm_mon + 1, local_time->tm_mday,
        latitude, longitude, config_get_daylight_sun_altitude(), 0,
        &hour_daylight_begins, &hour_daylight_ends
    );

    struct tm utc;
    gmtime_r(&now, &utc);

    time_t midnight = now - utc.tm_hour * 3600 - utc.tm_min * 60 - utc.tm_sec;
    // Depending on the time in the local_time and the time zone, the UTC "midnight"
    // might be yesterday's or tomorrow's midnight. Need to correct for that
    if ( utc.tm_yday > local_time->tm_yday || utc.tm_year > local_time->tm_year) {
        midnight -= 86400;
    } else if ( utc.tm_yday < local_time->tm_yday || utc.tm_year < local_time->tm_year) {
        midnight += 86400;
    }
    *daylight_begins = midnight + (time_t)(hour_daylight_begins * 3600);
    *daylight_ends   = midnight + (time_t)(hour_daylight_ends   * 3600);
}

#define SEC_TO_TICKS( xTimeInSec ) ( ( TickType_t ) ( xTimeInSec ) * ( TickType_t ) configTICK_RATE_HZ )

/**
 * Suspends the task until the specified moment in the future.
 *
 * @param until time when the task should be awoken
 * @param description current status
 * @return current time
 */
static time_t task_delay_until(time_t until, const char * description)
{
    time_t now;
    time(&now);

    set_status(description, until);
    TickType_t ticks_now = xTaskGetTickCount();
    while ( now < until ) {
        if ( are_lights_on() ) {
            vTaskDelayUntil( &ticks_now, pdMS_TO_TICKS(1000) );
        } else {
            led_on();
            vTaskDelayUntil( &ticks_now, pdMS_TO_TICKS(200) );
            led_off();
            vTaskDelayUntil( &ticks_now, pdMS_TO_TICKS(800) );
        }
        ++now;
    }
    
    time(&now);
    return now;
}

static void lights_ctrl_task(void * arg)
{
    ESP_LOGI(TAG, "Lights control task started");

    latitude  = strtod(CONFIG_LATITUDE,  NULL);
    longitude = strtod(CONFIG_LONGITUDE, NULL);

    for (;;) {
        time_t now;
        time(&now);

        struct tm local_time;
        localtime_r(&now, &local_time);

        time_t daylight_begins, daylight_ends;
        local_daylight(
            now, &local_time,
            &daylight_begins, &daylight_ends
        );

        bool is_weekday = (1 <= local_time.tm_wday && local_time.tm_wday <= 5);
        if ( is_weekday && now < daylight_begins && config_is_morning_lights_enabled() ) {
            time_t morning_on_time = config_get_morning_on_time(&local_time);
            if ( now < morning_on_time ) {
                now = task_delay_until(morning_on_time, "Awaiting morning");
            }
            if ( (daylight_begins - now) >= config_get_morning_min_duration() ) {
                lights_on();
                now = task_delay_until(daylight_begins, "Lights are on. Awaiting daylight");
            } else {
                now = task_delay_until(daylight_begins, "Awaiting daylight");
            }
            lights_off();
        }

        if ( now < daylight_ends ) {
            now = task_delay_until(daylight_ends, "Awaiting evening");
        }

        int yday = local_time.tm_yday;
        time_t evening_off_time = config_get_evening_off_time(&local_time);
        if ( now < evening_off_time ) {
            if ( config_is_evening_lights_enabled() ) {
                lights_on();
                now = task_delay_until(evening_off_time, "Lights are on. Awaiting night");
            } else {
                now = task_delay_until(evening_off_time, "Awaiting night");
            }
            lights_off();
        }

        if ( yday == local_time.tm_yday || local_time.tm_hour < 2 ) {
            if ( yday == local_time.tm_yday ) {
                local_time.tm_mday++;
            }
            local_time.tm_hour = 2;
            local_time.tm_min  = 0;
            local_time.tm_sec  = 2;
            time_t _2am = mktime(&local_time);
            if ( _2am > now ) {
                task_delay_until(_2am, "Awaiting 2am");
            }
        }
    }
}

/**
 * Handle of the task that controls lights.
 */
static TaskHandle_t lights_ctrl_task_handle = NULL;

void start_lights_ctrl_task_if_not_running()
{
    if ( lights_ctrl_task_handle == NULL ) {
        xTaskCreate(lights_ctrl_task, "lights_ctrl_task", 1024, NULL, 5, &lights_ctrl_task_handle);
    }
}

int get_lights_ctrl_task_high_watermark()
{
    return ( lights_ctrl_task_handle == NULL ? -1 : uxTaskGetStackHighWaterMark(lights_ctrl_task_handle) );
}