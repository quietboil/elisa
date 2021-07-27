#include <config.h>
#include <nvs.h>
#include <time.h>
#include <stdio.h>

typedef union {
    struct {
        uint32_t evening_is_active    :  1;
        int32_t  evening_off_hour     :  4; // relative to next day's 00:00
        uint32_t evening_off_min      :  6; //
        uint32_t morning_is_active    :  1;
        uint32_t morning_on_hour      :  4; // since 00:00
        uint32_t morning_on_min       :  6; //
        uint32_t morning_min_duration :  4; // in min
        uint32_t daylight_sun_alt     :  6; // Sun's altitude below horizon in 0.1 degree increments
    };
    uint32_t nvs_data;
} conf_t;

_Static_assert(sizeof(conf_t) == sizeof(uint32_t), "configuration must be packed into 32 bits");

static volatile conf_t config;

static esp_err_t config_save()
{
    nvs_handle nvs;
    esp_err_t res = nvs_open("elisa", NVS_READWRITE, &nvs);
    if ( res == ESP_OK ) {
        res = nvs_set_u32(nvs, "config", config.nvs_data);
        if ( res == ESP_OK ) {
            res = nvs_commit(nvs);
        }
        nvs_close(nvs);
    }
    return res;
}

esp_err_t config_load()
{
    nvs_handle nvs;
    esp_err_t res = nvs_open("elisa", NVS_READONLY, &nvs);
    if ( res == ESP_OK ) {
        uint32_t saved_conf;
        res = nvs_get_u32(nvs, "config", &saved_conf);
        if ( res == ESP_OK ) {
            config.nvs_data = saved_conf;
        }
        nvs_close(nvs);
    } else if ( res == ESP_ERR_NVS_NOT_FOUND ) {
        // create a default configuration instead
        config.evening_is_active    =  1;
        config.evening_off_hour     = -1; // 11pm
        config.evening_off_min      = 45;
        config.morning_is_active    =  1;
        config.morning_on_hour      =  6;
        config.morning_on_min       = 45;
        config.morning_min_duration =  5;
        config.daylight_sun_alt     = 60; // -6.0 degrees, i.e. it's a civil twilight
        res = ESP_OK;
    }
    return res;
}

double config_get_daylight_sun_altitude()
{
    return -config.daylight_sun_alt / 10.0;
}

bool config_is_morning_lights_enabled()
{
    return config.morning_is_active != 0;
}

time_t config_get_morning_on_time(struct tm * local_time)
{
    local_time->tm_hour = config.morning_on_hour;
    local_time->tm_min  = config.morning_on_min;
    local_time->tm_sec  = 0;
    return mktime(local_time);
}

time_t config_get_morning_min_duration()
{
    return config.morning_min_duration * 60;
}

bool config_is_evening_lights_enabled()
{
    return config.evening_is_active != 0;
}

time_t config_get_evening_off_time(struct tm * local_time)
{
    local_time->tm_hour = 24 + config.evening_off_hour;
    local_time->tm_min  = config.evening_off_min;
    local_time->tm_sec  = 0;
    return mktime(local_time);
}

esp_err_t config_serialize(char * buf, size_t * size)
{
    int len = snprintf(buf, *size,
        "{\"morning\":{\"enabled\":%s,\"on_time\":\"%u:%02u\",\"min_duration\":%u}"
        ",\"evening\":{\"enabled\":%s,\"off_time\":\"%u:%02u\"}"
        ",\"daylight_alt\":%u.%u}",
        config.morning_is_active ? "true" : "false",
        config.morning_on_hour,
        config.morning_on_min,
        config.morning_min_duration,
        config.evening_is_active ? "true" : "false",
        config.evening_off_hour < 0 ? config.evening_off_hour + 24 : config.evening_off_hour,
        config.evening_off_min,
        config.daylight_sun_alt / 10,
        config.daylight_sun_alt % 10
    );
    if ( len < 0 || *size <= len ) {
        return ESP_FAIL;
    }
    *size = len;
    return ESP_OK;
}

typedef struct {
    conf_t   new_conf;
    uint32_t num_errs;
} temp_conf_t;

esp_err_t config_patch(const char * json, uint16_t len)
{
    jssp_t parser;
    jssp_init(&parser, config_path_next_state, config_name_next_state);

    temp_conf_t temp_conf;
    temp_conf.new_conf.nvs_data = config.nvs_data;
    temp_conf.num_errs = 0;

    uint8_t res = jssp_start(&parser, &temp_conf, json, len);
    if ( res != JSON_END || temp_conf.num_errs > 0 ) {
        return ESP_FAIL;
    }
    config.nvs_data = temp_conf.new_conf.nvs_data;
    return config_save();
}

void config_set_morning_enabled(const char * data, uint16_t data_length, uint8_t data_type, const jssp_context_t * context)
{
    temp_conf_t * temp_conf = context->result;
    if ( data_type == JSON_TRUE ) temp_conf->new_conf.morning_is_active = 1;
    else if ( data_type == JSON_FALSE ) temp_conf->new_conf.morning_is_active = 0;
    else ++temp_conf->num_errs;
}

void config_set_morning_on_time(const char * data, uint16_t data_length, uint8_t data_type, const jssp_context_t * context)
{
    temp_conf_t * temp_conf = context->result;
    uint32_t hours, minutes;
    if ( data_type == JSON_STRING && sscanf(data, "%u:%u", &hours, &minutes) == 2 && hours < 16 && minutes < 60 ) {
        temp_conf->new_conf.morning_on_hour = hours;
        temp_conf->new_conf.morning_on_min = minutes;
    } else {
        ++temp_conf->num_errs;
    }
}

void config_set_morning_min_duration(const char * data, uint16_t data_length, uint8_t data_type, const jssp_context_t * context)
{
    temp_conf_t * temp_conf = context->result;
    uint32_t minutes;
    if ( data_type == JSON_INTEGER && sscanf(data, "%u", &minutes) == 1 && minutes < 16 ) {
        temp_conf->new_conf.morning_min_duration = minutes;
    } else {
        ++temp_conf->num_errs;
    }
}

void config_set_evening_enabled(const char * data, uint16_t data_length, uint8_t data_type, const jssp_context_t * context)
{
    temp_conf_t * temp_conf = context->result;
    if ( data_type == JSON_TRUE ) temp_conf->new_conf.evening_is_active = 1;
    else if ( data_type == JSON_FALSE ) temp_conf->new_conf.evening_is_active = 0;
    else ++temp_conf->num_errs;
}

void config_set_evening_off_time(const char * data, uint16_t data_length, uint8_t data_type, const jssp_context_t * context)
{
    temp_conf_t * temp_conf = context->result;
    uint32_t hours, minutes;
    if ( data_type == JSON_STRING && sscanf(data, "%u:%u", &hours, &minutes) == 2 && (hours <= 7 || hours >= 16) && minutes < 60 ) {
        temp_conf->new_conf.evening_off_hour = ( hours >= 16 ? hours - 24 : hours );
        temp_conf->new_conf.evening_off_min = minutes;
    } else {
        ++temp_conf->num_errs;
    }
}

static bool is_daylight_sun_alt_valid(uint32_t integer, uint32_t fraction)
{
    return ( integer <= 5 && fraction <= 9 ) || ( integer == 6 && fraction <= 3 );
}

void config_set_daylight_altitude(const char * data, uint16_t data_length, uint8_t data_type, const jssp_context_t * context)
{
    temp_conf_t * temp_conf = context->result;
    uint32_t integer, fraction;
    if ( data_type == JSON_INTEGER && sscanf(data, "%u", &integer) == 1 && integer <= 6 ) {
        temp_conf->new_conf.daylight_sun_alt = integer * 10;
    } else if ( data_type == JSON_DECIMAL && sscanf(data, "%u.%u", &integer, &fraction) == 2 && is_daylight_sun_alt_valid(integer, fraction) ) {
        temp_conf->new_conf.daylight_sun_alt = integer * 10 + fraction;
    } else {
        ++temp_conf->num_errs;
    }
}
