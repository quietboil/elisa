#include <FreeRTOS.h>
#include <task.h>
#include <espressif/esp_system.h>
#include <espressif/esp_wifi.h>
#include <espressif/esp_sta.h>
#include <esplibs/libmain.h>
#include <esp/gpio.h>
#include <lwip/apps/sntp.h>
#include <sys/time.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sntp_time.h>
#include <sunsiret.h>
#include "config.h"
#include "blink.h"
#include "main.h"
#include "stat.h"

#ifdef DEBUG
#define TIME_FMT "%04d-%02d-%02d %02d:%02d:%02d"
#define TM2ARGS(tm) ((tm)->tm_year + 1900),((tm)->tm_mon + 1),(tm)->tm_mday,(tm)->tm_hour,(tm)->tm_min,(tm)->tm_sec
#define LOG(fmt,...) printf("main(%u)> " fmt,(uint32_t)uxTaskGetStackHighWaterMark(NULL),## __VA_ARGS__)
#define LOG_TM(msg,tm) LOG(msg TIME_FMT "\n",TM2ARGS(tm))
#define LOG_TIME(msg,t) do { \
    struct tm _tm; \
    localtime_r(&(t),&_tm); \
    LOG_TM(msg,&_tm); \
} while (0)
#else
#define LOG(fmt,...)
#define LOG_TM(msg,tm)
#define LOG_TIME(msg,t)
#endif

#define SEC_TO_TICKS(sec)   ((TickType_t)(sec * configTICK_RATE_HZ))
#define USEC_TO_TICKS(usec) ((TickType_t)(usec * configTICK_RATE_HZ / 1000000))

static void local_civil_twilight(time_t time_now, struct tm const * now, time_t * morning_off, time_t * evening_on)
{
    double sunrise_hour, sunset_hour;
    civil_twilight(
        now->tm_year + 1900, now->tm_mon + 1, now->tm_mday,
        LATITUDE, LONGITUDE,
        &sunrise_hour, &sunset_hour
    );
    struct tm utc;
    gmtime_r(&time_now, &utc);
    time_t midnight = time_now - utc.tm_hour * 3600 - utc.tm_min * 60 - utc.tm_sec;
    // Depending on the local time and the timezone, the UT "midnight" might be yesterday's
    // or tomorrow's midnight. Need to correct for that
    if (utc.tm_yday > now->tm_yday || utc.tm_year > now->tm_year) {
        midnight -= 86400;
    } else if (utc.tm_yday < now->tm_yday || utc.tm_year < now->tm_year) {
        midnight += 86400;
    }

    *morning_off = midnight + (time_t)(sunrise_hour * 3600);
    *evening_on  = midnight + (time_t)(sunset_hour  * 3600);
}

static void task_delay_until(TickType_t * last_wake_time, struct timeval * now, time_t * end)
{
    vTaskDelayUntil(last_wake_time, USEC_TO_TICKS(1000000 - now->tv_usec));
    if (*end > now->tv_sec) {
        vTaskDelayUntil(last_wake_time, SEC_TO_TICKS(*end - now->tv_sec));
    }
}

static void lights_on_until(TickType_t * last_wake_time, struct timeval * now, time_t * end)
{
    blink_stop();
    gpio_write(RELAY_PIN, 0);
    task_delay_until(last_wake_time, now, end);
    gpio_write(RELAY_PIN, 1);
    blink_restart();
}

static void main_task(void * arg)
{
    main_task_state_t * st = arg;
    st->period = "Initializing";
    LOG("Started\n");

    // wait until WiFi gets connected
    while (sdk_wifi_station_get_connect_status() != STATION_GOT_IP) {
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    LOG("Connected\n");
    blink_set_mask(0x3);

    LOCK_TCPIP_CORE();
    sntp_init();
    UNLOCK_TCPIP_CORE();

    LOG("Waiting for NTP time\n");
    LOG("TZ=%s\n", getenv("TZ"));
    blink_set_mask(0x7);

    // wait until NTP sets the time
    do {
        vTaskDelay(pdMS_TO_TICKS(100));
    } while (!sntp_time_is_set());

    LOG("Entering day loop\n");
    blink_set_mask(0xf);

    // day long loop that turns lights on and off
    TickType_t last_wake_time = xTaskGetTickCount();
    for (;;) {
        gettimeofday(&st->checkpoint, NULL);
        struct tm tm;
        localtime_r(&st->checkpoint.tv_sec, &tm);
        LOG_TM("Local time: ", &tm);
        local_civil_twilight(st->checkpoint.tv_sec, &tm, &st->morning_off, &st->evening_on);
        LOG_TIME("Morning twilight: ", st->morning_off);
        LOG_TIME("Evening twilight: ", st->evening_on);

        bool is_weekday = (1 <= tm.tm_wday && tm.tm_wday <= 5);

        tm.tm_hour = MORNING_LIGHTS_ON_HOURS;
        tm.tm_min = MORNING_LIGHTS_ON_MIN;
        tm.tm_sec = 0;
        st->morning_on = mktime(&tm);

        #if (NIGHT_LIGHTS_OFF_HOURS < MORNING_LIGHTS_ON_HOURS)
        tm.tm_mday += 1;
        #endif
        tm.tm_hour = NIGHT_LIGHTS_OFF_HOURS;
        tm.tm_min = NIGHT_LIGHTS_OFF_MIN;
        tm.tm_sec = 0;
        st->night_off = mktime(&tm);

        if (is_weekday) {
            LOG("Weekday\n");

            if (st->checkpoint.tv_sec < st->morning_on) {
                LOG("Waiting until morning\n");
                st->period = "Awaiting Morning";
                task_delay_until(&last_wake_time, &st->checkpoint, &st->morning_on);
                gettimeofday(&st->checkpoint, NULL);
                LOG_TIME("Local time: ", st->checkpoint.tv_sec);
            }
            if (st->checkpoint.tv_sec < st->morning_off) {
                if (st->morning_off - st->checkpoint.tv_sec < MIN_MORNING_LIGHTS * 60) {
                    LOG("Skipping %d min %d sec of morning lights\n",
                            (uint32_t)(st->morning_off - st->checkpoint.tv_sec) / 60,
                            (uint32_t)(st->morning_off - st->checkpoint.tv_sec) % 60);
                } else {
                    LOG_TIME("Lights on until ", st->morning_off);
                    st->period = "Morning Lights";
                    lights_on_until(&last_wake_time, &st->checkpoint, &st->morning_off);
                    gettimeofday(&st->checkpoint, NULL);
                    LOG_TIME("Lights off at ", st->checkpoint.tv_sec);
                }
            }
        }

        if (st->checkpoint.tv_sec < st->evening_on) {
            LOG("Waiting until evening\n");
            st->period = "Awaiting Evening";
            task_delay_until(&last_wake_time, &st->checkpoint, &st->evening_on);
            gettimeofday(&st->checkpoint, NULL);
            LOG_TIME("Local time: ", st->checkpoint.tv_sec);
        }

        // night lights
        LOG_TIME("Lights on until ", st->night_off);
        st->period = "Night Lights";
        lights_on_until(&last_wake_time, &st->checkpoint, &st->night_off);
        LOG("Lights off\n");
    }
}

#define TASK_STACK_SIZE 192

void main_task_init()
{
    ip_addr_t dns[] = { SNTP_SERVERS };
    for (int i = 0; i < SNTP_MAX_SERVERS && i < sizeof(dns) / sizeof(ip_addr_t); i++) {
        sntp_setserver(i, &dns[i]);
    }

    sntp_time_init();

    gpio_enable(RELAY_PIN, GPIO_OUTPUT);
    gpio_write(RELAY_PIN, 1);

    static main_task_state_t state;
    stat_vfs_init(&state);

    xTaskCreate(main_task, "main", TASK_STACK_SIZE, &state, tskIDLE_PRIORITY, &state.task_handle);
}