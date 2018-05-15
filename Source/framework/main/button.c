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

#define TIMER_DIVIDER   80

system_state_t mystate;

rwlock_t i2c_lock;


int button =0;
bool ledState = true;
int first = 0;



void IRAM_ATTR mcp_isr_handler(void* arg) {


  button=1;
}

void mcp_task(void* arg) 
{

  while(1){


    
    if (button == 1){
  
  

    begin(0);

    vTaskDelay(300/portTICK_PERIOD_MS);

    uint8_t pin=getLastInterruptPin();
    uint8_t val=getLastInterruptPinValue();

    // printf("%u ",pin);
    // printf("%u \n", val );
     
     // Here either the button has been pushed or released.
    if ( pin == 0 && val == 0) 
        { //  Test for release - pin pulled high
     
            button = 0;


        rwlock_reader_lock(&system_state_lock);
        get_system_state(&mystate);
        rwlock_reader_unlock(&system_state_lock);

          
          if ( mystate.mode == 0)
          {
            rwlock_writer_lock(&system_state_lock);
            get_system_state(&gb_system_state);
            gb_system_state.set_point ++ ;
            set_system_state(&gb_system_state);
            rwlock_writer_unlock(&system_state_lock);
          }


        }

        if( pin == 2 && val == 0)
        {
            button = 0;

          if ( mystate.mode == 0)
          {
                rwlock_writer_lock(&system_state_lock);
                get_system_state(&gb_system_state);
                gb_system_state.set_point -- ;
                set_system_state(&gb_system_state);
                rwlock_writer_unlock(&system_state_lock);
           }

        }

         if( pin == 1 && val == 0)
        {
            button = 0;
            rwlock_writer_lock(&system_state_lock);
        get_system_state(&gb_system_state);
        gb_system_state.mode = 0;
        set_system_state(&gb_system_state);
        rwlock_writer_unlock(&system_state_lock);

        }

         if( pin == 3 && val == 0)
        {
            button = 0;

          
        rwlock_writer_lock(&system_state_lock);
        get_system_state(&gb_system_state);
        gb_system_state.mode = 1;
        set_system_state(&gb_system_state);
        rwlock_writer_unlock(&system_state_lock);

        }


       }
        vTaskDelay(500/portTICK_PERIOD_MS);
         
       }


}



void button_init_task( void ) {

  //  I/O expander reset
  gpio_pad_select_gpio(PIN_MCP_RESET);
  gpio_set_direction(PIN_MCP_RESET, GPIO_MODE_OUTPUT);
  gpio_set_level(PIN_MCP_RESET, LEVEL_HIGH);

  begin(0);

  pinMode(4,GPIO_MODE_OUTPUT); 
   digitalWrite(4,0);

  gpio_pad_select_gpio(13);
  gpio_set_direction(13, GPIO_MODE_OUTPUT);


   //I/O expander interrupt initialization
  setupInterrupts(true,false, 0);              
  pinMode(3,GPIO_MODE_INPUT);
  pullUp(3,1);
  setupInterruptPin(3,GPIO_INTR_NEGEDGE);

  pinMode(2,GPIO_MODE_INPUT);
  pullUp(2,1);
  setupInterruptPin(2,GPIO_INTR_NEGEDGE);

  pinMode(1,GPIO_MODE_INPUT);
  pullUp(1,1);
  setupInterruptPin(1,GPIO_INTR_NEGEDGE);

  pinMode(0,GPIO_MODE_INPUT);
  pullUp(0,1);
  setupInterruptPin(0,GPIO_INTR_NEGEDGE);

  //esp32 interrupt initialization on GPIO4
  gpio_set_intr_type(4, GPIO_INTR_NEGEDGE);       
  gpio_isr_handler_add(4, mcp_isr_handler, NULL);



  xTaskCreatePinnedToCore(mcp_task, "mcp_task", 1024, NULL, 7, NULL,0);




}

