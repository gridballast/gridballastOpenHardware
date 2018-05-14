/**
 * @file wifi_module.c
 *
 * @brief wifi task to send and receive system state fields from OpenChirp
 *
 * @author Aaron Perley (aperley@andrew.cmu.edu)
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
#include "cJSON.h"
#include "esp_request.h"
#include "wifi_module.h"
#include "util.h"
#include "config.h"
#include "nvs.h"
#include "config_server.h"

typedef enum {
    MODULE_MODE_NORMAL,
    MODULE_MODE_CONFIG
} module_mode_t;

/* OpenChirp transducer ids for system state fields */
#define TRANSDUCER_ID_TEMP_BOTTOM "5a016520f230cf7055615e56"
#define TRANSDUCER_ID_TEMP_TOP    "5a01652df230cf7055615e57"
#define TRANSDUCER_ID_GRID_FREQ   "5a9c8b4fa447657867c7a286"
#define TRANSDUCER_ID_SET_POINT   "5a01655af230cf7055615e5b"

const char * const wifi_task_name = "wifi_module_task";
static const char *TAG = "wifi";

static volatile module_mode_t module_mode;

/** @brief FreeRTOS event group to signal when softap is up */
static EventGroupHandle_t wifi_event_group;
const int SOFTAP_UP_BIT = BIT1;
const int CONNECTED_BIT = BIT0;

/* OpenChirp API definitions */
static const char * const HOSTNAME = "openchirp.io";
static const char * const BASE_URL = "https://api.openchirp.io/apiv1/device/5a011bb4f230cf7055615e4c/transducer/";
static const char * const AUTH_HEADER = "Authorization: Basic NWEwMTFiYjRmMjMwY2Y3MDU1NjE1ZTRjOlA0UUtadGtaMGdqY2dIaU9DdVlnT09VNFNPVEdwODA=";
static const char * const USER_AGENT_HEADER = "User-Agent: gridballast1.1";

static system_state_t system_state;

/* Static function definitions */
static void reset_transducer_response();
static void run_mode_normal();
static void run_mode_config();

/**
 * @brief Wifi event handler
 */
static esp_err_t event_handler(void *ctx, system_event_t *event) {
    switch(event->event_id) {
    case SYSTEM_EVENT_AP_START:
        ESP_LOGI(TAG, "Got event AP_START");
        xEventGroupSetBits(wifi_event_group, SOFTAP_UP_BIT);
        break;
    case SYSTEM_EVENT_AP_STOP:
        ESP_LOGI(TAG, "Got event AP_STOP");
        xEventGroupClearBits(wifi_event_group, SOFTAP_UP_BIT);
        break;
    case SYSTEM_EVENT_AP_STACONNECTED:
        ESP_LOGI(TAG, "Got event AP_STACONNECTED");
        break;
    case SYSTEM_EVENT_AP_STADISCONNECTED:
        ESP_LOGI(TAG, "Got event AP_STADISCONNECTED");
        break;
    case SYSTEM_EVENT_STA_START:
        ESP_LOGI(TAG, "Got event start");
        esp_wifi_connect();
        break;
    case SYSTEM_EVENT_STA_STOP:
        ESP_LOGI(TAG, "Got event stop");
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

/**
 * @brief Common wifi initialization that occurs before the wifi task starts
 */
static void init_wifi(void) {
    tcpip_adapter_init();

    wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK( esp_event_loop_init(event_handler, NULL) );

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
    ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );

    config_init();
}

/**
 * @brief Set wifi to station mode (connect to access point)
 */
static void init_mode_sta(const char *ssid, const char *password) {
    wifi_config_t wifi_config;
    strncpy((char *)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid));
    strncpy((char *)wifi_config.sta.password, password, sizeof(wifi_config.sta.password));

    ESP_LOGI(TAG, "Setting WiFi configuration SSID %s password %s", wifi_config.sta.ssid, wifi_config.sta.password);
    ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK( esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );

    ESP_ERROR_CHECK( esp_wifi_start() );
}

/**
 * @brief Set wifi to soft-ap mode (serve as access point)
 */
static void init_mode_ap(void) {
    ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_AP) );
    wifi_config_t ap_config = {
        .ap = {
            .ssid = "gridballast",
            .channel = 0,
            .authmode = WIFI_AUTH_OPEN,
            .ssid_hidden = 0,
            .max_connection = 4,
            .beacon_interval = 100
        }
    };
    esp_wifi_set_config(WIFI_IF_AP, &ap_config);
    ESP_ERROR_CHECK( esp_wifi_start() );
    ESP_LOGI(TAG, "Wifi started in AP mode, SSID %s", ap_config.ap.ssid);
}

/*****************************************
 ************ MODULE FUNCTIONS ***********
 *****************************************/

/**
 * @brief pointer to OpenChirp API response body
 *
 * This is filled in but the get_transducer_download_callback function
 */
static char *transducer_response = NULL;
/** @brief total length of transducer response array */
static int transducer_response_len = 0;

/**
 * @brief reset and free the transducer response buffer
 */
static void reset_transducer_response()
{
    if (transducer_response != NULL) {
        free(transducer_response);
        transducer_response = NULL;
    }
    transducer_response_len = 0;
}

/**
 * @brief callback that runs when the body of transducers index is received
 *
 * @return 0 on success,
 *         -1 on failure (signals to the http client to stop reading response data)
 *
 * @note this may be called more than once by the http client if the response body
 *       is larger than its internal buffer
 */
static int get_transducer_download_callback(request_t *req, char *data, int len)
{
    if (transducer_response == NULL) {
        transducer_response = malloc(len + 1);
        if (transducer_response == NULL) {
            ESP_LOGE(TAG, "Malloc error");
            return -1;
        }
    } else {
        transducer_response = realloc(transducer_response, transducer_response_len + len + 1);
        if (transducer_response == NULL) {
            ESP_LOGE(TAG, "Realloc error");
            return -1;
        }
    }

    memcpy(&transducer_response[transducer_response_len], data, len);
    transducer_response_len += len;
    transducer_response[transducer_response_len] = '\0'; // null terminate
    return 0;
}

/**
 * @brief parse the latest transducer value out of transducer index json response
 *
 * @param response      transducers index response body, should be a json array of objects
 * @param transducer_id OpenChirp id of the transducer to read
 * @param value         pointer to be filled in with the numeric transducer value
 *
 * @return 0 on success, -1 on parsing failure
 *
 * Example json response that will be parsed correctly:
 * [
 *     {
 *          "value": "150",
 *          "timestamp": "2018-04-10T19:07:50.980699904Z",
 *          "name": "set_point",
 *          "unit": "celsius",
 *          "_id": "5a01655af230cf7055615e5b",
 *          "is_actuable": true
 *      }
 *  ]
 */
static int parse_transducer_value(const char *response, const char *transducer_id, double *value) {
    if (value == NULL) {
        return -1;
    }

    int ret = -1;

    cJSON *transducer_array = cJSON_Parse(response);
    if (transducer_array == NULL) {
        goto parse_transducer_value_end;
    }
    const cJSON *transducer;
    cJSON_ArrayForEach(transducer, transducer_array) {
        if (transducer == NULL) {
            goto parse_transducer_value_end;
        }
        const cJSON *id_field = cJSON_GetObjectItemCaseSensitive(transducer, "_id");
        if (id_field == NULL) {
            goto parse_transducer_value_end;
        }

        if (strcmp(id_field->valuestring, transducer_id) == 0) {
            const cJSON *value_field = cJSON_GetObjectItemCaseSensitive(transducer, "value");
            if (value == NULL) {
                goto parse_transducer_value_end;
            }
            if (cJSON_IsNumber(value_field)) {
                *value = value_field->valuedouble;
            } else {
                *value = atof(value_field->valuestring);
            }
            ret = 0;
            break;
        }
    }

    parse_transducer_value_end:
    cJSON_Delete(transducer_array);
    return ret;
}

/**
 * @brief poll OpenChirp for the latest transducer value using the REST API
 *
 * @param transducer_id OpenChirp transducer id
 * @param value         pointer that will be filled in with the latest numeric value
 *
 * @return 0 on success, -1 on request or parse failure
 */
static int get_transducer_value(const char *transducer_id, double *value) {
    ESP_LOGI(TAG, "fetching transducers %s", BASE_URL);
    transducer_response = NULL;
    request_t *req = req_new(BASE_URL);
    req_setopt(req, REQ_SET_HEADER, (void*)AUTH_HEADER);
    req_setopt(req, REQ_SET_HEADER, (void*)USER_AGENT_HEADER);
    // use HTTP 1.0 to prevent OpenChirp from using chunked transfer encoding/
    // which the HTTP client library cannot parse
    req_setopt(req, REQ_SET_HTTP_VER, (void*)HTTP_VER_1_0);
    req_setopt(req, REQ_FUNC_DOWNLOAD_CB, get_transducer_download_callback);
    int status = req_perform(req);
    req_clean(req);

    int ret = 0;
    if (status != 200) {
        ESP_LOGE(TAG, "Error receiving transducer value, received non-200 response: %d", status);
        ret = -1;
    }

    if (ret == 0 && parse_transducer_value(transducer_response, transducer_id, value) != 0) {
        ESP_LOGE(TAG, "Error parsing transducer value");
        ret = -1;
    }

    if (ret == 0) {
        ESP_LOGI(TAG, "received from transducer %s, value = %0.4f", transducer_id, *value);
    }
    reset_transducer_response();
    return ret;

}

/**
 * @brief post a transducer value to OpenChirp using the REST API
 *
 * @param transducer_id OpenChirp transducer id
 * @param value         transducer value as a string (sprintf'd)
 *
 * @return 0 on success, -1 on failure
 */
static int send_transducer_value(const char *transducer_id, const char *value) {
    int url_len = strlen(BASE_URL) + strlen(transducer_id);
    char *url = malloc(url_len + 1);
    if (url == NULL) {
        ESP_LOGE(TAG, "Malloc failed");
        return -1;
    }
    strcpy(url, BASE_URL);
    strcat(url, transducer_id);

    ESP_LOGI(TAG, "sending transducer value");
    request_t *req = req_new(url);
    req_setopt(req, REQ_SET_METHOD, "POST");
    req_setopt(req, REQ_SET_HEADER, (void*)AUTH_HEADER);
    req_setopt(req, REQ_SET_HEADER, (void*)USER_AGENT_HEADER);
    req_setopt(req, REQ_SET_HEADER, "Connection: close");
    req_setopt(req, REQ_SET_HEADER, "Content-Type: text/plain");
    req_setopt(req, REQ_SET_DATAFIELDS, (void*)value);
    int status = req_perform(req);
    req_clean(req);
    free(url);

    if (status != 200) {
        ESP_LOGE(TAG, "Error sending transducer value, received non-200 response: %d", status);
        return -1;
    }

    ESP_LOGI(TAG, "posted to transducer %s, value = %s", transducer_id, value);
    return 0;
}

/**
 * @brief post data from system state to OpenChirp
 *
 * @param system_state snapshotted system state struct
 *
 * @return 0 on success, -1 on failure
 */
static int send_data(system_state_t *system_state) {
    ESP_LOGI(TAG, "sending data");
    char data_buf[16]; // make sure this is large enough to hold the sprintf'd value
    int err = 0;

    // only update grid frequency if it is nonzero so we don't push bogus value when
    // the zero crossing circuit is not connected
    if (system_state->grid_freq > 5.0) {
        sprintf(data_buf, "%.4f", system_state->grid_freq);
        err = send_transducer_value(TRANSDUCER_ID_GRID_FREQ, data_buf);
    }

    if (err == 0) {
        sprintf(data_buf, "%d", system_state->temp_bottom);
        err = send_transducer_value(TRANSDUCER_ID_TEMP_BOTTOM, data_buf);
    }

    if (err == 0) {
        sprintf(data_buf, "%d", system_state->temp_top);
        err = send_transducer_value(TRANSDUCER_ID_TEMP_TOP, data_buf);
    }

    if (err == 0) {
        sprintf(data_buf, "%d", system_state->set_point);
        err = send_transducer_value(TRANSDUCER_ID_SET_POINT, data_buf);
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
    // try to read saved wifi config
    char ssid[ssid_maxlen];
    char pass[pass_maxlen];
    esp_err_t err = config_get_wifi(ssid, ssid_maxlen, pass, pass_maxlen);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        // no saved values, so enter config mode
        module_mode = MODULE_MODE_CONFIG;
    } else {
        ESP_ERROR_CHECK(err);
        // saved values, so enter normal mode
        module_mode = MODULE_MODE_NORMAL;
    }

    while (1) {
        // loop to allow exiting normal mode and entering config mode
        switch (module_mode) {
            case MODULE_MODE_NORMAL:
                init_mode_sta(ssid, pass);
                run_mode_normal();
                break;
            case MODULE_MODE_CONFIG:
                init_mode_ap();
                run_mode_config();
                break;
        }
        ESP_ERROR_CHECK(esp_wifi_stop());
    }
}

static void run_mode_normal() {
    reset_transducer_response();
    ESP_LOGI(TAG, "Running normal mode");
    while (module_mode == MODULE_MODE_NORMAL) {
        /* Wait for the callback to set the CONNECTED_BIT in the
           event group.
        */
        xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT,
                            false, true, portMAX_DELAY);
        ESP_LOGI(TAG, "Connected to AP");

        // read system state into local copy
        rwlock_reader_lock(&system_state_lock);
        get_system_state(&system_state);
        rwlock_reader_unlock(&system_state_lock);

        // send data to openchirp
        send_data(&system_state);

        // get data from openchirp
        double set_point;
        if (get_transducer_value(TRANSDUCER_ID_SET_POINT, &set_point) == 0) {
            rwlock_reader_lock(&system_state_lock);
            get_system_state(&system_state);
            system_state.set_point = set_point;
            set_system_state(&system_state);
            rwlock_reader_unlock(&system_state_lock);
        }

        for (int countdown = 9; countdown >= 0; countdown--) {
            ESP_LOGI(TAG, "%d... ", countdown);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
        ESP_LOGI(TAG, "Starting again!");
    }
}


static void run_mode_config() {
    xEventGroupWaitBits(wifi_event_group, SOFTAP_UP_BIT, false, true, portMAX_DELAY);
    tcpip_adapter_ip_info_t ip_info;
    ESP_ERROR_CHECK( tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_AP, &ip_info) );
    ESP_LOGI(TAG, "Config mode: connect to the gridballast wifi network and go to http://%s", ip4addr_ntoa(&ip_info.ip));
    ESP_LOGI(TAG, "Reboot to exit config mode");

    config_server_run();
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
    init_wifi();
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

/**
 * @brief exit normal data publish/receive mode and enter configuration mode
 *
 * @note configuration mode can only be exited by rebooting the module
 * @note this function can be called from other tasks or interrupts
 */
void wifi_enter_config_mode() {
    module_mode = MODULE_MODE_CONFIG;
}
