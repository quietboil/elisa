#ifndef __CONFIG_H
#define __CONFIG_H

/** SNTP server(s) */
#ifndef SNTP_SERVERS
#define SNTP_SERVERS IPADDR4_INIT_BYTES(192,168,1,1)
#endif

/** Device location latitude */
#ifndef LATITUDE
#define LATITUDE 38.889465
#endif

/** Device location longitude */
#ifndef LONGITUDE
#define LONGITUDE -77.035246
#endif

/** Device location time zone */
#ifndef TIMEZONE
#define TIMEZONE "EST5EDT"
#endif

/** Pin where LED is attached */
#ifndef LED_PIN
#define LED_PIN 2
#endif

/** Pin where relay is attached */
#ifndef RELAY_PIN
#define RELAY_PIN 5
#endif

/** When to turn the lights on in the morning (hours) */
#ifndef MORNING_LIGHTS_ON_HOURS
#define MORNING_LIGHTS_ON_HOURS 6
#endif

/** When to turn on the lights on in the morning (minutes) */
#ifndef MORNING_LIGHTS_ON_MIN
#define MORNING_LIGHTS_ON_MIN 50
#endif

/** Do not turn the lights on if they wull be on for less then this (minutes) */
#ifndef MIN_MORNING_LIGHTS
#define MIN_MORNING_LIGHTS 15
#endif

/** When to turn the lights off at night (hours) */
#ifndef NIGHT_LIGHTS_OFF_HOURS
#define NIGHT_LIGHTS_OFF_HOURS 0
#endif

/** When to turn on the lights off at night (minutes) */
#ifndef NIGHT_LIGHTS_OFF_MIN
#define NIGHT_LIGHTS_OFF_MIN 10
#endif

#endif