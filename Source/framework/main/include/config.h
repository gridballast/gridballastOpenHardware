/**
 * @file config.h
 *
 * @brief manage config data saved in nonvolatile storage
 *
 * @author Aaron Perley <aperley@andrew.cmu.edu>
 */

#ifndef __config_module_h_
#define __config_module_h_

#include "esp_system.h"
#include "esp_wifi.h"

#define ssid_maxlen sizeof(((wifi_config_t *)0)->sta.ssid)
#define pass_maxlen sizeof(((wifi_config_t *)0)->sta.password)

void config_init();
esp_err_t config_get_wifi(char *ssid, size_t ssid_len, char *password, size_t password_len);
esp_err_t config_set(const char *field, const char *val);
esp_err_t config_commit();

#endif /* __config_module_h_ */

