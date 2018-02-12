#include <stdio.h>

#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/spi_master.h"
#include "string.h"
#include "freertos/queue.h"
#include <driver/adc.h>
#include "esp_system.h"
#include "esp_spi_flash.h"
#include "driver/timer.h"
#include "math.h"
#include "esp_log.h"

#include "u8g2.h"                                //display driver library
#include "u8g2_esp32_hal.h"					     //ESP32 HAL library for u8g2

#include "driver/Ada_MCP.h"                      //IO expander library
#include "driver/generic_rw_i2c.h"



#define PIN_SDA 14

#define PIN_SCL 15

#define I2C_MASTER_FREQ_HZ    400000     /* I2C master clock frequency */


void task_lcd(void *arg)

{


	gpio_pad_select_gpio(2);             //MCP reset pin

	gpio_set_direction(2, GPIO_MODE_OUTPUT);

	gpio_set_level(2, 1);

	

	generic_i2c_master_init (I2C_NUM_0, PIN_SCL, PIN_SDA, I2C_MASTER_FREQ_HZ);   //initialize I2C communication with IO expander

	begin(0);                          /*ADA_MCP function*/

	pinMode(8,GPIO_MODE_OUTPUT);       // LCD reset pin

	digitalWrite(8,0);



	vTaskDelay(1000 / portTICK_PERIOD_MS);

	digitalWrite(8,1);




	u8g2_esp32_hal_t u8g2_esp32_hal = U8G2_ESP32_HAL_DEFAULT;
	u8g2_esp32_hal.sda = PIN_SDA;
	u8g2_esp32_hal.scl = PIN_SCL;
	u8g2_esp32_hal_init(u8g2_esp32_hal);


	u8g2_t u8g2; // a structure which will contain all the data for one display



	u8g2_Setup_ssd1309_i2c_128x64_noname0_f(&u8g2, U8G2_R0, u8g2_esp32_i2c_byte_cb, u8g2_esp32_gpio_and_delay_cb);  //initialize u8g2 structure

	u8x8_SetI2CAddress(&u8g2.u8x8, 0x78);


	u8g2_InitDisplay(&u8g2); // send init sequence to the display, display is in sleep mode after this,
	u8g2_SetPowerSave(&u8g2, 0); // wake up display

	u8g2_SetContrast(&u8g2, 100);

	u8g2_SetFlipMode(&u8g2, 1);

	//u8g2_DrawBox(&u8g2, 70,0,45,55);


	get_system_state(mystate);
	float freq = mystate -> grid_frequency;
	char str [32];

	sprintf(str, "freq= %2.2f", );
	u8g2_ClearBuffer(&u8g2);

	u8g2_SetFont(&u8g2,u8g2_font_ncenB14_tr);
	u8g2_DrawStr(&u8g2, 20, 20, str);
	u8g2_SendBuffer(&u8g2);

	//u8g2_ClearDisplay(&u8g2);

	ESP_LOGD(tag, "all done");

	vTaskDelay(1000);

}



void lcd_init()
{
	xTaskCreate(task_lcd, "lcd_task", 2048, NULL, 10, NULL);
}