/**
 * @file grid_ballast_main.c
 *
 * @brief main function that starts up the grid ballast system

 * @author Rohit Gaarg (rohitg1)
 * @author Vikram Shanker (vshanker@cmu.edu)
 */

#include <stdio.h>
#include <string.h> /* memset */
#include "rwlock.h"

#include "controller_module.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "sensing_module.h"

#include "Ada_MCP.h" // IO Expander Library
#include "generic_rw_i2c.h"  // generic I2C read/write functions

#include "system_state.h"
#include "wifi_module.h"
#include "lcd_module.h"
#include "frq_module.h"
#include "rs_485_module.h"
#include "button.h"
#include "controller_module.h"
#include "ct_module.h"
#include "driver/timer.h"
#include "util.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"

bool State = true;


#define ESP_INTR_FLAG_DEFAULT 0

system_state_t gb_system_state;
rwlock_t system_state_lock;
rwlock_t i2c_lock;


#define PIN_MCP_RESET 2                   // I/O expander reset pin

#define PIN_SDA 25                        /*i2c pins */
#define PIN_SCL 26                       
#define I2C_MASTER_FREQ_HZ     100000     /* I2C master clock frequency */

#define LEVEL_HIGH 1
#define LEVEL_LOW 0


/**
 * @brief initialization task that starts all other threads
 *
 * @param pv_parameters - parameters for task being created (should be NULL)
 *
 * @return void
 */




void init_task( void *pv_parameters ) {

    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    

    // temporary snippet to change thermostat set point
    rwlock_writer_lock(&system_state_lock);
    get_system_state(&gb_system_state);
    gb_system_state.set_point = 100;
    gb_system_state.threshold_overfrq = 60.01;
    gb_system_state.threshold_underfrq = 59.99;
    gb_system_state.mode =0;
    set_system_state(&gb_system_state);
    rwlock_writer_unlock(&system_state_lock);
    

    
    //wifi_init_task();
    //sensing_init_task();
    controller_init_task();

    

     printf("Initializing frq\n");
     frq_init_task();
     printf("Initializing rs485\n");

     button_init_task();

     printf("Initializing lcd\n");
     lcd_init_task();
        
     printf("Initialization done\n");

    
     rs485_init_task();
// 
      

      //ct_init_task();
       
        //vTaskDelay(500/portTICK_PERIOD_MS);

        while(1);
       
}
    


/**
 * @brief main function that starts the initialization task
 *
 * @return void
 */



void app_main( void ) 
{
    
    nvs_flash_init();
    /* initialize gb_system_state to 0's */
    memset(&gb_system_state, 0, sizeof(gb_system_state));
    rwlock_init(&system_state_lock);
    rwlock_init(&i2c_lock);
    printf("Intializing GridBallast system...\n");


    xTaskCreate( &init_task, "init_task", 4096, NULL, 1, NULL);
    

    gpio_pad_select_gpio(PIN_MCP_RESET);
    gpio_set_direction(PIN_MCP_RESET, GPIO_MODE_OUTPUT);
    gpio_set_level(PIN_MCP_RESET, 1);
  // // rwlock_writer_lock(&i2c_lock);
   generic_i2c_master_init (I2C_NUM_1, PIN_SCL, PIN_SDA, I2C_MASTER_FREQ_HZ);

    begin(0);

    pinMode(8,GPIO_MODE_OUTPUT);       // test o/p

    digitalWrite(8,0);

    vTaskDelay(0.05 / portTICK_PERIOD_MS);

    digitalWrite(8,1);

 //Characterize ADC at particular atten
    // esp_adc_cal_characteristics_t *adc_chars = calloc(1, sizeof(esp_adc_cal_characteristics_t));
    // esp_adc_cal_value_t val_type = esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_9, 1100, adc_chars);
    // //Check type of calibration value used to characterize ADC
    // if (val_type == ESP_ADC_CAL_VAL_EFUSE_VREF) {
    //     printf("eFuse Vref");
    // } else if (val_type == ESP_ADC_CAL_VAL_EFUSE_TP) {
    //     printf("Two Point");
    // } else {
    //     printf("Default");
    // }

    // while(1)
    // {

    // adc2_vref_to_gpio(26);

    // uint32_t reading =  adc1_get_raw(ADC1_CHANNEL_0);
    // uint32_t voltage = esp_adc_cal_raw_to_voltage(reading, adc_chars);

    // printf("%u\n",voltage);

    // vTaskDelay(2000 / portTICK_PERIOD_MS);
 

//}

}

