/**
 * @file frq_module.h
 *
 * @brief Defines the Frequency Sensor Module API
 *
 * @author Vikram Shanker (vshanker@cmu.edu)
 * author Rohit Garg (rohitg1@andrew.cmu.edu)
 */

#ifndef __frq_module_h_
#define __frq_module_h_

/** @brief depth of the controller stack */
#define frqUSStackDepth ((unsigned short) 2048) /* bytes */
/** @brief priority of the controller stack */
#define frqUXPriority (2)

/** @brief name of the controller task */
extern const char * const frq_task_name;

/**
 * @brief function that initializes that controller task
 *
 * @return void
 */
void frq_init_task( void );

#endif /* __frq_module_h_ */
