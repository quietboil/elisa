#include <lwip/apps/sntp.h>
#include <esp_log.h>
#include <esp_wifi.h>
#include <try.h>
#include <ctrl.h>
#include <httpd.h>
#include <stdbool.h>

#define TAG "wifi"

/**
 * IP assigned to the device by the AP.
 */
static ip4_addr_t device_ip = { .addr = IPADDR_NONE };

/**
 * Network mask that identifies the subnetwork where
 * the device is.
 */
static ip4_addr_t device_netmask;

bool is_peer_local(ip4_addr_t peer_ip)
{
    return ip4_addr_netcmp(&peer_ip, &device_ip, &device_netmask);
}

static void on_sntp_time_synced(struct timeval * tv)
{
    ESP_LOGI(TAG, "Sync-ed time with NTP.");
    start_lights_ctrl_task_if_not_running();
}

/**
 * Starts services that need a network to be already active.
 */
static void start_network_services()
{
    ESP_LOGI(TAG, "Using %s to sync clock", CONFIG_SNTP_SERVER);
    sntp_setservername(0, CONFIG_SNTP_SERVER);
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_set_time_sync_notification_cb(on_sntp_time_synced);
    sntp_init();

    ESP_ERROR_CHECK( start_websrv() );
}

static void on_wifi_start(void * arg, esp_event_base_t event_base, int32_t event_id, void * event_data)
{
    ESP_ERROR_CHECK( esp_wifi_connect() );
}

static void on_wifi_disconnected(void * arg, esp_event_base_t event_base, int32_t event_id, void * event_data)
{
    wifi_event_sta_disconnected_t * event = (wifi_event_sta_disconnected_t *) event_data;
    ESP_LOGI(TAG, "Disconnected from '%.*s' (reason %d)", event->ssid_len, event->ssid, event->reason);

    if ( event->reason == WIFI_REASON_BASIC_RATE_NOT_SUPPORT ) {
        esp_wifi_set_protocol(ESP_IF_WIFI_STA, WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G | WIFI_PROTOCOL_11N);
    }
    if ( event->reason != WIFI_REASON_ASSOC_LEAVE ) {
        ESP_LOGI(TAG, "Reconnecting...");
        ESP_ERROR_CHECK( esp_wifi_connect() );
    }
}

static void on_got_ip(void * arg, esp_event_base_t event_base, int32_t event_id, void * event_data)
{
    ip_event_got_ip_t * event = (ip_event_got_ip_t *) event_data;

    bool first_ip = (device_ip.addr == IPADDR_NONE);
    ip4_addr_copy(device_ip, event->ip_info.ip);
    ip4_addr_copy(device_netmask, event->ip_info.netmask);
    ESP_LOGI(TAG, "Device IP=%d.%d.%d.%d", ip4_addr1_val(device_ip), ip4_addr2_val(device_ip), ip4_addr3_val(device_ip), ip4_addr4_val(device_ip));

    if ( first_ip ) {
        start_network_services();
    }
}

esp_err_t wifi_init()
{
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    TRY( esp_wifi_init(&cfg) );

    TRY( esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_START,        &on_wifi_start,        NULL) );
    TRY( esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &on_wifi_disconnected, NULL) );
    TRY( esp_event_handler_register(IP_EVENT,   IP_EVENT_STA_GOT_IP,         &on_got_ip,            NULL) );

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = CONFIG_WIFI_SSID,
            .password = CONFIG_WIFI_PASSWORD,
            .threshold = {
                .authmode = WIFI_AUTH_WPA2_PSK
            }
        }
    };

    TRY( esp_wifi_set_mode(WIFI_MODE_STA) );
    TRY( esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
    TRY( esp_wifi_set_protocol(ESP_IF_WIFI_STA, WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G | WIFI_PROTOCOL_11N) );

    TRY( esp_wifi_start() );

    return ESP_OK;
}