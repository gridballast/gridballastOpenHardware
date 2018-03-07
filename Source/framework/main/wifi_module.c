/**
 * @file wifi_module.c
 *
 * @brief wifi module related functionality
 *
 * @author Vikram Shanker (vshanker@cmu.edu)
 */

#include <stdio.h>
#include <string.h>
#include "esp_event_loop.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "lwip/dns.h"
#include "lwip/err.h"
#include "lwip/netdb.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "http_client.h"
#include "wifi_module.h"
#include "util.h"

const char * const wifi_task_name = "wifi_module_task";

/* FreeRTOS event group to signal when we are connected & ready to make a request */
static EventGroupHandle_t wifi_event_group;

/* The event group allows multiple bits for each event,
   but we only care about one event - are we connected
   to the AP with an IP? */
const int CONNECTED_BIT = BIT0;

#define WIFI_SSID "Abraham Linksys"
#define WIFI_PASS "emancipation1863"

static const char * const HOSTNAME = "openchirp.io";
static const int WEB_PORT = 7000;
static const char * const BASE_URL = "/api/device/5a011bb4f230cf7055615e4c/transducer/";
static const char * const AUTH_HEADER = "Authorization: Basic NWEwMTFiYjRmMjMwY2Y3MDU1NjE1ZTRjOlA0UUtadGtaMGdqY2dIaU9DdVlnT09VNFNPVEdwODA=";
static const char * const USER_AGENT_HEADER = "User-Agent: gridballast1.1";

static const char *TAG = "wifi";

static system_state_t mystate;
static http_client_t http_client;

static esp_err_t event_handler(void *ctx, system_event_t *event) {
    switch(event->event_id) {
    case SYSTEM_EVENT_STA_START:
        ESP_LOGI(TAG, "Got event start");
        esp_wifi_connect();
        break;
    case SYSTEM_EVENT_STA_GOT_IP:
        ESP_LOGI(TAG, "Got event got ip");
        xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
        break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
        ESP_LOGI(TAG, "Got event disconnected");
        /* This is a workaround as ESP32 WiFi libs don't currently
           auto-reassociate. */
        esp_wifi_connect();
        xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
        break;
    default:
        break;
    }
    return ESP_OK;
}

static void initialise_wifi(void) {
    tcpip_adapter_init();
    wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK( esp_event_loop_init(event_handler, NULL) );
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
    ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
        },
    };
    ESP_LOGI(TAG, "Setting WiFi configuration SSID %s...", wifi_config.sta.ssid);
    ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK( esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );

    // Log MAC address
    uint8_t mac[6];
    ESP_ERROR_CHECK( esp_wifi_get_mac(ESP_IF_WIFI_STA, mac) );
    char mac_str[18];
    sprintf(mac_str, "%02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1], mac[2],
                                                      mac[3], mac[4], mac[5]);
    ESP_LOGI(TAG, "My MAC Address: %s", mac_str);

    ESP_ERROR_CHECK( esp_wifi_start() );
}

/*****************************************
 ************ MODULE FUNCTIONS ***********
 *****************************************/

static int send_transducer_value(const char *transducer_id, const char *value) {
    int url_len = strlen(BASE_URL) + strlen(transducer_id);
    char url[url_len + 1];
    strcpy(url, BASE_URL);
    strcat(url, transducer_id);
    int err = http_req_begin(&http_client, HOSTNAME, WEB_PORT, HTTP_CLIENT_POST, url);
    if (err == 0) {
        err = http_req_add_header(&http_client, AUTH_HEADER);
    }
    if (err == 0) {
        err = http_req_add_header(&http_client, USER_AGENT_HEADER);
    }
    if (err == 0) {
        // tell the server to close the connection right away after responding
        err = http_req_add_header(&http_client, "Connection: close");
    }
    if (err == 0) {
        err = http_req_add_header(&http_client, "Content-Type: text/plain");
    }
    if (err == 0) {
        err = http_req_add_body(&http_client, value, strlen(value));
    }
    if (err == 0) {
        err = http_req_end(&http_client);
    }
    if (err == 0) {
        if (http_client.status_code != STATUS_OK) {
            ESP_LOGE(TAG, "Received non-200 response");
            err = -1;
        }
    } else {
        ESP_LOGE(TAG, "Error sending transducer value");
    }

    if (err == 0) {
        ESP_LOGI(TAG, "posted to transducer %s, value = %s", transducer_id, value);
        ESP_LOGI(TAG, "response = %d \"%s\"", http_client.status_code, http_client.buf.data);
    }
    return err;
}

static int send_data() {
    char data_buf[16]; // make sure this is large enough to hold the sprintf'd value
    int err = 0;

    if (mystate.grid_freq > 5.0) {
        const char * const grid_freq_id = "5a9c8b4fa447657867c7a286";
        sprintf(data_buf, "%.4f", mystate.grid_freq);
        err = send_transducer_value(grid_freq_id, data_buf);
    }

    return err;
}

/**
 * @brief wifi task logic
 *
 * @param pv_parameters - parameters for task being create (should be NULL)
 *
 * @return void
 */
static void wifi_task_fn( void *pv_parameters ) {
    while(1) {
        /* Wait for the callback to set the CONNECTED_BIT in the
           event group.
        */
        xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT,
                            false, true, portMAX_DELAY);
        ESP_LOGI(TAG, "Connected to AP");

        // reast system state into local copy
        rwlock_reader_lock(&system_state_lock);
        get_system_state(&mystate);
        rwlock_reader_unlock(&system_state_lock);
        // send data to openchirp
        send_data();

        for (int countdown = 9; countdown >= 0; countdown--) {
            ESP_LOGI(TAG, "%d... ", countdown);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
        ESP_LOGI(TAG, "Starting again!");
    }
}

/*****************************************
 *********** INTERFACE FUNCTIONS *********
 *****************************************/

/**
 * @brief initialization task that starts all other threads
 *
 * @return void
 */
void wifi_init_task( void ) {

    printf("Intializing Wifi System...");
    initialise_wifi();
    xTaskCreate(
                &wifi_task_fn, /* task function */
                wifi_task_name, /* wifi task name */
                wifiUSStackDepth, /* stack depth */
                NULL, /* parameters to fn_name */
                wifiUXPriority, /* task priority */
                NULL /* task handle ( returns an id basically ) */
               );
    fflush(stdout);
}
