/**
 * @file grid_ballast_main.c
 *
 * @brief main function that starts up the grid ballast system
 *
 * @author Vikram Shanker (vshanker@cmu.edu)
 */

#include <stdio.h>
#include <string.h> /* memset */
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"

#include "controller_module.h"
#include "wifi_module.h"
#include "sensing_module.h"
#include "system_state.h"


system_state_t gb_system_state;

/**
 * @brief initialization task that starts all other threads
 *
 * @param pv_parameters - parameters for task being created (should be NULL)
 *
 * @return void
 */
void init_task( void *pv_parameters ) {
    printf("Intializing GridBallast system...\n");
    wifi_init_task();
    sensing_init_task();
    controller_init_task();
    while(1);
}

/**
 * @brief main function that starts the initialization task
 *
 * @return void
 */
void app_main( void ) {
    nvs_flash_init();
    /* initialize gb_system_state to 0's */
    memset(&gb_system_state, 0, size(gb_system_state));
    xTaskCreate( &init_task, "init_task", 2048, NULL, 3, NULL );
}
