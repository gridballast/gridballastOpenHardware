#include <stdio.h>
#include "driver/gpio.h"
#include "driver/timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
//#include "freertos/semphr.h"
#include "freertos/task.h"
#include "util.h"



#define ESP_INTR_FLAG_DEFAULT 0

#define CONFIG_FRQ_PIN  35				 // 60 Hz pulse ADC input
#define TIMER_DIVIDER   80               // Hardware timer clock divider
#define TIMER_INTR_SEL TIMER_INTR_LEVEL  // Timer level interrupt

xQueueHandle frq_queue;

system_state_t mystate;

static intr_handle_t s_timer_handle;

int timer_group = TIMER_GROUP_0;
int timer_idx = TIMER_0;

uint64_t timer_val = 0;
float frq = 0.0;
int status = 0;
// int freq_values = 63;
float avg_frq = 0;

int l = 0;
int flag = 0;

const char * const frq_task_name = "frq_module_task";


void IRAM_ATTR frq_isr_handler(void* arg) {
  timer_get_counter_value(timer_group, timer_idx, &timer_val);
  status = !status;
  gpio_set_level(12, status);
  xQueueSendFromISR(frq_queue, &timer_val, NULL);
}

void frq_task(void* arg) {
	// infinite loop
	uint64_t duration = 0;
	uint64_t last_val = 0;

  uint64_t timer_val;

  for(;;) {
    // wait for the notification from the ISR
		
		xQueueReceive(frq_queue, &timer_val, portMAX_DELAY);

    duration = timer_val - last_val;

		// if(duration < 15000 && first == -1) {
  //   	first = duration;
		// } else if(duration < 15000 && second == -1) {
		//   second = duration;
		//   duration = first + second;
		//   second = -1;
		//   first = -1;
  //     last_val = 0;
		// }
		last_val = timer_val;

		if(duration > 15000) {
      frq = 1.0/duration*1000000*1.0;
      


      if (frq > 50)
      {
        avg_frq = (avg_frq*l + frq)/(l+1);

        // freq_values++;
  
        l++;
      }
      if(l >= 200) {
        //printf("frequency is %f\n", avg_frq);
        //freq_values = 0;
        rwlock_writer_lock(&system_state_lock);
        get_system_state(&mystate);
        mystate.grid_freq = avg_frq;
        set_system_state(&mystate);
        rwlock_writer_unlock(&system_state_lock);
        l = 0;
        avg_frq = 0;
        
      }  
			
		}
	}
}
void frq_init_task(void *arg) {

          printf("frequency is %f\n", avg_frq);


    gpio_pad_select_gpio(CONFIG_FRQ_PIN);
    gpio_pad_select_gpio(12);

    // set the correct direction
    gpio_set_direction(CONFIG_FRQ_PIN, GPIO_MODE_INPUT);
    gpio_set_direction(12, GPIO_MODE_OUTPUT);

    frq_queue = xQueueCreate(1, sizeof(timer_val));


    //timer to measure frequency
    timer_config_t config;
    config.alarm_en = 0;
    config.auto_reload = 0;
    config.counter_dir = TIMER_COUNT_UP;
    config.divider = TIMER_DIVIDER;
    config.intr_type = TIMER_INTR_SEL;
    config.counter_en = TIMER_PAUSE;

    timer_init(timer_group, timer_idx, &config);
    timer_start(timer_group, timer_idx);

    //GPIO interrupt from 60Hz pulse
    gpio_set_intr_type(CONFIG_FRQ_PIN, GPIO_INTR_POSEDGE);

    // install ISR service with default configuration
    //gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    // attach the interrupt service routine
    gpio_isr_handler_add(CONFIG_FRQ_PIN, frq_isr_handler, NULL);
    xTaskCreatePinnedToCore(frq_task, "frq_task", 2048, NULL, 5, NULL,1);
}
