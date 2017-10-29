/**
 * @file sensing_module.c
 *
 * @brief sensing module related functionality
 *
 * @author Vikram Shanker (vshanker@cmu.edu)
 */

#include <stdio.h>
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"

/** @brief functiont that contains sensing task logic */
static TaskFunction_t sensing_task_fn;

/*****************************************
 *********** INTERFACE FUNCTIONS *********
 *****************************************/

/**
 * @brief initialization task that starts all other threads
 *
 * @return void
 */
void sensing_init_task( void ) {

    printf("Intializing Sensing System...");
    xTaskCreate(
                &sensing_task_fn, /* task function */
                sensing_task_name, /* wifi task name */
                sensingUSStackDepth, /* stack depth */
                NULL, /* parameters to fn_name */
                sensingUXPriority, /* task priority */
                NULL /* task handle ( returns an id basically ) */
               );
    fflush(stdout);
}

/*****************************************
 ************ MODULE FUNCTIONS ***********
 *****************************************/

/**
 * @brief sensing task logic
 *
 * @param pv_parameters - parameters for task being create (should be NULL)
 *
 * @return void
 */
static void sensing_task_fn( void *pv_parameters ) {
  printf(" done (sensing)!\n");
  return;
}
