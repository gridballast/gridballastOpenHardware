/**
 * @file config_server.h
 *
 * @brief serve configuration webpage
 *
 * @author Aaron Perley <aperley@andrew.cmu.edu>
 */

#include <string.h>
#include <stdlib.h>
#include "config.h"
#include "esp_log.h"
#include "http_server.h"
#include "http_parser.h"
#include "nvs.h"

static const char *TAG = "config_server";

static const char * const config_html_fmt =
"<!DOCTYPE html>"
"<html>"
"<body>"
"<form action=\"/\" method=\"post\">"
	"<label>SSID</label><input type=\"text\" name=\"ssid\" value=\"%s\"><br>"
	"<label>Password</label><input type=\"text\" name=\"pass\" value=\"%s\"><br>"
	"<input type=\"submit\" value=\"Submit\">"
"</form>"
"</body>"
"</html>";

static void send_html(http_client_conn_t *conn) {
    // try to read saved wifi config
    char ssid[ssid_maxlen] = "";
    char pass[pass_maxlen] = "";
    esp_err_t err = config_get_wifi(ssid, ssid_maxlen, pass, pass_maxlen);
    if (err != ESP_ERR_NVS_NOT_FOUND) {
        ESP_ERROR_CHECK(err);
    }

	int buf_len = snprintf(NULL, 0, config_html_fmt, ssid, pass);
	char *buf = malloc(buf_len + 1);
	if (buf == NULL) {
		return;
	}
	snprintf(buf, buf_len + 1, config_html_fmt, ssid, pass);

    http_client_send(conn, buf);
	free(buf);
}

static void handle_get(http_client_conn_t *conn) {
    ESP_LOGI(TAG, "Got GET request");
	send_html(conn);
}

static void parse_body(char *body) {
    char *field = strtok(body, "=");
    int fields_set = 0;
	while (field != NULL) {
        if (strcmp(field, "ssid") != 0 && strcmp(field, "pass") != 0) {
            ESP_LOGE(TAG, "Unknown form field %s", field);
            return;
        }

        // copy from body into field
        char *val = strtok(NULL, "&");
        if (val == NULL) {
            return;
        }

        // support spaces in value (which get encoded into +s)
        for (char *chr = val; *chr != '\0'; chr++) {
            if (*chr == '+') {
                *chr = ' ';
            }
        }
        ESP_ERROR_CHECK(config_set(field, val));
        fields_set++;

        // advance to next field
        field = strtok(NULL, "=");
    }

    if (fields_set == 2) {
        ESP_ERROR_CHECK(config_commit());
    }
}

static void handle_post(http_client_conn_t *conn, char *body) {
    ESP_LOGI(TAG, "Got POST request %s", body);
	parse_body(body);
	send_html(conn);
}

static void req_handler(http_client_conn_t *client_conn, http_req_t *req) {
    if (strcmp(req->url, "/") == 0) {
        // only serve index route
        switch(req->method) {
            case HTTP_GET:
                handle_get(client_conn);
                break;
            case HTTP_POST:
                handle_post(client_conn, req->body);
                break;
            default:
                ESP_LOGE(TAG, "Unsupported HTTP method %d", req->method);
                break;
        }
    }
}

// Interface functions
void config_server_run() {
    http_server_run(80, req_handler);
}
