
        
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "driver/uart.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "soc/uart_struct.h"



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

static void rs485_task()
{
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
    
    uart_set_pin(uart_num, ECHO_TEST_TXD, ECHO_TEST_RXD, ECHO_TEST_RTS, ECHO_TEST_CTS);
   

    
    uart_set_rs485_hd_mode(uart_num, true);

    uart_driver_install(uart_num, BUF_SIZE * 2, 0, 0, NULL, 0);
    //int len = 1;
   
   
    uint8_t setpoint = 68;


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
            toptemp = data[15];
        
            bottemp = data[16];
  
        
        }

        if (len > 0 && memcmp(msg_poll_slave, data, 2) == 0 && flag == 0) {

                sendData(bytes,6);
                flag = 1;
            }

        else if(len > 0 && memcmp(msg_poll_slave, data, 2) == 0 && flag == 1){
           
            sendData(slave_ok,5);
        }

    }
}

void app_main()
{
 
    xTaskCreate(rs485_task, "rs485_task", 1024, NULL, 10, NULL);
}

   