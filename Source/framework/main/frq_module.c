#include <stdio.h>
#include "driver/gpio.h"
#include "driver/timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "util.h"

#define ESP_INTR_FLAG_DEFAULT 0

#define CONFIG_FRQ_PIN  26				 // 60 Hz pulse ADC input
#define TIMER_DIVIDER   80               // Hardware timer clock divider
#define TIMER_INTR_SEL TIMER_INTR_LEVEL  // Timer level interrupt

xQueueHandle frq_queue;

static intr_handle_t s_timer_handle;

int timer_group = TIMER_GROUP_0;
int timer_idx = TIMER_0;

uint64_t timer_val = 0;
float frq = 0.0;
int status = 0;

float avg_frq;

int l =0;
int flag = 0;

const char * const frq_task_name = "frq_module_task";




//function to check averaging length overflow
size_t highestOneBitPosition(uint32_t a) {
    size_t bits=0;
    while (a!=0) {
        ++bits;
        a>>=1;
    };
    return bits;
}
bool addition_is_safe(uint32_t a) {
    size_t a_bits=highestOneBitPosition(a), b_bits=highestOneBitPosition(1);
    return (a_bits<32 && b_bits<32);
}



void IRAM_ATTR timer_group0_isr(void *para)
{

  TIMERG0.int_clr_timers.t1 = 1;

  TIMERG0.hw_timer[TIMER_1].config.alarm_en = 1;

  flag=1;


}


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
	uint64_t first = -1;
	uint64_t second = -1;
	system_state_t mystate;
  for(;;) {
    // wait for the notification from the ISR
		uint64_t timer_val;
		xQueueReceive(frq_queue, &timer_val, portMAX_DELAY);

		if(timer_val < 15000 && first == -1) {
    	first = timer_val;
		} else if(timer_val < 15000 && second == -1) {
		  second = timer_val;
		  timer_val = first + second;
		  second = -1;
		  first = -1;
		}
		duration = timer_val - last_val;
		last_val = timer_val;
		if(duration > 15000) {
      frq = 1.0/duration*1000000*1.0;
      //printf("frequency is %f\n", frq);



      if (l == 0)
       {
        avg_frq = frq;
          l+= 1;
       }
        avg_frq = (avg_frq*l + frq)/(l+1);
        if(addition_is_safe(l)) {
          l+= 1;
        } else {
          l = 0;
        }
        
        
        if(flag == 1) {
          printf("frequency is %f\n", avg_frq);
          flag=0;
        }


			get_system_state(&mystate);
			mystate.grid_freq = frq;
		}
	}
}

void frq_init_task(void *arg) {
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


    //TIMER FOR REPORTING AVERAGE FREQUENCY

    timer_config_t config1;
    config1.alarm_en = 1;
    config1.auto_reload = 1;
    config1.counter_dir = TIMER_COUNT_UP;
    config1.divider = 80;
    config1.intr_type = TIMER_INTR_SEL;
    config1.counter_en = false;

    timer_init(timer_group, TIMER_1, &config1);

    timer_set_alarm_value(timer_group, TIMER_1, 5000000);             
  
    timer_enable_intr(timer_group, TIMER_1);

    timer_isr_register(timer_group, TIMER_1, &timer_group0_isr, NULL, 0, &s_timer_handle);

    timer_start(timer_group, TIMER_1);



    //GPIO interrupt from 60Hz pulse
    gpio_set_intr_type(CONFIG_FRQ_PIN, GPIO_INTR_ANYEDGE);

    // install ISR service with default configuration
    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    // attach the interrupt service routine
    gpio_isr_handler_add(CONFIG_FRQ_PIN, frq_isr_handler, NULL);
    xTaskCreate(&frq_task, "frq_task", 2048, NULL, 10, NULL);
}
