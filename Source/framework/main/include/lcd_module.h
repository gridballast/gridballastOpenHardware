/**
 * @file lcd_module.h
 *
 * @brief Defines the LCD Module API
 *
 * @author Vikram Shanker (vshanker@cmu.edu)
 */

#ifndef __lcd_module_h_
#define __lcd_module_h_

/** @brief depth of the controller stack */
#define lcdUSStackDepth ((unsigned short) 2048) /* bytes */
/** @brief priority of the controller stack */
#define lcdUXPriority (2)

/** @brief name of the controller task */
extern const char * const lcd_task_name;

/**
 * @brief function that initializes that controller task
 *
 * @return void
 */
void lcd_init_task( void );

#endif /* __lcd_module_h_ */
