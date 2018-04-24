#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <time.h>


#include "Ada_MCP.h" // IO Expander Library
#include "generic_rw_i2c.h"  // generic I2C read/write functions
#include "driver/adc.h"
#include "driver/timer.h"
#include "esp_spi_flash.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "rwlock.h"
#include "util.h"

/*work under progress*/
#define ESP_INTR_FLAG_DEFAULT 0

#define PIN_MCP_RESET 2
#define PIN_SDA 25
#define PIN_SCL 26
#define LEVEL_HIGH 1
#define LEVEL_LOW 0

#define I2C_MASTER_FREQ_HZ     100000     /* I2C master clock frequency */
// #define TAG "gridballast"
//uint8_t p,v;

#define TIMER_DIVIDER   80

system_state_t mystate;
// xQueueHandle adc_queue;
rwlock_t i2c_lock;


int button =0;
bool ledState = true;
int first = 0;
// void delay(int milli_seconds)
// {
    // Converting time into milli_seconds
    // int milli_seconds = 1000 * number_of_seconds;
 
//     // Stroing start time
//     clock_t start_time = clock();
 
//     // looping till required time is not acheived
//     while (clock() < start_time + milli_seconds)
//         ;
// }




void IRAM_ATTR mcp_isr_handler(void* arg) {


  button=1;
}

void mcp_task(void* arg) 
{

  while(1){

//     rwlock_writer_lock(&i2c_lock);      
//     begin(0);
//     digitalWrite(4,0);
//     //vTaskDelay(100 / portTICK_PERIOD_MS);
//     digitalWrite(4,1);
//     i2c_driver_delete(I2C_NUM_1);
//     rwlock_writer_unlock(&i2c_lock);  
   

// }
    
    if (button == 1){
      

    // rwlock_writer_lock(&i2c_lock);
    // printf("%s\n", "blah");
    // if(first == 1) {

  

    begin(0);

    // //   // gpio_pad_select_gpio(13);
    // //   // gpio_set_direction(13, GPIO_MODE_OUTPUT);


       // setupInterrupts(true,false, 0);               //IO expander button interrupt
       // pinMode(3,GPIO_MODE_INPUT);
       // pullUp(3,1);
       // setupInterruptPin(3,GPIO_INTR_NEGEDGE);

    // } else {
    //     first = 1;
    // //     //delay(500);
    // }
    vTaskDelay(500/portTICK_PERIOD_MS);

    uint8_t pin=getLastInterruptPin();
    uint8_t val=getLastInterruptPinValue();
    // printf("%u ",pin);
    // printf("%u \n", val );
     // Here either the button has been pushed or released.
    if ( pin ==3 && val == 0) 
        { //  Test for release - pin pulled high
        // if ( ledState ) {
        //    gpio_set_level(13, LEVEL_HIGH);
        // } else {
        //    gpio_set_level(13, LEVEL_LOW);
        // }

        //  ledState = ! ledState;

      // pinMode(8,GPIO_MODE_OUTPUT);       // LCD reset 

      // digitalWrite(8,0);

      //vTaskDelay(0.05 / portTICK_PERIOD_MS);

       //digitalWrite(8,1);
    
        button = 0;

        //printf("hello1\n");
        rwlock_writer_lock(&system_state_lock);
    get_system_state(&gb_system_state);
    gb_system_state.set_point ++ ;
    set_system_state(&gb_system_state);
    rwlock_writer_unlock(&system_state_lock);


        }
    // vTaskDelay(200/portTICK_PERIOD_MS);    
    // i2c_driver_delete(I2C_NUM_0);
    // vTaskDelay(500/portTICK_PERIOD_MS);
    // rwlock_writer_unlock(&i2c_lock);  
   }
   //printf("hello\n");
   // if ( ledState ) {
   //         gpio_set_level(13, LEVEL_HIGH);
   //      } else {
   //         gpio_set_level(13, LEVEL_LOW);
   //      }

   //       ledState = ! ledState;
     
   }


}



void button_init_task( void ) {


  gpio_pad_select_gpio(PIN_MCP_RESET);
  gpio_set_direction(PIN_MCP_RESET, GPIO_MODE_OUTPUT);
  gpio_set_level(PIN_MCP_RESET, LEVEL_HIGH);
  //generic_i2c_master_init (I2C_NUM_1, PIN_SCL, PIN_SDA, I2C_MASTER_FREQ_HZ);

  // gpio_pad_select_gpio(PIN_MCP_RESET);
  // gpio_set_direction(PIN_MCP_RESET, GPIO_MODE_OUTPUT);
  // gpio_set_level(PIN_MCP_RESET, 1);
  // rwlock_writer_lock(&i2c_lock);
  //generic_i2c_master_init (I2C_NUM_1, PIN_SCL, PIN_SDA, I2C_MASTER_FREQ_HZ);

  begin(0);

  pinMode(4,GPIO_MODE_OUTPUT); 
   digitalWrite(4,0);

  gpio_pad_select_gpio(13);
  gpio_set_direction(13, GPIO_MODE_OUTPUT);


  setupInterrupts(true,false, 0);               //IO expander button interrupt
  pinMode(3,GPIO_MODE_INPUT);
  pullUp(3,1);
  setupInterruptPin(3,GPIO_INTR_NEGEDGE);

  gpio_set_intr_type(4, GPIO_INTR_NEGEDGE);       //esp32 interrupt GPIO4
  //gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
  gpio_isr_handler_add(4, mcp_isr_handler, NULL);



  xTaskCreate(mcp_task, "mcp_task", 1024, NULL, 10, NULL);

  // pinMode(4,GPIO_MODE_OUTPUT);       // test o/p

  //   digitalWrite(4,0);
  //   vTaskDelay(500 / portTICK_PERIOD_MS);


}

