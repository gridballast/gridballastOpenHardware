/**
 * @file wifi_module.h
 *
 * @brief Defines the Wifi Module API
 *
 * @author Vikram Shanker (vshanker@cmu.edu)
 */

#ifndef __wifi_module_h_
#define __wifi_module_h_

/** @brief depth of the wifi stack */
#define wifiUSStackDepth ((unsigned short) 4096) /* bytes */
/** @brief priority of the wifi stack */
#define wifiUXPriority (2)

/** @brief name of the wifi task */
extern const char * const wifi_task_name;

/**
 * @brief function that initializes that wifi task
 *
 * @return void
 */
void wifi_init_task( void );

#endif /* __wifi_module_h_ */
