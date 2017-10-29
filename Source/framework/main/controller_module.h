/**
 * @file controller_module.h
 *
 * @brief Defines the Controller Module API
 *
 * @author Vikram Shanker (vshanker@cmu.edu)
 */

#ifndef __controller_module_h_
#define __controller_module_h_

/** @brief depth of the controller stack */
#define controllerUSStackDepth ((unsigned short) 2048) /* bytes */
/** @brief priority of the controller stack */
#define controllerUXPriority (2)

/** @brief name of the controller task */
const char *controller_task_name = "controller_module_task";

/**
 * @brief function that initializes that controller task
 *
 * @return void
 */
void controller_init_task( void );

#endif /* __controller_module_h_ */
