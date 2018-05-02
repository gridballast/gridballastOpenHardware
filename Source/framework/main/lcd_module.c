#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "Ada_MCP.h" // IO Expander Library
#include "driver/adc.h"
#include "generic_rw_i2c.h"  // generic I2C read/write functions
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "driver/timer.h"
#include "esp_log.h"
#include "esp_spi_flash.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "rwlock.h"
#include "u8g2.h" // LCD driver library
#include "u8g2_esp32_hal.h" // ESP32 HAL library for u8g2
#include "util.h"  

#include "button.h"

#define PIN_MCP_RESET 2
#define PIN_SDA 25
#define PIN_SCL 26
#define LEVEL_HIGH 1
#define _I2C_MASTER_FREQ_HZ     100000     /* I2C master clock frequency */
#define TAG "gridballast"

system_state_t mystate;


// uint8_t temprature_sens_read(); 

static void task_lcd(void *arg) {

  // u8g2_esp32_hal_t u8g2_esp32_hal = U8G2_ESP32_HAL_DEFAULT;
  // u8g2_esp32_hal.sda = PIN_SDA;
  // u8g2_esp32_hal.scl = PIN_SCL;
  // u8g2_esp32_hal_init(u8g2_esp32_hal);

  // // a structure which will contain all the data for one display
  // u8g2_t u8g2;

  // // initialize u8g2 structure
  // u8g2_Setup_ssd1309_i2c_128x64_noname0_f(&u8g2, U8G2_R0, u8g2_esp32_i2c_byte_cb, u8g2_esp32_gpio_and_delay_cb);
  // u8x8_SetI2CAddress(&u8g2.u8x8, 0x78);

  
  // // send init sequence to the display, display is in sleep mode after this,
  // u8g2_InitDisplay(&u8g2);
  // //wake up display
  // u8g2_SetPowerSave(&u8g2, 0);
  // u8g2_SetContrast(&u8g2, 100);
  // u8g2_SetFlipMode(&u8g2, 1);


  float freq = 0;
  float pwr = 0;
  int t1 = 0;
  int t2 = 0;
  int m =0;
  char str [32];

  u8g2_t u8g2;

  // initialize u8g2 structure
  u8g2_Setup_ssd1309_i2c_128x64_noname0_f(&u8g2, U8G2_R0, u8g2_esp32_i2c_byte_cb, u8g2_esp32_gpio_and_delay_cb);
  u8x8_SetI2CAddress(&u8g2.u8x8, 0x78);

  
  // send init sequence to the display, display is in sleep mode after this,
  u8g2_InitDisplay(&u8g2);
  //wake up display
  u8g2_SetPowerSave(&u8g2, 0);
  u8g2_SetContrast(&u8g2, 100);
  u8g2_SetFlipMode(&u8g2, 1);
  //printf("hi\n");

while(1)
  {
    // read system state to access state variables for display

    rwlock_reader_lock(&system_state_lock);
    get_system_state(&mystate);
    rwlock_reader_unlock(&system_state_lock);

    freq = mystate.grid_freq;
    t1 = mystate.temp_top;
    t2 = mystate.temp_bottom;

    //printf("%s\n","pwr" );
    pwr = mystate.power;

    m = mystate.mode;

    //printf("%s\n","pwr" );

    int sp = mystate.set_point;
  //ESP_LOGI("lcd", "the t bottom is %d\n",t2);
  rwlock_writer_lock(&i2c_lock);

  // a structure which will contain all the data for one display
   u8x8_SetI2CAddress(&u8g2.u8x8, 0x78);

    u8g2_ClearBuffer(&u8g2);


    u8g2_SetFont(&u8g2,u8g2_font_t0_13_te);
      sprintf(str, "Freq: %2.4fHz", freq);

    u8g2_DrawStr(&u8g2, 10, 10, str);
  
    sprintf(str,"Tt:%dF",t1);
    u8g2_DrawStr(&u8g2, 10, 60, str);
  
    sprintf(str,"Tb:%dF",t2);
    u8g2_DrawStr(&u8g2, 70, 60, str);
  
    sprintf(str,"Ts:%dF",sp);
    u8g2_DrawStr(&u8g2, 10, 40, str);

   
    sprintf(str,"Mode:%d ",m);
    u8g2_DrawStr(&u8g2, 70, 40, str);
    

    sprintf(str,"Power: %3.2f", pwr);
    u8g2_DrawStr(&u8g2, 10, 25, str);


    
    u8g2_SendBuffer(&u8g2);

    rwlock_writer_unlock(&i2c_lock);
    // ESP_LOGD(TAG, "all done");
    //vTaskDelay(500/portTICK_PERIOD_MS);
     //i2c_driver_delete(I2C_NUM_1);
         //vTaskDelay(500/portTICK_PERIOD_MS);

    
    
     //printf("hi 2\n");
    

    // i2c_driver_delete(I2C_NUM_1);

    //button_init_task();

     //printf("ESP32 onchip Temperature = %d\n", temprature_sens_read());

  }
   //
}

void lcd_init_task( void ) {


  // gpio_pad_select_gpio(PIN_MCP_RESET);
  // gpio_set_direction(PIN_MCP_RESET, GPIO_MODE_OUTPUT);
  // gpio_set_level(PIN_MCP_RESET, LEVEL_HIGH);
  // generic_i2c_master_init (I2C_NUM_0, PIN_SCL, PIN_SDA, I2C_MASTER_FREQ_HZ);


  // begin(0);

   // pinMode(4,GPIO_MODE_OUTPUT);       // LCD reset 

   // digitalWrite(4,1);

  // vTaskDelay(0.05 / portTICK_PERIOD_MS);

  // digitalWrite(8,1);
  

  //initialize I2C communication with IO expander
 printf("hit\n");

	xTaskCreate(task_lcd, "lcd_task", 4096, NULL, 10, NULL);
}
