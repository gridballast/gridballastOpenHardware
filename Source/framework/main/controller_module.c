/**
 * @file controller_module.c
 *
 * @brief controller module related functionality
 *
 * @author Vikram Shanker (vshanker@cmu.edu)
 */

#include <stdio.h>
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"

/** @brief controller task logic function */
static TaskFunction_t controller_task_fn;

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
    xTaskCreate(
                &controller_task_fn, /* task function */
                controller_task_name, /* controller task name */
                controllerUSStackDepth, /* stack depth */
                NULL, /* parameters to fn_name */
                controllerUXPriority, /* task priority */
                NULL /* task handle ( returns an id basically ) */
               );
    fflush(stdout);
}

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
static void controller_task_fn( void *pv_parameters ) {
  printf(" done!\n");
  return;
}
