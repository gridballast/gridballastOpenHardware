/**
 * @file sensing_module.h
 *
 * @brief Defines the Sensing Module API
 *
 * @author Vikram Shanker (vshanker@cmu.edu)
 */

#ifndef __sensing_module_h_
#define __sensing_module_h_

/** @brief depth of the sensing stack */
#define sensingUSStackDepth ((unsigned short) 2048) /* bytes */
/** @brief priority of the sensing stack */
#define sensingUXPriority (2)

/** @brief name of the sensing task */
const char *sensing_task_name = "sensing_module_task";

/**
 * @brief function that initializes that sensing task
 *
 * @return void
 */
void sensing_init_task( void );

#endif /* __sensing_module_h_ */
