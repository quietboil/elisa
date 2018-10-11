#ifndef __MAIN_H
#define __MAIN_H

#include <FreeRTOS.h>
#include <task.h>
#include <time.h>

/**
 * \brief Creates main task
 */
void main_task_init();

/** Internal task state */
typedef struct {
    TaskHandle_t	task_handle;
    const char *    period;       ///< What the task is waiting for
    uint32_t	    duration;     ///< Duration of the `period`
    time_t          since;        ///< When the current period started
    time_t          morning_on;   ///< Time the lights will be turned on in the morning
    time_t          morning_off;  ///< Time the lights will be turned off in the morning
    time_t          evening_on;   ///< Time the lights will be turned on in the evening
    time_t          night_off;    ///< Time the lights will be turned off at night
} main_task_state_t;

#endif