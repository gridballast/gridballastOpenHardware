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

#define WIFI_SSID "CMU"
#define WIFI_PASS ""

/* OpenChirp transducer ids for system state fields */
#define TRANSDUCER_ID_TEMP_BOTTOM "5a016520f230cf7055615e56"
#define TRANSDUCER_ID_TEMP_TOP    "5a01652df230cf7055615e57"
#define TRANSDUCER_ID_GRID_FREQ   "5a9c8b4fa447657867c7a286"
#define TRANSDUCER_ID_SET_POINT   "5a01655af230cf7055615e5b"

const char * const wifi_task_name = "wifi_module_task";
static const char *TAG = "wifi";

/** @brief FreeRTOS event group to signal when we are connected & ready to make a request */
static EventGroupHandle_t wifi_event_group;

/* The event group allows multiple bits for each event,
   but we only care about one event - are we connected
   to the AP with an IP? */
const int CONNECTED_BIT = BIT0;

/* OpenChirp API definitions */
static const char * const HOSTNAME = "openchirp.io";
static const char * const BASE_URL = "https://api.openchirp.io/apiv1/device/5a011bb4f230cf7055615e4c/transducer/";
static const char * const AUTH_HEADER = "Authorization: Basic NWEwMTFiYjRmMjMwY2Y3MDU1NjE1ZTRjOlA0UUtadGtaMGdqY2dIaU9DdVlnT09VNFNPVEdwODA=";
static const char * const USER_AGENT_HEADER = "User-Agent: gridballast1.1";

static system_state_t system_state;

/* Static function definitions */
static void reset_transducer_response();

/**
 * @brief Wifi event handler
 */
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

/**
 * @brief Initialize the wifi module
 */
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

    reset_transducer_response();
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
    while(1) {
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
