#ifndef __CONFIG_H
#define __CONFIG_H

#include <jssp.h>

uint32_t config_name_next_state(uint32_t current_state, char next_char);
uint32_t config_path_next_state(uint32_t current_state, uint8_t next_elem, data_cb_t * action_cb_ptr);

void config_set_morning_enabled(const char * data, uint16_t data_length, uint8_t data_type, const jssp_context_t * context);
void config_set_morning_on_time(const char * data, uint16_t data_length, uint8_t data_type, const jssp_context_t * context);
void config_set_morning_min_duration(const char * data, uint16_t data_length, uint8_t data_type, const jssp_context_t * context);
void config_set_evening_enabled(const char * data, uint16_t data_length, uint8_t data_type, const jssp_context_t * context);
void config_set_evening_off_time(const char * data, uint16_t data_length, uint8_t data_type, const jssp_context_t * context);
void config_set_daylight_altitude(const char * data, uint16_t data_length, uint8_t data_type, const jssp_context_t * context);

#endif
