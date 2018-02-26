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
#include "util.h"

/* contains commented code which may be needed for further testing*/


#define ECHO_TEST_TXD  (17)
#define ECHO_TEST_RXD  (16)
#define ECHO_TEST_RTS  (22)
#define ECHO_TEST_CTS  UART_PIN_NO_CHANGE

#define BUF_SIZE (512)


system_state_t mystate;

int ret;
int currentSetpoint = 0;

const char * const rs485_task_name = "rs485_module_task";




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

    
    rwlock_reader_lock(&system_state_lock);
    get_system_state(&mystate);
    rwlock_reader_unlock(&system_state_lock);

    uint8_t setpoint = mystate.set_point;
    //uint8_t setpoint = 68;


    unsigned char slave_ok[5] = {0x07,0x01,0x03,0x04,0x0F};
    uint8_t* data = (uint8_t*) malloc(BUF_SIZE);



    unsigned char msg_poll_slave[2] = {0x87,0x00};
    unsigned char mtype2[4] = {0x40,0x09,0x14,0x00};

    unsigned char toptemp;
    unsigned char bottemp;
    int flag=0;
    unsigned char bytes[6] = { 0x87, 0x09, 0x03, setpoint, setpoint, 0x00};
    bytes[5] = calculate_checksum(bytes, 5);

    size_t buf_len;
    int len = 0;
    // int ret = 1;
    int breakFlag = 1;
     while(1)
     {
        //printf("temperature is %d\n",gb_system_state.temp_top);
        //uart_get_buffered_data_len(uart_num, &buf_len);
        //if(buf_len > 0) printf("-----buflen%d\n", buf_len);

        if(currentSetpoint != setpoint) {
            uart_flush(uart_num);
            len = uart_read_bytes(uart_num, data, BUF_SIZE, 20 / portTICK_RATE_MS);
            if (len > 0 && memcmp(msg_poll_slave, data, 2) == 0 && flag == 0) {
                sendData(bytes,6);

                flag = 1;
                currentSetpoint = setpoint;
            } else if(len > 0 && memcmp(msg_poll_slave, data, 2) == 0 && flag == 1){
                sendData(slave_ok,5);
            // printf("%s\n", );
            } 
        } else {
            uart_flush(uart_num);
            len = uart_read_bytes(uart_num, data, BUF_SIZE, 40 / portTICK_RATE_MS);
            // for(int i = 0; i < len; i++) {
            //     printf("%02x ", data[i]);

            // }

            if (len > 0 && memcmp(mtype2,data, 4) == 0){
                toptemp = data[15];
                bottemp = data[16];

                
                
                rwlock_writer_lock(&system_state_lock);
                get_system_state(&mystate);
                mystate.temp_top = (uint8_t)toptemp;
                mystate.temp_bottom = (uint8_t)bottemp;
                set_system_state(&mystate);
                rwlock_writer_unlock(&system_state_lock);
            

    
            //breakFlag = 0;

            }
        }


        
        
        // else if(len > 0) {
        //for(int i = 0; i < len && data[i] != NULL; i++) {
            //printf("%02x", data[i]);
        
        // 
        memset(data,0,BUF_SIZE);
        //ret = 0;


    }
}


void rs485_init_task()
{
    /* temporary testing
    get_system_state(&mystate);
    mystate.temp_top = 5;
    mystate.temp_bottom = 6;
    set_system_state(&mystate);
    */
    //ret = 1;
    xTaskCreate(rs485_task, "rs485_task", 1024, NULL, 10, NULL);
}


