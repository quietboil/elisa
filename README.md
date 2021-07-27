# ESP8266 Light Switch App

**elisa** is a firmware for the smart :smirk: light switch that controls outside lights in my house.

## Motivation

I wanted a light switch that would turn lights on when it is dark and off when they are not needed. The commercial "smart" light switches in my opinion are ludicrously expensive and building my own one is more fun anyway :smiley:

## Features

The list is very short for the moment:
- It turns lights on and off :smile:
- It keeps the system clock synced with NTP time
- It can be updated over the air
- It blinks the board LED :grin: while the lights are off (so I can see that it has booted and is working)
- It offers a simple [SPA](https://en.wikipedia.org/wiki/Single-page_application) to manually control the lights and change the light switch settings.  

## Installation

**elisa** is implemented using [ESP8266 RTOS SDK](https://github.com/espressif/ESP8266_RTOS_SDK) and the latter has to be installed to compile the former.

Clone the repository:
```sh
$ git clone https://github.com/quietboil/elisa
```

Configure the project:
```sh
make menuconfig
```

The project specific setting can be found under the `Device config`.

Build the firmware:
```sh
make
```

And flash the device:
```sh
make flash
```
> **Note** that initial flashing must be done via the hardware flasher. Subsequent updates can be done over the air.

## OTA Updates

The following steps represent a possible workflow of OTA firmware update:

1) Build the firmware and start the webserver:
```sh
make
python -m http.server
```
2) Send the firmware update request to the device:
```sh
curl http://device.host.name/fw -d 'http://build.machine.name:port/build/elisa.bin'
```
> **Note** that the POST-ed OTA update request is the URL that the device will use to fetch the new firmware. The device will **only** accept requests (firmware URLs) when the HTTP server that hosts new firmware image is on the same subnetwork as the device.
