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


const char * const controller_task_name = "controller_module_task";

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

#define ECHO_TEST_TXD  (17)
#define ECHO_TEST_RXD  (16)
#define ECHO_TEST_RTS  (22)
#define ECHO_TEST_CTS  UART_PIN_NO_CHANGE

#define BUF_SIZE (512)




unsigned char calculate_checksum(unsigned char* buf, int len) {
    unsigned char sum = 0;
    size_t index = 0;
    for (; (len>0) && (index < (len)); index++) {
        sum += buf[index];
    }
    return sum;
}


void sendData(unsigned char* bytes, int len) {
    // int parity = checkParity(bytes);
    int uart_num = UART_NUM_2;

    uart_write_bytes(uart_num, (const char*)bytes, 1);


    for(int i = 1; i < len; i++) {
        uart_write_bytes(uart_num, (const char*)bytes+i, 1);
    }

}




static void controller_task_fn( void *pv_parameters ) {
    while(1);
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
