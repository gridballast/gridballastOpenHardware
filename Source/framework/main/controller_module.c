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

  system_state_t * mystate;
  
  
  esp_task_wdt_feed();

    const int uart_num = UART_NUM_2;
    uart_config_t uart_config = {
        .baud_rate = 19200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_EVEN,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .rx_flow_ctrl_thresh = 122,
    };


    //Configure UART1 parameters
    uart_param_config(uart_num, &uart_config);
    //Set UART1 pins(TX: IO4, RX: I05, RTS: IO18, CTS: IO19)
    uart_set_pin(uart_num, ECHO_TEST_TXD, ECHO_TEST_RXD, ECHO_TEST_RTS, ECHO_TEST_CTS);
    //Install UART driver (we don't need an event queue here)
    //In this example we don't even use a buffer for sending data.

    
    uart_set_rs485_hd_mode(uart_num, true);

    uart_driver_install(uart_num, BUF_SIZE * 2, 0, 0, NULL, 0);
    //int len = 1;
   
    get_system_state(mystate);
    uint8_t setpoint = mystate->set_point;


    unsigned char slave_ok[5] = {0x07,0x01,0x03,0x04,0x0F};
                    // uart_set_rs485_hd_mode(uart_num, true);
    uint8_t* data = (uint8_t*) malloc(BUF_SIZE);



    unsigned char msg_poll_slave[2] = {0x87,0x00};
    unsigned char mtype2[4] = {0x40,0x09,0x14,0x00};

    unsigned char toptemp;
    unsigned char bottemp;
    int flag=0;
    unsigned char bytes[6] = { 0x87, 0x09, 0x03, setpoint, setpoint, 0x00};
    bytes[5] = calculate_checksum(bytes, 5);

     while(1) 
     { 

        int len = uart_read_bytes(uart_num, data, BUF_SIZE, 20 / portTICK_RATE_MS);


      
        if (len > 0 && memcmp(mtype2,data, 4) == 0){
            mystate -> temp_top = data[15];
        
            mystate -> temp_bottom = data[16];
     
            set_system_state(mystate);    
        
        }

        if (len > 0 && memcmp(msg_poll_slave, data, 2) == 0 && flag == 0) {
                sendData(bytes,6);
                flag = 1;
        }
        else if(len > 0 && memcmp(msg_poll_slave, data, 2) == 0 && flag == 1){
           
            sendData(slave_ok,5);
        }

    }
  return;
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
