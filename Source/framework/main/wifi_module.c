/**
 * @file wifi_module.c
 *
 * @brief wifi module related functionality
 *
 * @author Vikram Shanker (vshanker@cmu.edu)
 */

#include <stdio.h>
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "wifi_module.h"

const char * const wifi_task_name = "wifi_module_task";

/*****************************************
 ************ MODULE FUNCTIONS ***********
 *****************************************/

/**
 * @brief wifi task logic
 *
 * @param pv_parameters - parameters for task being create (should be NULL)
 *
 * @return void
 */
static void wifi_task_fn( void *pv_parameters ) {
  printf(" done (wifi)!\n");
  while(1);
  return;
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
