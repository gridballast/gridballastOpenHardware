/**
 * @file grid_ballast_main.c
 *
 * @brief main function that starts up the grid ballast system

 * @author Rohit Gaarg (rohitg1@andrew.cmu.edu)
 * @author Vikram Shanker (vshanker@cmu.edu)
 */

#include <stdio.h>
#include <string.h> /* memset */
#include "controller_module.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "sensing_module.h"

#include "system_state.h"
#include "wifi_module.h"
#include "lcd_module.h"
#include "frq_module.h"
#include "rs_485_module.h"
#include "ct_module.h"
#include "driver/timer.h"
#include "util.h"


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


    // temporary snippet to change thermostat set point
    rwlock_writer_lock(&system_state_lock);
    get_system_state(&gb_system_state);
    gb_system_state.set_point = 125;
    set_system_state(&gb_system_state);
    rwlock_writer_unlock(&system_state_lock);
    

    
    wifi_init_task();
    //sensing_init_task();
    //controller_init_task();

      
     //   frq_init_task();
      //  rs485_init_task();
        //ct_init_task();
      //  lcd_init_task();
        

    while(1);

    
}

/**
 * @brief main function that starts the initialization task
 *
 * @return void
 */
void app_main( void ) {
    printf("hit\n");
    nvs_flash_init();
    /* initialize gb_system_state to 0's */
    memset(&gb_system_state, 0, sizeof(gb_system_state));
    rwlock_init(&system_state_lock);
    printf("Intializing GridBallast system...\n");

    xTaskCreate( &init_task, "init_task", 4096, NULL, 1, NULL );
}

