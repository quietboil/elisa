#include <sdkconfig.h>
#include <conf.h>
#include <gpio.h>
#include <wifi.h>
#include <try.h>
#include <nvs_flash.h>
#include <esp_netif.h>
#include <esp_event.h>
#include <time.h>

/**
 * Initializes the application.
 */
static esp_err_t app_init()
{
    // init GPIO first to ensure that lights are off ASAP
    TRY( lights_gpio_init() );
    TRY( nvs_flash_init() );
    TRY( config_load() );
    TRY( esp_netif_init() );
    TRY( esp_event_loop_create_default() );
    TRY( wifi_init() );

    return ESP_OK;
}

void app_main()
{
    setenv("TZ", CONFIG_TIMEZONE, 1);
    tzset();

    ESP_ERROR_CHECK( app_init() );
}