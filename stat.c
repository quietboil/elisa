#include "main.h"
#include <time.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <lwip/apps/tftp_server.h>

#define TIME_FMT "%04d-%02d-%02d %02d:%02d:%02d"
#define TM2ARGS(tm) ((tm)->tm_year + 1900),((tm)->tm_mon + 1),(tm)->tm_mday,(tm)->tm_hour,(tm)->tm_min,(tm)->tm_sec

static main_task_state_t * main_task_state;

void stat_vfs_init(main_task_state_t * state)
{
    main_task_state = state;
}

static void * tftp_open(const char * fname, const char * mode, uint8_t write)
{
    if (write || strlen(fname) != 4 || strncmp(fname, "stat", 4) != 0)  {
        return NULL;
    }
    return main_task_state; // return NULL until it is set
}

static void tftp_close(void * handle)
{
}

static int tftp_read(void * handle, void * buf, int bytes)
{
    struct timeval time_now;
    gettimeofday(&time_now, NULL);
    struct tm tm;
    localtime_r(&time_now.tv_sec, &tm);
    bool is_weekday = (1 <= tm.tm_wday && tm.tm_wday <= 5);
    int len = 0;
    len += sprintf((char*)buf + len, "Local Time: " TIME_FMT "\n", TM2ARGS(&tm));
    if (is_weekday) {
        localtime_r(&main_task_state->morning_on, &tm);
        len += sprintf((char*)buf + len, "Morning on: " TIME_FMT "\n", TM2ARGS(&tm));
        localtime_r(&main_task_state->morning_off, &tm);
        len += sprintf((char*)buf + len, "Morning off: " TIME_FMT "\n", TM2ARGS(&tm));
    }
    localtime_r(&main_task_state->evening_on, &tm);
    len += sprintf((char*)buf + len, "Evening on: " TIME_FMT "\n", TM2ARGS(&tm));
    if (main_task_state->night_off) {
        localtime_r(&main_task_state->night_off, &tm);
        len += sprintf((char*)buf + len, "Night off: " TIME_FMT "\n", TM2ARGS(&tm));
    }
    len += sprintf((char*)buf + len, "<< %s >>\n", main_task_state->period);
    localtime_r(&main_task_state->checkpoint.tv_sec, &tm);
    len += sprintf((char*)buf + len, "Last checkpoint: " TIME_FMT "\n", TM2ARGS(&tm));
    len += sprintf((char*)buf + len, "Watermark: %u\n", (uint32_t)uxTaskGetStackHighWaterMark(main_task_state->task_handle));
    return len;
}

static int tftp_write(void * handle, struct pbuf * p)
{
    return -1;
}

struct tftp_context const STAT_VFS = {
    .open  = tftp_open,
    .close = tftp_close,
    .read  = tftp_read,
    .write = tftp_write
};
