/**
 * @file grid_ballast_main.c
 *
 * @brief main function that starts up the grid ballast system
 *
 * @author Vikram Shanker (vshanker@cmu.edu)
 */

#include <stdio.h>
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"

/**
 * @brief initialization task that starts all other threads
 *
 * @param pv_parameters - parameters for task being created (should be NULL)
 *
 * @return void
 */
void init_task( void *pv_parameters )
{
    printf("Intializing GridBallast system...\n");
    fflush(stdout);
}

/**
 * @brief main function that starts the initialization task
 *
 * @return void
 */
void app_main( void )
{
    nvs_flash_init( void );
    xTaskCreate( &init_task, "init_task", 2048, NULL, 3, NULL );
}
