#include <stdio.h>

#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "driver/timer.h"
#include "freertos/queue.h"

#define ESP_INTR_FLAG_DEFAULT 0


#define CONFIG_FRQ_PIN  26
#define TIMER_DIVIDER   80               /*!< Hardware timer clock divider */
#define TIMER_INTR_SEL TIMER_INTR_LEVEL  /*!< Timer level interrupt */

xQueueHandle frq_queue;

int timer_group = TIMER_GROUP_0;
int timer_idx = TIMER_0;

uint64_t timr_val = 0;

bool status = false;

float frq = 0.0;


// interrupt service routine, called when the button is pressed
void IRAM_ATTR frq_isr_handler(void* arg) {
    
	
    // notify the button task
 
    //uint64_t timer_val = 0;
	//uint64_t duration = 0;

    timer_get_counter_value(timer_group, timer_idx, &timr_val);

    //duration = timr_val - timer_val;
	//timer_val = timr_val;

    status = !status;
	gpio_set_level(12, status);

	xQueueSendFromISR(frq_queue, &timr_val, NULL);
	//xSemaphoreGiveFromISR(xSemaphore, NULL);
}

// task that will react to button clicks
void frq_task(void* arg) {

	

	// infinite loop
	uint64_t duration = 0;
	uint64_t last_val = 0;
	uint64_t first = -1;
	uint64_t second = -1;
	for(;;) {
		
		// wait for the notification from the ISR
		uint64_t timer_val;

		// if(xSemaphoreTake(xSemaphore,portMAX_DELAY) == pdTRUE) {
		// 	if(flag == 0) {
		// 		timer_start(timer_group, timer_idx);
		// 		flag = 1;
		// 	}
			//printf("Button pressed!\n");
			//timer_init(timer_group, timer_idx, &config);
			//timer_start(timer_group, timer_idx);
			//status = !status;
			//gpio_set_level(12, status);
			// timer_get_counter_value(timer_group, timer_idx, &timr_val);
			// printf("tmr : %llu\n", timr_val);
			// duration = timr_val - timer_val;
			// timer_val = timr_val;

		    xQueueReceive(frq_queue, &timer_val, portMAX_DELAY);
		    // printf("tmr : %llu\n", timer_val);
		    if(timer_val < 15000 && first == -1) {
		    	first = timer_val;
		    }
		    else if(timer_val < 15000 && second == -1) {
		    	second = timer_val;
		    	timer_val = first + second;
		    	second = -1;
		    	first = -1;
		    }
			duration = timer_val - last_val;
			last_val = timer_val;
			if(duration > 15000) {
				//printf("frequency is %f\n", 1.0/duration*1000000*1.0);

				frq = 1.0/duration*1000000*1.0;
				printf("frequency is %f\n", frq);
			
			}

		
	}
}

static void timerconfig(void *arg)
{

	// timer_config_t config;
 //    config.alarm_en = 0;
 //    config.auto_reload = 0;
 //    config.counter_dir = TIMER_COUNT_UP;
 //    config.divider = TIMER_DIVIDER;
 //    config.intr_type = TIMER_INTR_SEL;
 //    config.counter_en = TIMER_PAUSE;

 //    timer_init(timer_group, timer_idx, &config);

 //    timer_start(timer_group, timer_idx);

 //    gpio_set_intr_type(CONFIG_FRQ_PIN, GPIO_INTR_ANYEDGE);

 //    printf("Testing...\n");
	
	
	// // install ISR service with default configuration
	// gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
	
	// // attach the interrupt service routine
	// gpio_isr_handler_add(CONFIG_FRQ_PIN, frq_isr_handler, NULL);



}

void app_main()
{
	

	
	// configure button and led pins as GPIO pins
	gpio_pad_select_gpio(CONFIG_FRQ_PIN);
	gpio_pad_select_gpio(12);
	
	// set the correct direction
	gpio_set_direction(CONFIG_FRQ_PIN, GPIO_MODE_INPUT);
    gpio_set_direction(12, GPIO_MODE_OUTPUT);

    frq_queue = xQueueCreate(1, sizeof(timr_val)); //comment if you don't want to use capture module
    // xTaskCreate(timerconfig, "timerconfig", 4096, NULL, 5, NULL);
	timer_config_t config;
    config.alarm_en = 0;
    config.auto_reload = 0;
    config.counter_dir = TIMER_COUNT_UP;
    config.divider = TIMER_DIVIDER;
    config.intr_type = TIMER_INTR_SEL;
    config.counter_en = TIMER_PAUSE;

    timer_init(timer_group, timer_idx, &config);

    timer_start(timer_group, timer_idx);

    gpio_set_intr_type(CONFIG_FRQ_PIN, GPIO_INTR_ANYEDGE);

    // printf("Testing...\n");
	
	
	// install ISR service with default configuration
	gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
	
	// attach the interrupt service routine1.0/duration*1000000*1.0
	gpio_isr_handler_add(CONFIG_FRQ_PIN, frq_isr_handler, NULL);



	//timer_start(timer_group, timer_idx);
	// enable interrupt on falling (1->0) edge for button pin
	/*gpio_set_intr_type(CONFIG_FRQ_PIN, GPIO_INTR_ANYEDGE);
	
	
	// install ISR service with default configuration
	gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
	
	// attach the interrupt service routine
	gpio_isr_handler_add(CONFIG_FRQ_PIN, frq_isr_handler, NULL);*/

	// start the task that will handle the button
	xTaskCreate(frq_task, "frq_task", 2048, NULL, 10, NULL);
}