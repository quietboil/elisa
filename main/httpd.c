#include <conf.h>
#include <wifi.h>
#include <ctrl.h>
#include <gpio.h>
#include <fwupd.h>
#include <try.h>
#include <esp_http_server.h>
#include <esp_log.h>
#include <esp_system.h>
#include <esp_spi_flash.h>
#include <netdb.h>
#include <time.h>
#include <string.h>

#define TAG "httpd"

extern const char app_html_gz[]     asm("_binary_app_html_gz_start");
extern const char app_html_gz_end[] asm("_binary_app_html_gz_end");
extern const char icon_svg_gz[]     asm("_binary_icon_svg_gz_start");
extern const char icon_svg_gz_end[] asm("_binary_icon_svg_gz_end");

/**
 * Reads request payload into the provided buffer.
 *
 * @param req HTTP request
 * @param buf pointer to the data buffer
 * @param buf_lem size of the buffer
 */
static esp_err_t read_request_payload(httpd_req_t * req, char * buf, size_t buf_len)
{
    size_t remaining = (req->content_len < buf_len ? req->content_len : buf_len - 1 );
    while ( remaining > 0 ) {
        int recv_cnt = httpd_req_recv(req, buf, remaining);
        if ( recv_cnt <= 0 ) {
            if ( recv_cnt == HTTPD_SOCK_ERR_TIMEOUT ) {
                continue;
            }
            return ESP_FAIL;
        }
        remaining -= recv_cnt;
        buf += recv_cnt;
    }
    *buf = '\0';
    return ESP_OK;
}

/**
 * Handles GET "/" requests.
 *
 * Returns the html of this app's SPA front-end.
 *
 * @param req HTTP request
 */
static esp_err_t handle_get_root(httpd_req_t * req)
{
    httpd_resp_set_type(req, HTTPD_TYPE_TEXT);
    httpd_resp_set_hdr(req, "Content-Encoding", "gzip");

    return httpd_resp_send(req, app_html_gz, app_html_gz_end - app_html_gz);
}

static httpd_uri_t get_root = {
    .uri = "/",
    .method = HTTP_GET,
    .handler = handle_get_root
};

/**
 * Handles GET "/icon.svg" requests.
 *
 * Returns the site's favicon
 *
 * @param req HTTP request
 */
static esp_err_t handle_get_icon(httpd_req_t * req)
{
    httpd_resp_set_type(req, "image/svg+xml");
    httpd_resp_set_hdr(req, "Content-Encoding", "gzip");

    return httpd_resp_send(req, icon_svg_gz, icon_svg_gz_end - icon_svg_gz);
}

static httpd_uri_t get_icon = {
    .uri = "/icon.svg",
    .method = HTTP_GET,
    .handler = handle_get_icon
};

/**
 * Handles GET "/conf" requests.
 *
 * Returns the app configuration serialized into JSON.
 *
 * @param req HTTP request
 */
static esp_err_t handle_get_conf(httpd_req_t * req)
{
    char json[140];
    size_t len = sizeof(json);
    if ( config_serialize(json, &len) != ESP_OK ) {
        static const char resp[] = "Cannot generate a response";
        httpd_resp_set_status(req, HTTPD_500);
        return httpd_resp_send(req, resp, sizeof(resp) - 1);
    }
    httpd_resp_set_type(req, HTTPD_TYPE_JSON);
    return httpd_resp_send(req, json, len);
}

static httpd_uri_t get_conf = {
    .uri = "/conf",
    .method = HTTP_GET,
    .handler = handle_get_conf
};

/**
 * Handles PATCH "/conf" requests.
 *
 * Applies the app configuration patch.
 *
 * @param req HTTP request
 */
static esp_err_t handle_patch_conf(httpd_req_t * req)
{
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");

    if ( req->content_len == 0 ) {
        httpd_resp_set_status(req, "304 Not Modified");
        static const char resp[] = "Patch was empty";
        return httpd_resp_send(req, resp, sizeof(resp) - 1);
    }

    char json[140];
    if ( req->content_len >= sizeof(json) ) {
        static const char resp[] = "Configuration patch is too long";
        httpd_resp_set_status(req, "413 Payload Too Large");
        return httpd_resp_send(req, resp, sizeof(resp) - 1);
    }

    if ( read_request_payload(req, json, sizeof(json)) != ESP_OK ) {
        static const char resp[] = "Cannot read request payload";
        httpd_resp_set_status(req, HTTPD_500);
        return httpd_resp_send(req, resp, sizeof(resp) - 1);
    }

    if ( config_patch(json, req->content_len) != ESP_OK ) {
        static const char resp[] = "Cannot parse configuration patch";
        httpd_resp_set_status(req, HTTPD_400);
        return httpd_resp_send(req, resp, sizeof(resp) - 1);
    }

    httpd_resp_set_type(req, "text/plain");
    return httpd_resp_send(req, "OK", 2);
}

static httpd_uri_t patch_conf = {
    .uri = "/conf",
    .method = HTTP_PATCH,
    .handler = handle_patch_conf
};

/**
 * Handles OPTIONS "/conf" requests.
 *
 * Reports allowed method.
 *
 * @param req HTTP request
 */
static esp_err_t handle_options_conf(httpd_req_t * req)
{
    httpd_resp_set_hdr(req, "Allow", "GET, PATCH");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Methods", "GET, PATCH");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Headers", "Content-Type");
    return httpd_resp_send(req, "", 0);
}

static httpd_uri_t options_conf = {
    .uri = "/conf",
    .method = HTTP_OPTIONS,
    .handler = handle_options_conf
};

/**
 * Handles GET "/ctrl/wm" requests.
 *
 * Returns the light control task watermark as plain text.
 *
 * @param req HTTP request
 */
static esp_err_t handle_get_ctrl_watermark(httpd_req_t * req)
{
    int wm = get_lights_ctrl_task_high_watermark();
    if ( wm < 0 ) {
        static const char resp[] = "Lights control task is not running";
        httpd_resp_set_status(req, HTTPD_404);
        return httpd_resp_send(req, resp, sizeof(resp) - 1);
    }
    char resp[16];
    int len = snprintf(resp, sizeof(resp), "%u", wm);
    if ( len < 0 || sizeof(resp) <= len ) {
        static const char resp[] = "Cannot generate a response";
        httpd_resp_set_status(req, HTTPD_500);
        return httpd_resp_send(req, resp, sizeof(resp) - 1);
    }
    httpd_resp_set_type(req, "text/plain");
    return httpd_resp_send(req, resp, len);
}

static httpd_uri_t get_ctrl_watermark = {
    .uri = "/ctrl/wm",
    .method = HTTP_GET,
    .handler = handle_get_ctrl_watermark
};

/**
 * Handles GET "/ctrl/st" requests.
 *
 * Returns the light control task current state as plain text.
 *
 * @param req HTTP request
 */
static esp_err_t handle_get_ctrl_state(httpd_req_t * req)
{
    char descr[88];
    size_t len = get_lights_ctlr_status(descr, sizeof(descr));

    httpd_resp_set_type(req, "text/plain");
    return httpd_resp_send(req, descr, len);
}

static httpd_uri_t get_ctrl_state = {
    .uri = "/ctrl/st",
    .method = HTTP_GET,
    .handler = handle_get_ctrl_state
};

/**
 * Handles GET "/ctrl" requests.
 *
 * Returns the light switch state as plain text.
 *
 * @param req HTTP request
 */
static esp_err_t handle_get_ctrl(httpd_req_t * req)
{
    const char * resp = ( are_lights_on() ? "1" : "0" );

    httpd_resp_set_type(req, "text/plain");
    return httpd_resp_send(req, resp, 1);
}

static httpd_uri_t get_ctrl = {
    .uri = "/ctrl",
    .method = HTTP_GET,
    .handler = handle_get_ctrl
};

/**
 * Handles PUT "/ctrl" requests.
 *
 * Turns the lights on or off.
 *
 * @param req HTTP request
 */
static esp_err_t handle_put_ctrl(httpd_req_t * req)
{
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");

    if ( req->content_len == 0 ) {
        httpd_resp_set_status(req, "304 Not Modified");
        static const char resp[] = "Request was empty";
        return httpd_resp_send(req, resp, sizeof(resp) - 1);
    }

    char state[4];
    if ( req->content_len >= sizeof(state) ) {
        static const char resp[] = "Request is too long";
        httpd_resp_set_status(req, "413 Payload Too Large");
        return httpd_resp_send(req, resp, sizeof(resp) - 1);
    }

    if ( read_request_payload(req, state, sizeof(state)) != ESP_OK ) {
        static const char resp[] = "Cannot read request payload";
        httpd_resp_set_status(req, HTTPD_500);
        return httpd_resp_send(req, resp, sizeof(resp) - 1);
    }

    unsigned int pos;
    if ( sscanf(state, "%u", &pos) != 1 || pos > 1 ) {
        static const char resp[] = "Cannot parse this request";
        httpd_resp_set_status(req, HTTPD_400);
        return httpd_resp_send(req, resp, sizeof(resp) - 1);
    }

    if ( pos ) {
        lights_on();
    } else {
        lights_off();
    }

    httpd_resp_set_type(req, "text/plain");
    return httpd_resp_send(req, "OK", 2);
}

static httpd_uri_t put_ctrl = {
    .uri = "/ctrl",
    .method = HTTP_PUT,
    .handler = handle_put_ctrl
};

/**
 * Handles OPTIONS "/ctrl" requests.
 *
 * Reports allowed methods.
 *
 * @param req HTTP request
 */
static esp_err_t handle_options_ctrl(httpd_req_t * req)
{
    httpd_resp_set_hdr(req, "Allow", "GET, PUT");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Methods", "GET, PUT");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Headers", "Content-Type");
    return httpd_resp_send(req, "", 0);
}

static httpd_uri_t options_ctrl = {
    .uri = "/ctrl",
    .method = HTTP_OPTIONS,
    .handler = handle_options_ctrl
};

/**
 * Handles POST "/fw" (firmware update) requests.
 * Post's payload is the URL of the firmware file.
 *
 * @note The web server that serves the firmware update
 * must be on the same network as the device.
 *
 * The following steps represent the workflow of OTA
 * firmware update:
 * 1) build the firmware and start the web server
 * ```sh
 * make
 * python -m http.server
 * ```
 * 2) find out WiFi IP address of the build machine
 * ```sh
 * ip addr
 * ```
 * 3) send the firware update request to the device
 * ```sh
 * curl http://device.name.or.ip/fw -d 'http://fw.server.name.or.ip/build/elisa.bin'
 * ```
 * Device will download the BIN file, update itself and reboot.
 *
 * @param req HTTP request
 */
static esp_err_t handle_post_firmware(httpd_req_t * req)
{
    if ( req->content_len == 0 ) {
        static const char resp[] = "Firmware update URL is expected";
        httpd_resp_set_status(req, "411 Length Required");
        return httpd_resp_send(req, resp, sizeof(resp) - 1);
    }

    char url[100];
    if ( req->content_len >= sizeof(url) ) {
        static const char resp[] = "Firmware update URL is too long";
        httpd_resp_set_status(req, "413 Payload Too Large");
        return httpd_resp_send(req, resp, sizeof(resp) - 1);
    }

    if ( read_request_payload(req, url, sizeof(url)) != ESP_OK ) {
        static const char resp[] = "Cannot read request payload";
        httpd_resp_set_status(req, HTTPD_500);
        return httpd_resp_send(req, resp, sizeof(resp) - 1);
    }

    if ( strncmp(url, "http://", 7) != 0 ) {
        static const char resp[] = "Firmware update URL must start with http://";
        httpd_resp_set_status(req, HTTPD_404);
        return httpd_resp_send(req, resp, sizeof(resp) - 1);
    }

    char * host_name = url + 7;
    char host_name_term = ':';
    char * host_name_end = strchr(host_name, host_name_term);
    if ( host_name_end == NULL ) {
        host_name_term = '/';
        host_name_end = strchr(host_name, host_name_term);
        if ( host_name_end == NULL ) {
            static const char resp[] = "Cannot find server name";
            httpd_resp_set_status(req, HTTPD_404);
            return httpd_resp_send(req, resp, sizeof(resp) - 1);
        }
    }
    *host_name_end = '\0';

    struct hostent * he = gethostbyname(host_name);
    int resolve_count = 10;
    while ( he == NULL && --resolve_count > 0 ) {
        vTaskDelay(200 / portTICK_RATE_MS);
        he = gethostbyname(host_name);
    }
    if ( he == NULL ) {
        static const char resp[] = "Cannot resolve server address";
        httpd_resp_set_status(req, HTTPD_404);
        return httpd_resp_send(req, resp, sizeof(resp) - 1);
    }
    struct in_addr * ip_addr = (struct in_addr *) he->h_addr;
    ip4_addr_t server_ip;
    inet_addr_to_ip4addr(&server_ip, ip_addr);
    if ( !is_peer_local(server_ip) ) {
        static const char resp[] = "Firmware server is not local";
        httpd_resp_set_status(req, "403 Forbidden");
        return httpd_resp_send(req, resp, sizeof(resp) - 1);
    }
    *host_name_end = host_name_term;

    size_t url_len = req->content_len + 1;
    char * fw_url = malloc(url_len);
    strncpy(fw_url, url, url_len);
    ESP_LOGI(TAG, "Initiating FW update from %s", fw_url);

    start_firmware_update(fw_url);

    return httpd_resp_send(req, "OK", 2);
}

static httpd_uri_t post_firmware = {
    .uri = "/fw",
    .method = HTTP_POST,
    .handler = handle_post_firmware
};

static httpd_handle_t httpd = NULL;

esp_err_t start_websrv()
{
    if ( httpd != NULL ) {
        return ESP_OK;
    }

    httpd_config_t conf = HTTPD_DEFAULT_CONFIG();
    conf.max_uri_handlers = 11;
    ESP_LOGI(TAG, "Starting web server on port %d...", conf.server_port);

    esp_err_t res;
    if (
        ( res = httpd_start(&httpd, &conf) )                                == ESP_OK &&
        ( res = httpd_register_uri_handler(httpd, &get_root) )              == ESP_OK &&
        ( res = httpd_register_uri_handler(httpd, &get_icon) )              == ESP_OK &&
        ( res = httpd_register_uri_handler(httpd, &get_conf) )              == ESP_OK &&
        ( res = httpd_register_uri_handler(httpd, &patch_conf) )            == ESP_OK &&
        ( res = httpd_register_uri_handler(httpd, &options_conf) )          == ESP_OK &&
        ( res = httpd_register_uri_handler(httpd, &get_ctrl) )              == ESP_OK &&
        ( res = httpd_register_uri_handler(httpd, &put_ctrl) )              == ESP_OK &&
        ( res = httpd_register_uri_handler(httpd, &options_ctrl) )          == ESP_OK &&
        ( res = httpd_register_uri_handler(httpd, &get_ctrl_state) )        == ESP_OK &&
        ( res = httpd_register_uri_handler(httpd, &get_ctrl_watermark) )    == ESP_OK &&
        ( res = httpd_register_uri_handler(httpd, &post_firmware) )         == ESP_OK
    ) {
        ESP_LOGI(TAG, "Web server started");
    } else {
        ESP_LOGW(TAG, "Cannot start the web server - error %d.", res);
    }
    return res;
}
