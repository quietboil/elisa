#include "config.h"

uint32_t config_name_next_state(uint32_t state, char next_char)
{
	switch (state) {
		case 1: {
			switch (next_char) {
				case 'm': return 256;
				case 'e': return 262;
				case 'o': return 268;
				case 'd': return 295;
			}
			break;
		}
		case 256: {
			switch (next_char) {
				case 'o': return 257;
				case 'i': return 274;
			}
			break;
		}
		case 257: {
			if (next_char == 'r') return 258;
			break;
		}
		case 258: {
			if (next_char == 'n') return 259;
			break;
		}
		case 259: {
			if (next_char == 'i') return 260;
			break;
		}
		case 260: {
			if (next_char == 'n') return 261;
			break;
		}
		case 261: {
			if (next_char == 'g') return 19;
			break;
		}
		case 274: {
			if (next_char == 'n') return 275;
			break;
		}
		case 275: {
			if (next_char == '_') return 276;
			break;
		}
		case 276: {
			if (next_char == 'd') return 277;
			break;
		}
		case 277: {
			if (next_char == 'u') return 278;
			break;
		}
		case 278: {
			if (next_char == 'r') return 279;
			break;
		}
		case 279: {
			if (next_char == 'a') return 280;
			break;
		}
		case 280: {
			if (next_char == 't') return 281;
			break;
		}
		case 281: {
			if (next_char == 'i') return 282;
			break;
		}
		case 282: {
			if (next_char == 'o') return 283;
			break;
		}
		case 283: {
			if (next_char == 'n') return 22;
			break;
		}
		case 262: {
			switch (next_char) {
				case 'n': return 263;
				case 'v': return 284;
			}
			break;
		}
		case 263: {
			if (next_char == 'a') return 264;
			break;
		}
		case 264: {
			if (next_char == 'b') return 265;
			break;
		}
		case 265: {
			if (next_char == 'l') return 266;
			break;
		}
		case 266: {
			if (next_char == 'e') return 267;
			break;
		}
		case 267: {
			if (next_char == 'd') return 20;
			break;
		}
		case 284: {
			if (next_char == 'e') return 285;
			break;
		}
		case 285: {
			if (next_char == 'n') return 286;
			break;
		}
		case 286: {
			if (next_char == 'i') return 287;
			break;
		}
		case 287: {
			if (next_char == 'n') return 288;
			break;
		}
		case 288: {
			if (next_char == 'g') return 23;
			break;
		}
		case 268: {
			switch (next_char) {
				case 'n': return 269;
				case 'f': return 289;
			}
			break;
		}
		case 269: {
			if (next_char == '_') return 270;
			break;
		}
		case 270: {
			if (next_char == 't') return 271;
			break;
		}
		case 271: {
			if (next_char == 'i') return 272;
			break;
		}
		case 272: {
			if (next_char == 'm') return 273;
			break;
		}
		case 273: {
			if (next_char == 'e') return 21;
			break;
		}
		case 289: {
			if (next_char == 'f') return 290;
			break;
		}
		case 290: {
			if (next_char == '_') return 291;
			break;
		}
		case 291: {
			if (next_char == 't') return 292;
			break;
		}
		case 292: {
			if (next_char == 'i') return 293;
			break;
		}
		case 293: {
			if (next_char == 'm') return 294;
			break;
		}
		case 294: {
			if (next_char == 'e') return 24;
			break;
		}
		case 295: {
			if (next_char == 'a') return 296;
			break;
		}
		case 296: {
			if (next_char == 'y') return 297;
			break;
		}
		case 297: {
			if (next_char == 'l') return 298;
			break;
		}
		case 298: {
			if (next_char == 'i') return 299;
			break;
		}
		case 299: {
			if (next_char == 'g') return 300;
			break;
		}
		case 300: {
			if (next_char == 'h') return 301;
			break;
		}
		case 301: {
			if (next_char == 't') return 302;
			break;
		}
		case 302: {
			if (next_char == '_') return 303;
			break;
		}
		case 303: {
			if (next_char == 'a') return 304;
			break;
		}
		case 304: {
			if (next_char == 'l') return 305;
			break;
		}
		case 305: {
			if (next_char == 't') return 25;
			break;
		}
	}
	return 0;
}

static inline uint32_t object_data(uint8_t json_token, uint32_t current_state, uint32_t next_state, data_cb_t * action_ptr, data_cb_t action_cb) {
	if (json_token == JSON_NUMBER_PART || json_token == JSON_STRING_PART) {
		*action_ptr = action_cb;
		return current_state;
	}
	if (JSON_NULL <= json_token && json_token <= JSON_STRING) {
		*action_ptr = action_cb;
		return next_state;
	}
	return 0;
}

static inline uint32_t array_data(uint8_t json_token, uint32_t current_state, uint32_t next_state, data_cb_t * action_ptr, data_cb_t action_cb) {
	if (JSON_NUMBER_PART <= json_token && json_token <= JSON_STRING) {
		*action_ptr = action_cb;
		return current_state;
	}
	return json_token == JSON_ARRAY_END ? next_state : 0;
}

uint32_t config_path_next_state(uint32_t state, uint8_t next_elem, data_cb_t * action)
{
	switch (state) {
		case 1: {
			if (next_elem == 15) return 2;
			break;
		}
		case 2: {
			switch (next_elem) {
				case 19: return 3;
				case 23: return 8;
				case 25: return 12;
				case 16: return 13;
			}
			break;
		}
		case 3: {
			if (next_elem == 15) return 4;
			break;
		}
		case 4: {
			switch (next_elem) {
				case 20: return 5;
				case 21: return 6;
				case 22: return 7;
				case 16: return 2;
			}
			break;
		}
		case 5: {
			return object_data(next_elem, 5, 4, action, config_set_morning_enabled);
		}
		case 6: {
			return object_data(next_elem, 6, 4, action, config_set_morning_on_time);
		}
		case 7: {
			return object_data(next_elem, 7, 4, action, config_set_morning_min_duration);
		}
		case 8: {
			if (next_elem == 15) return 9;
			break;
		}
		case 9: {
			switch (next_elem) {
				case 20: return 10;
				case 24: return 11;
				case 16: return 2;
			}
			break;
		}
		case 10: {
			return object_data(next_elem, 10, 9, action, config_set_evening_enabled);
		}
		case 11: {
			return object_data(next_elem, 11, 9, action, config_set_evening_off_time);
		}
		case 12: {
			return object_data(next_elem, 12, 2, action, config_set_daylight_altitude);
		}
	}
	return 0;
}
