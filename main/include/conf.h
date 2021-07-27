#ifndef __CONF_H
#define __CONF_H

#include <esp_err.h>
#include <time.h>
#include <stdbool.h>

esp_err_t config_load();
esp_err_t config_serialize(char * buf, size_t * size);
esp_err_t config_patch(const char * json, uint16_t len);

double config_get_daylight_sun_altitude();
time_t config_get_morning_on_time(struct tm * local_time);
bool config_is_morning_lights_enabled();
time_t config_get_morning_min_duration();
bool config_is_evening_lights_enabled();
time_t config_get_evening_off_time(struct tm * local_time);

#endif