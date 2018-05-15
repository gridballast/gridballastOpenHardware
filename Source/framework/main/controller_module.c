/**
 * @file controller_module.c
 *
 * @brief controller module related functionality
 *
 * @author Vikram Shanker (vshanker@cmu.edu)
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "controller_module.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "soc/uart_struct.h"
#include "util.h"
#include "rwlock.h"


const char * const controller_task_name = "controller_module_task";

system_state_t mystate;

/*****************************************
 ************ MODULE FUNCTIONS ***********
 *****************************************/

/**
 * @brief controller task logic
 *
 * @param pv_parameters - parameters for task being create (should be NULL)
 *
 * @return void
 */
static void controller_task_fn( void *pv_parameters ) 
{
    while(1)
    {
    rwlock_reader_lock(&system_state_lock);
    get_system_state(&mystate);
    // gb_system_state.set_point = 130;
    // set_system_state(&gb_system_state);
    rwlock_reader_unlock(&system_state_lock);

    //if ( strcmp(mystate.mode,"E")== 0)

    //printf("bye...\n");
     if ( mystate.mode == 1)
      {

        if (mystate.grid_freq > mystate.threshold_overfrq)
        {
            rwlock_writer_lock(&system_state_lock);
            get_system_state(&gb_system_state);
            gb_system_state.set_point = 140 ;
            set_system_state(&gb_system_state);
            rwlock_writer_unlock(&system_state_lock);
        }

        if (mystate.grid_freq < mystate.threshold_underfrq)
        {
            rwlock_writer_lock(&system_state_lock);
            get_system_state(&gb_system_state);
            gb_system_state.set_point = 110 ;
            set_system_state(&gb_system_state);
            rwlock_writer_unlock(&system_state_lock);
        }

    }
    vTaskDelay(500/portTICK_PERIOD_MS);
}

}



/*****************************************
 *********** INTERFACE FUNCTIONS *********
 *****************************************/

/**
 * @brief intializes the controller task
 *
 * @return void
 */
void controller_init_task( void ) {

    printf("Intializing Controlling System...");
    xTaskCreatePinnedToCore(
                controller_task_fn, /* task function */
                "controller_task_fn", /* controller task name */
                2048, /* stack depth */
                NULL, /* parameters to fn_name */
                6, /* task priority */
                NULL,0 /* task handle ( returns an id basically ) */
               );
    //fflush(stdout);
}
