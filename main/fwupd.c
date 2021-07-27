#include <esp_log.h>
#include <esp_https_ota.h>
#include <esp_system.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#define TAG "firmware_update"

static void firmware_update_task(void * pvParameter)
{
    char * fw_update_url = (char *) pvParameter;
    ESP_LOGI(TAG, "Starting firmware update from %s", fw_update_url);
    esp_http_client_config_t http_client_config = {
        .url = fw_update_url,
    };
    if ( esp_https_ota(&http_client_config) != ESP_OK ) {
        ESP_LOGE(TAG, "OTA firmware update has failed");
        free(pvParameter);        
    } else {
        ESP_LOGI(TAG, "Restarting now");
        esp_restart();
    }
    vTaskDelete(NULL);
}

void start_firmware_update(char * fw_url)
{
    xTaskCreate(firmware_update_task, "firmware_update_task", 8192, fw_url, 7, NULL);
}