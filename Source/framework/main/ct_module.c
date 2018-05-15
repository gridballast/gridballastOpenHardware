#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>


#include "Ada_MCP.h" // IO Expander Library
#include "generic_rw_i2c.h"  // generic I2C read/write functions
#include "driver/adc.h"
#include "driver/timer.h"
#include "esp_spi_flash.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "util.h"

/*work under progress*/
#define ESP_INTR_FLAG_DEFAULT 0

#define TAG "gridballast"
//uint8_t p,v;

#define TIMER_DIVIDER   80

system_state_t mystate;

rwlock_t i2c_lock;

const char * const ct_task_name = "ct_module_task";

static intr_handle_t s_timer_handle;
// xQueueHandle adc_queue;


// int flag =0;
// bool ledState = true;


int timr_group = TIMER_GROUP_1;
int timr_idx = TIMER_1;
int i=0;
float adc_val[10];
int ct_flag = 1;

void IRAM_ATTR timer_group1_isr(void *param) {
	TIMERG1.int_clr_timers.t1 = 1;
	TIMERG1.hw_timer[timr_idx].config.alarm_en = 1;
	ct_flag=1;
}
  
int sum = 0;
float curr = 0.0;

static void adc_task(void* arg) {
  
  while(1) {

    	if (ct_flag == 1) {
        //timer_disable_intr(timr_group,timr_idx);
    		adc_val[i] = (adc1_get_voltage(ADC1_CHANNEL_0) * 0.00087) - 1.64;
        //printf("i %d\n", i );
    		adc_val[i] = adc_val[i]*adc_val[i];
        sum += adc_val[i];
    		i++;
    	  ct_flag = 0;
        //timer_enable_intr(timr_group,timr_idx);
    	}
    	 if(i == 10) {
    	//if( ct_flag == 1) {
    		// for(int j=0; j<50; j++) {
    		// 	sum += adc_val[j];
    		// }
        // ct_flag = 0;
        //printf("Yo %d\n",i );
    		curr = sqrt(sum/i*1.0);       //RMS current
        
        
      // printf("current is %f\n",  curr);


        rwlock_writer_lock(&system_state_lock);
        get_system_state(&mystate);
        mystate.power = adc1_get_voltage(ADC1_CHANNEL_0);
        set_system_state(&mystate);
        rwlock_writer_unlock(&system_state_lock);

        //printf("current s %f\n",  mystate.power);
        sum = 0;

        i=0;

        
    		
    	}
      vTaskDelay(500/portTICK_PERIOD_MS);
  }
}

static void relay_task(void* arg)
{
  while(1)
  {
     rwlock_reader_lock(&system_state_lock);
    get_system_state(&mystate);
    rwlock_reader_unlock(&system_state_lock);

    if (mystate.set_point > 127)
    {
      rwlock_writer_lock(&i2c_lock);  
      begin(0);
      pinMode(4,GPIO_MODE_OUTPUT); 
      digitalWrite(4,1);
      rwlock_writer_unlock(&i2c_lock);  
    }
  }
}


void ct_init_task( void ) {
  adc1_config_width(ADC_WIDTH_9Bit);
  adc1_config_channel_atten(ADC1_CHANNEL_0,ADC_ATTEN_11db);
  timer_config_t config;
  config.alarm_en = 1;
  config.auto_reload = 1;
  config.counter_dir = TIMER_COUNT_UP;
  config.divider = TIMER_DIVIDER;
  config.intr_type = TIMER_INTR_LEVEL;
  config.counter_en = false;
  timer_init(timr_group, timr_idx, &config);
  timer_set_alarm_value(timr_group, timr_idx, 5000);
  timer_enable_intr(timr_group, timr_idx);
  timer_isr_register(timr_group, timr_idx, &timer_group1_isr, NULL, 0, &s_timer_handle);
  timer_start(timr_group, timr_idx);
  xTaskCreatePinnedToCore(adc_task, "adc_task", 1024, NULL, 10, NULL,0);

  //xTaskCreate(relay_task, "adc_task", 1024, NULL, 10, NULL);


}
