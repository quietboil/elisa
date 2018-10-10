# ESP8266 Light Switch App

**elisa** is a firmware for the smart :smirk: light switch that controls outside lights in my house.

## Motivation

I wanted a light switch that would turn lights on when it is dark and off when they are not needed. The commercial "smart" light switches in my opinion are ludicrously expensive and building my own one is more fun anyway :smiley:

## Features

The list is very short for the moment:
- It turns lights on and off :smile:
- It keeps the system clock synced with NTP time
- It can be updated over the air (TFTP)
- It blinks the board LED :grin: while the lights are off (so I can see that it has booted and is working)

More functions are planned:
- A web-app to program it - change schedule, turn on and off manually, etc.
- Add a display and show something mildly useful - clock and weather maybe.

## Installation

**elisa** is an [esp-open-rtos](https://github.com/SuperHouse/esp-open-rtos) program and expects the latter to be installed.

Clone the repository recursively:
```sh
$ git clone --recursive https://github.com/quietboil/elisa
```
This will also fetch `esp-open-rtos-components`.

Next you need to install a bootloader.
> **Note** that **elisa** does not use the default for esp-open-rtos rboot bootloader.
Follow steps in the README from the components/bootloader.

Create `local.mk` file and describe your development environment. For example, here's the one I use locally:
```makefile
# Location where esp-open-rtos is installed
ESP_OPEN_RTOS_DIR = $(HOME)/ESP/open-rtos
# The default FLASH_MODE is "qio". If that works, then
# FLASH_MODE is not needed. NodeMCU, some of them anyway,
# do not tolerate qui and need dio.
FLASH_MODE = dio
# The default FLASH_SIZE is 16.
FLASH_SIZE = 32
# The default FLASH_SPEED is 40.
FLASH_SPEED = 80
# The default ESPBAUD which is safe, but most ESP's can
# be rpogrammed faster. ESP-12F that I have can be programmed
# at 921600. That's as fast as CP2102 that I used to program
# it would go. NodeMCU however chokes at that speed even though
# CH340 on that board can go as fast as 2000000.
ESPBAUD = 460800
# The IP of the DEV board for the "ota_update" target.
DEVICE_IP = 192.168.1.251
```
Then in the same `local.mk` configure the application:
```makefile
# SNTP server(s)
SNTP_MAX_SERVERS = 1
SNTP_SERVERS = "IPADDR4_INIT_BYTES(192,168,1,1)"
# LATITUDE and LONGITUDE is where the device is located.
# Sunriset uses it it calculate sunrise and sunset times.
# In case you are wondering - the values below are not real,
# they are here just to provide an example.
LATITUDE  = 38.889465
LONGITUDE = -77.035246
# Value to use to create TZ environemnt variable to make
# localtime return local time.
TIMEZONE  = EST5EDT
# Using the board's LED to blink
LED_PIN   = 2
# Where the relay is attached.
# Note that, like LED, RELAY pin is active low in this
# implementation (becuase that's how I attached it to my board)
RELAY_PIN = 5
# When to turn the lights on in the morning
MORNING_LIGHTS_ON_HOURS = 6
MORNING_LIGHTS_ON_MIN = 50
# Do not turn lights on in the morning if they will be on
# for less then this (in minutes)
MIN_MORNING_LIGHTS = 15
# When to turn lights off at night
NIGHT_LIGHTS_OFF_HOURS = 0
NIGHT_LIGHTS_OFF_MIN = 10
```

Then build and flash the firmware:
```sh
$ make -j4
$ make flash
```
Flashing needs to be done only once. While **elisa** is running it can be updated over the air using
```sh
$ make ota_update
```
