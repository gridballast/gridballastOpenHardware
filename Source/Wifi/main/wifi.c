#include "freertos/FreeRTOS.h"
#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_event_loop.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include <stdio.h>
#include <string.h>


// Event Loop Handler:
// Should be activated after the event loop 
esp_err_t event_handler(void *ctx, system_event_t *event)
{
    if (event->event_id == SYSTEM_EVENT_SCAN_DONE) {
        uint16_t apNumber; 
        esp_wifi_scan_get_ap_num(&apNumber);
        printf("Number of APs = %d\n ", apNumber);
        wifi_ap_record_t *ap_records = (wifi_ap_record_t*) malloc(sizeof(wifi_ap_record_t) * apNumber);
        ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&apNumber, ap_records));
        int i;
        for (i=0; i < apNumber; i++) {
            char *authmode;

            if (strlen((char *) ap_records[i].ssid) != 0){
                switch(ap_records[i].authmode) {
                    case WIFI_AUTH_OPEN:
                       authmode = "WIFI_AUTH_OPEN";
                       break;
                    case WIFI_AUTH_WEP:
                       authmode = "WIFI_AUTH_WEP";
                       break;
                    case WIFI_AUTH_WPA_PSK:
                       authmode = "WIFI_AUTH_WPA_PSK";
                       break;
                    case WIFI_AUTH_WPA2_PSK:
                       authmode = "WIFI_AUTH_WPA2_PSK";
                       break;
                    case WIFI_AUTH_WPA_WPA2_PSK:
                       authmode = "WIFI_AUTH_WPA_WPA2_PSK";
                       break;
                    default:
                       authmode = "Unknown";
                    break; 
                }
                printf("ssid=%s, authmode=%s\n", ap_records[i].ssid, authmode);
            }
        }
        free(ap_records);
    }
    else {
        //printf("Something else Happened: %d", event->event_id);
    }
    return ESP_OK;
}

void app_main(void)
{

    nvs_flash_init();
    tcpip_adapter_init();

    // Initialize Event Loop for the TCP/IP module and WIFI 
    // module to send events to handler
    ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));

    // Initialize the Wifi Module 
    wifi_init_config_t config = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&config));

    // Set the mode you want to initialize the wifi to:
    //      WIFI_MODE_NULL          Mode is left floating
    //      WIFI_MODE_STA           Make device a station
    //      WIFI_MODE_AP            Make device an Access Point (AP)
    //      WIFI_MODE_APSTA         Make device an AP and a Station
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));

    // Set where the storage 
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));


    /*
     *  Setup AP parameters for device as a Access Point 
     *
     */

    wifi_config_t ap_config = {
        .ap = {
            .ssid = "Nard Wifi",
            .password = "ESP32",
            .ssid_len = 0,
            .channel = 0,
            .authmode = WIFI_AUTH_OPEN,
            .ssid_hidden = false,
            .max_connection = 4,
            .beacon_interval = 100
        }
    };

    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &ap_config));


    // Start the module then scan for the SSIDs
    ESP_ERROR_CHECK(esp_wifi_start());


    /*
     *   
     *  Configuration for Scanning 
     *  When the device is set to a station 
     *
     */

    //Configure the Scanning Parameters
    // wifi_scan_config_t scanConfig = {
    //     .ssid = NULL,
    //     .bssid = NULL,
    //     .channel = 0,
    //     .show_hidden = 1
    // };

    // Scan the area and do not block the caller process (Hence the 0)
    // ESP_ERROR_CHECK(esp_wifi_scan_start(&scanConfig, 0));
    
    while (true) {

    }
}

