/**
 * @file config.c
 *
 * @brief manage config data saved in nonvolatile storage
 *
 * @author Aaron Perley <aperley@andrew.cmu.edu>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "config.h"

static const char *TAG = "config";
static nvs_handle handle = 0;

void config_init() {
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES) {
        // NVS partition was truncated and needs to be erased
        ESP_LOGI(TAG, "Erasing and reinitializing NVS partition");
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    ESP_ERROR_CHECK(nvs_open("config", NVS_READWRITE, &handle));
    ESP_LOGI(TAG, "NVS handle opened");
}

esp_err_t config_get_wifi(char *ssid, size_t ssid_len, char *password, size_t password_len) {
    esp_err_t err = nvs_get_str(handle, "ssid", ssid, &ssid_len);
    if (err != ESP_OK) {
        return err;
    }

    err = nvs_get_str(handle, "pass", password, &password_len);
    return err;
}

esp_err_t config_set(const char *field, const char *val) {
    ESP_LOGI(TAG, "config setting %s = %s", field, val);
    return nvs_set_str(handle, field, val);
}

esp_err_t config_commit() {
    return nvs_commit(handle);
}
