/* Hello World Example

 This example code is in the Public Domain (or CC0 licensed, at your option.)

 Unless required by applicable law or agreed to in writing, this
 software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 CONDITIONS OF ANY KIND, either express or implied.
 */
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "driver/ledc.h"
#include "lwip/err.h"
#include "lwip/sockets.h"

/* FreeRTOS event group to signal when we are connected & ready to make a request */
static EventGroupHandle_t wifi_event_group;

/* The event group allows multiple bits for each event,
 but we only care about one event - are we connected
 to the AP with an IP? */
static const int CONNECTED_BIT = BIT0;

static const char *TAG = "example";


/* The examples use simple WiFi configuration that you can set via
 'make menuconfig'.

 If you'd rather not, just change the below entries to strings with
 the config you want - ie #define EXAMPLE_WIFI_SSID "mywifissid"
 */
#define SSID_NAME "Anthony Rowe is Cool"
// Password must be more than 8 characters 
#define SSID_PASS "1234567890"

static esp_err_t event_handler(void *ctx, system_event_t *event) {
   switch (event->event_id) {
   case SYSTEM_EVENT_STA_START:
      esp_wifi_connect();
      break;
   case SYSTEM_EVENT_STA_GOT_IP:
      xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
      break;
   case SYSTEM_EVENT_STA_DISCONNECTED:
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

static void initialize_wifi(void) {
   tcpip_adapter_init();
   wifi_event_group = xEventGroupCreate();
   ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));
   wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
   ESP_ERROR_CHECK(esp_wifi_init(&cfg));
   ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
   wifi_config_t ap_config = {.ap = {
            .ssid = SSID_NAME,
            .password = SSID_PASS,
            .authmode = WIFI_AUTH_WPA2_PSK,
            .max_connection = 2
        }
    };
   ESP_LOGI(TAG, "Setting WiFi configuration SSID %s...", ap_config.sta.ssid);
   ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
   ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &ap_config));
   ESP_ERROR_CHECK(esp_wifi_start());
   ESP_LOGI(TAG, "The wifi is setup correctly!");
}

const static char http_html_msg[] =
      "HTTP/1.1 200 OK\r\nContent-type: text/html\r\nHost: NardServer/2.1\r\nConnection: close\r\n\r\n"
      "<html><head><title>Congrats!</title></head><body><h1>Welcome to our lwIP HTTP server!</h1><p>This is a small test page, served by httpserver-netconn.</body></html>";



static void http_server_thread() {
   //xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT,
   //false, true, portMAX_DELAY);
   int sock = socket(AF_INET, SOCK_STREAM, 0);
   printf("New sock = %d\n", sock);

   struct sockaddr_in serverAddress;
   serverAddress.sin_family = AF_INET;
   serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
   serverAddress.sin_port = htons(80);
   int rc = bind(sock, (struct sockaddr * ) &serverAddress,
         sizeof(serverAddress));

   printf("bind completed! %d\n", rc);
   printf("Server Address: %8x\n", serverAddress.sin_addr.s_addr);

   rc = listen(sock, 10);
   printf("listen completed! %d\n", rc);


   struct sockaddr_in clientAddress;
   socklen_t clientAddressLength = sizeof(clientAddress);
   int newsockfd, n;
   char buffer[256];
   while (1) {
      printf("Waiting connection accept\n");
      newsockfd = accept(sock, (struct sockaddr * ) &clientAddress,
            &clientAddressLength);
      printf("Accepted a client connection\n");
      if (newsockfd < 0)
         printf("ERROR on accept\n");

      bzero(buffer, 256);
      n = read(newsockfd, buffer, 255);
      printf("I've read %d\n", n);
      if (n < 0)
         printf("ERROR reading from socket\n");
      printf("Here is the message: %s\n", buffer);
      n = write(newsockfd, http_html_msg, sizeof(http_html_msg));
      if (n < 0)
         printf("ERROR writing to socket\n");
      close(newsockfd);
      printf("socket closed %d\n\n", newsockfd);
   }
}

void app_main() {
   nvs_flash_init();
   initialize_wifi();
   xTaskCreate(&http_server_thread, "http_server_netconn_thread", 2048, NULL,
         5, NULL);
}