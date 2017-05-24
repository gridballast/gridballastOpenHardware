#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/timer.h"
#include <stdio.h>


#define TIMER_DIVIDE            0x0050
#define ALARM_VALUE             8000
#define ESP_INTR_FLAG_DEFAULT   0
#define GPIO_MASK               (1 << GPIO_NUM_25)

xQueueHandle time_queue, timer_queue, gpio_evt_queue;

/************************************************************************
 ************************************************************************
 **************************        TASKS        *************************
 ************************************************************************
 ************************************************************************/

void LED_strobe_task(void *arg)
{      
    gpio_set_direction(GPIO_NUM_4, GPIO_MODE_OUTPUT);
    gpio_pullup_en(GPIO_NUM_4);
    int level = 0;
    while (true) {
        // ISR Controlled LED
        gpio_set_level(GPIO_NUM_4, level);
        level = !level;
        vTaskDelay(100 / portTICK_PERIOD_MS);    
    }
}

void timer_print_task(void *arg)
{
    double time_sec;
    while(true) {
        xQueueReceive(timer_queue, &time_sec, portMAX_DELAY);
        printf("time: %.8f s\n", time_sec); 
    }
}

void freq_read_task(void *arg)
{
    double time, freq;
    while(true) {
        if(xQueueReceive(time_queue, &time, portMAX_DELAY)) {
            freq =  1/time;
            printf("Frequency: %0.5fHz Time: %0.10fSecs\n", freq, time);
        }
    }
}


/************************************************************************
 ************************************************************************
 *********************        Interrupts        *************************
 ************************************************************************
 ************************************************************************/

void IRAM_ATTR timer_isr (void *Paramters)
{
    int timer_idx = (int) Paramters;
 
    // Clear the timer Interrupt 
    TIMERG0.int_clr_timers.t0 = 1;
    // Request the value from the timer
    TIMERG0.hw_timer[timer_idx].update = 1;
    // Reset the timer to 0
    timer_set_counter_value(TIMER_GROUP_0, TIMER_0, 0x00000000ULL);
    // Get the clock tick count from the timer registers
    uint64_t timer_val = ((uint64_t) TIMERG0.hw_timer[timer_idx].cnt_high) << 32 | TIMERG0.hw_timer[timer_idx].cnt_low;
    // Convert the clock ticks into seconds
    double time_sec = (double) timer_val / (TIMER_BASE_CLK / TIMERG0.hw_timer[timer_idx].config.divider);
    // Send the seconds to LED_TIMER task
    xQueueSendFromISR(timer_queue, &time_sec, NULL);
    // Turn alarm back on
    TIMERG0.hw_timer[timer_idx].config.alarm_en = 1;

}

void IRAM_ATTR freq_isr (void *arg)
{
    static int level = 0;
    double time;
    int t_group = TIMER_GROUP_0;
    int t_idx   = TIMER_0;
    level = !level;
    if(level){
        timer_start(t_group, t_idx);
    }
    else{
        timer_pause(t_group, t_idx);
        timer_get_counter_time_sec(t_group, t_idx, &time);
        // Clear timer after retreiving the time 
        timer_set_counter_value(TIMER_GROUP_0, TIMER_0, 0x00000000ULL);
        xQueueSendFromISR(time_queue, &time, NULL);
    }
}

/************************************************************************
 ************************************************************************
 ****************    Configuration Functions        *********************
 ************************************************************************
 ************************************************************************/

// Timer Interrupt Configure
void timer0_interrupt_config()
{
    // Create a timer Queue
    timer_queue = xQueueCreate(10, sizeof(double));

    // Intialization Paramters 
    timer_config_t config;      // Timer Configuration Type
    config.alarm_en = 1;        // Alarm is how interrupts are triggered
    config.auto_reload = 0;     // Will restart the timer to whatever the counter value
    config.counter_dir = TIMER_COUNT_UP;
    config.divider = TIMER_DIVIDE; //Divide by 8000 to get a freq of 10KHz 
    config.intr_type = TIMER_INTR_LEVEL;
    config.counter_en = TIMER_PAUSE;

    // Configure Timer Regsiters 

    int t_group = TIMER_GROUP_0;
    int t_idx   = TIMER_0;

    /* Configure Timer Registers*/
    timer_init(t_group, t_idx, &config);
    /*Stop timer counter*/
    timer_pause(t_group, t_idx);
    /*Reload Value for Counter*/
    timer_set_counter_value(t_group, t_idx, 0x00000000ULL);
    
    /*Set alarm Value*/
    timer_set_alarm_value(t_group, t_idx, ALARM_VALUE);
    
    /*Enable the Timer Interrupt*/
    timer_enable_intr(t_group, t_idx);
    /*Set ISR handler*/
    timer_isr_register(t_group, t_idx, timer_isr, (void*) t_idx, ESP_INTR_FLAG_IRAM, NULL);
    
    /*Start the Timer*/
    timer_start(t_group, t_idx);
}

// Setup the Frequency 
void freq_counter_setup()
{
    // Create the Queue 
    time_queue = xQueueCreate(10, sizeof(double));

    int t_group = TIMER_GROUP_0;
    int t_idx   = TIMER_0;

    timer_config_t freq_config;                 // Frequency Configuration Type
    freq_config.alarm_en = 0;                   
    freq_config.auto_reload = 0;
    freq_config.counter_dir = TIMER_COUNT_UP;
    freq_config.divider = TIMER_DIVIDE;         // Divide by 8000 to get a freq of 10KHz
    freq_config.counter_en = TIMER_PAUSE;

    timer_init(t_group, t_idx, &freq_config);
    timer_set_counter_value(t_group, t_idx, 0x00000000ULL);
    printf("DONE freq setup\n");
}


void gpio_freq_config()
{
    gpio_config_t freq;

    // Initialization Parameters
    freq.intr_type = GPIO_PIN_INTR_POSEDGE;
    freq.pin_bit_mask = GPIO_MASK;
    freq.mode = GPIO_MODE_INPUT;
    freq.pull_up_en = 0;
    gpio_config(&freq);

    //create a queue to handle gpio event from isr
    gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));  

    //install gpio isr service
    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    //hook isr handler for specific gpio pin
    gpio_isr_handler_add(GPIO_NUM_25, freq_isr, (void*) GPIO_NUM_25);
    printf("DONE gpio\n");

}


/************************************************************************
 ************************************************************************
 **************************        MAIN       ***************************
 ************************************************************************
 ************************************************************************/
// Main App where main code goes
void app_main(void)
{
    // Setup Interrupt Timer 
    //timer0_interrupt_config();

    // Task Timer Setup
    freq_counter_setup();

    // GPIO Setup
    gpio_freq_config();


    //xTaskCreate(LED_strobe_task, "LED_strobe_task", 2048, NULL, 3, NULL);
    xTaskCreate(freq_read_task, "freq_read_task", 2048, NULL, 20, NULL);  
    //xTaskCreate(timer_print_task, "timer_print_task", 2048, NULL, 5, NULL);
}


