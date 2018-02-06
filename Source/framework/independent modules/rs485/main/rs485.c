
        
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

/**
 * This is a example exaple which echos any data it receives on UART1 back to the sender, with hardware flow control
 * turned on. It does not use UART driver event queue.
 *
 * - port: UART1
 * - rx buffer: on
 * - tx buffer: off
 * - flow control: on
 * - event queue: off
 * - pin assignment: txd(io4), rxd(io5), rts(18), cts(19)
 */

#define ECHO_TEST_TXD  (17)
#define ECHO_TEST_RXD  (16)
#define ECHO_TEST_RTS  (22)
#define ECHO_TEST_CTS  UART_PIN_NO_CHANGE

#define BUF_SIZE (512)

void delay(int milli_seconds)
{
    // Converting time into milli_seconds
    // int milli_seconds = 1000 * number_of_seconds;
 
    // Stroing start time
    clock_t start_time = clock();
 
    // looping till required time is not acheived
    while (clock() < start_time + milli_seconds)
        ;
}

int checkParity(uint8_t* t)
{
    int noofones = 0;
    uint8_t mask = 0x0001; /* start at first bit */

    while(mask != 0) /* until all bits tested */
    {
        if(mask & t[0])  /*if bit is 1, increment noofones */
        {
            noofones++;
        }
        mask = mask << 1; /* go to next bit */
    }

    /* if noofones is odd, least significant bit will be 1 */

    return (noofones & 1);
}
int max(int x, int y) {
    if (x > y) {
        return x;
    }
    return y;
}

unsigned char calculate_checksum(unsigned char* buf, int len) {
    unsigned char sum = 0;
    size_t index = 0;
    for (; (len>0) && (index < (len)); index++) {
        sum += buf[index];
    }
    return sum;
}
void init(uart_parity_t p) {
    const int uart_num = UART_NUM_2;

   uart_config_t even = {
        .baud_rate = 19200,
        .data_bits = UART_DATA_8_BITS,
        .parity = p,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .rx_flow_ctrl_thresh = 122,
    };
    //Configure UART1 parameters
    uart_param_config(uart_num, &even);
    //Set UART1 pins(TX: IO4, RX: I05, RTS: IO18, CTS: IO19)
    uart_set_pin(uart_num, ECHO_TEST_TXD, ECHO_TEST_RXD, ECHO_TEST_RTS, ECHO_TEST_CTS);
    //Install UART driver (we don't need an event queue here)
    //In this example we don't even use a buffer for sending data.
   
    uart_set_rs485_hd_mode(uart_num, true);

    uart_driver_install(uart_num, BUF_SIZE * 2, 0, 0, NULL, 0);
    //uart_set_rs485_hd_mode(uart_num, true);

}
void sendData(unsigned char* bytes, int len) {
    // int parity = checkParity(bytes);
    int uart_num = UART_NUM_2;
    // uart_parity_t par;
    // uart_get_parity(uart_num, &par);
    // if(parity == 0) {
    //     if(par == UART_PARITY_EVEN) {
    //         uart_driver_delete(uart_num);
    //         init(UART_PARITY_ODD);
    //     }
   

    // } else {
    //     if(par == UART_PARITY_ODD) {
    //         uart_driver_delete(uart_num);
    //         delay(250);
    //         //esp_deep_sleep(3000);
    //         init(UART_PARITY_EVEN);
    //     }// uart_set_parity(uart_num, UART_PARITY_EVEN);
        // uart_driver_install(uart_num, BUF_SIZE * 2, 0, 0, NULL, 0);
// 
    // }
    uart_write_bytes(uart_num, (const char*)bytes, 1);


    for(int i = 1; i < len; i++) {
        // parity = checkParity(bytes + i);
        // uart_get_parity(uart_num, &par);

        // if(parity == 0) {
        //     if(par == UART_PARITY_ODD) {
        //         uart_driver_delete(uart_num);
        //         //esp_deep_sleep(3000);
        //         delay(250);
        //         init(UART_PARITY_EVEN);
        //     }
        //     // uart_set_parity(uart_num, UART_PARITY_EVEN);
        //     // uart_driver_install(uart_num, BUF_SIZE * 2, 0, 0, NULL, 0);

        // } 
        // else {
        // // uart_set_parity(uart_num, UART_PARITY_ODD);
        //     if(par == UART_PARITY_EVEN) {
        //         uart_driver_delete(uart_num);
        //         //esp_deep_sleep(3000);
        //         delay(250);
        //         init(UART_PARITY_ODD);
        //     }
        //         // uart_driver_install(uart_num, BUF_SIZE * 2, 0, 0, NULL, 0);

        // }
        uart_write_bytes(uart_num, (const char*)bytes+i, 1);

    }

}
//an example of echo test with hardware flow control on UART1
static void echo_task()
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

    // setUartNum_0()

    //Configure UART1 parameters
    uart_param_config(uart_num, &uart_config);
    //Set UART1 pins(TX: IO4, RX: I05, RTS: IO18, CTS: IO19)
    uart_set_pin(uart_num, ECHO_TEST_TXD, ECHO_TEST_RXD, ECHO_TEST_RTS, ECHO_TEST_CTS);
    //Install UART driver (we don't need an event queue here)
    //In this example we don't even use a buffer for sending data.

    
    uart_set_rs485_hd_mode(uart_num, true);

    uart_driver_install(uart_num, BUF_SIZE * 2, 0, 0, NULL, 0);
    //int len = 1;
   
   
    uint8_t setpoint = 105;


    unsigned char slave_ok[5] = {0x07,0x01,0x03,0x04,0x0F};
                    // uart_set_rs485_hd_mode(uart_num, true);
    uint8_t* data = (uint8_t*) malloc(BUF_SIZE);

   
    // fflush(stdout);

    unsigned char msg_poll_slave[2] = {0x87,0x00};
    unsigned char mtype2[4] = {0x40,0x09,0x14,0x00};

    unsigned char toptemp;
    unsigned char bottemp;
    int flag=0;
    unsigned char bytes[6] = { 0x87, 0x09, 0x03, setpoint, setpoint, 0x00};
    bytes[5] = calculate_checksum(bytes, 5);

     while(1) 
     { 
    //Write data back to UART
        //     int parity = checkParity(setpoint);
        //     uart_parity_t par;
        //     uart_get_parity(uart_num, &par);
        //     if(parity == 0) {
        //         if(par == UART_PARITY_EVEN) {
        //             uart_driver_delete(uart_num);
        //             init(UART_PARITY_ODD);
        //         }
        //         //uart_set_parity(uart_num, UART_PARITY_ODD);

        //         // uart_driver_install(uart_num, BUF_SIZE * 2, 0, 0, NULL, 0);

        //     } else {
        //         if(par == UART_PARITY_ODD) {
        //             uart_driver_delete(uart_num);
        //             delay(300);
        //             init(UART_PARITY_EVEN);
        //         }// uart_set_parity(uart_num, UART_PARITY_EVEN);
        //         // uart_driver_install(uart_num, BUF_SIZE * 2, 0, 0, NULL, 0);

        //     }
        // uart_write_bytes(uart_num, setpoint, 1);

        
        
        int len = uart_read_bytes(uart_num, data, BUF_SIZE, 20 / portTICK_RATE_MS);


       
        //printf("%s%d\n", "read from ET and length is : ", 0);
        //fflush(stdout)
        // for(int i=0;i<len;i++){
        //     printf("%2.2X\n", data[i]);
        //     fflush(stdout);
        // }
        if (len > 0 && memcmp(mtype2,data, 4) == 0){
            toptemp = data[15];
        
            bottemp = data[16];
            //for(int i=0;i<len;i++){
              //  printf("%2.2x ", data[i]);

            //}
            //printf("%s\n", "blah");
            //printf("%2.2x\n",toptemp);

            //printf("%2.2x\n",bottemp);
                
        
        }

        if (len > 0 && memcmp(msg_poll_slave, data, 2) == 0 && flag == 0) {
            //printf("# Got Poll for Slave\n");
        
          
            
            // int parity = checkParity(bytes);
            // uart_parity_t par;
            // uart_get_parity(uart_num, &par);
            // if(parity == 0) {
            //     if(par == UART_PARITY_EVEN) {
            //         uart_driver_delete(uart_num);
            //         init(UART_PARITY_ODD);
            //     }
            //     // uart_set_parity(uart_num, UART_PARITY_ODD);

            //     // uart_driver_install(uart_num, BUF_SIZE * 2, 0, 0, NULL, 0);

            // } else {
            //     if(par == UART_PARITY_ODD) {
            //         uart_driver_delete(uart_num);
            //         delay(300);
            //         init(UART_PARITY_EVEN);
            //     }// uart_set_parity(uart_num, UART_PARITY_EVEN);
            //     // uart_driver_install(uart_num, BUF_SIZE * 2, 0, 0, NULL, 0);

            // }
            // uart_write_bytes(uart_num, (const char*)bytes, 1);


            // for(int i = 1; i <=6; i++) {
            //     parity = checkParity(bytes + i);
            //     uart_get_parity(uart_num, &par);

            //     if(parity == 0) {
            //         if(par == UART_PARITY_ODD) {
            //             uart_driver_delete(uart_num);
            //             delay(300);
            //             init(UART_PARITY_EVEN);
            //         }
            //         // uart_set_parity(uart_num, UART_PARITY_EVEN);
            //         // uart_driver_install(uart_num, BUF_SIZE * 2, 0, 0, NULL, 0);

            //     } 
            //     else {
            //         // uart_set_parity(uart_num, UART_PARITY_ODD);
            //         if(par == UART_PARITY_EVEN) {
            //             uart_driver_delete(uart_num);
            //             delay(300);
            //             init(UART_PARITY_ODD);
            //         }
            //         // uart_driver_install(uart_num, BUF_SIZE * 2, 0, 0, NULL, 0);

            //     }
            //     uart_write_bytes(uart_num, (const char*)bytes+i, 1);

            // }
            // if(len > 1) {
                //unsigned char lol[4] = {1,2,3};
                sendData(bytes,6);
                flag = 1;
            }

        else if(len > 0 && memcmp(msg_poll_slave, data, 2) == 0 && flag == 1){
           
            sendData(slave_ok,5);
        }
    //         // int length = max(len1,len2);
    //         // int32_t x = (int32_t)(*data1);
    //         // int32_t y = (int32_t)(*data2);
    //         // int z = (*data1) + (*data2);
    //         // for(int i = 0; i < 4; i++) {
    //             // sum[i] = data1[i] + data2[i];

    //         // }
    //         // uart_write_bytes(uart_num, (const char*)sum, 4);

    //     //}

    }
}

void app_main()
{
    //A uart read/write example without event queue;
    xTaskCreate(echo_task, "uart_echo_task", 1024, NULL, 10, NULL);
}

   