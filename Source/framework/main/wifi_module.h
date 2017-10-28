/**
 * @file wifi_module.h
 *
 * @author Vikram Shanker (vshanker@cmu.edu)
 */

#ifndef _wifi_module_h_

#define _wifi_module_h_

/** @brief depth of the wifi stack */
#define wifiUSStackDepth ((unsigned short) 2048)
/** @brief priority of the wifi stack */
#define wifiUXPriority (2)

/** @brief name of the wifi task */
const char *wifi_task_name = "wifi_module_task";

/**
 * @brief function that initializes that wifi task
 *
 * @param pv_parameters - parameters for task being created (should be NULL)
 *
 * @return void
 */
void wifi_init_task( void );

#endif /* _wifi_module_h_ */
