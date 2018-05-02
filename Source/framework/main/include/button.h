/**
 * @file ct_module.h
 *
 * @brief Defines the CT Module API
 *
 * @author Vikram Shanker (vshanker@cmu.edu)
 * @author Rohit Garg (rohitg1@andrew.cmu.edu)
 */

#ifndef __button_h_
#define __button_h_

/** @brief depth of the controller stack */
#define ctUSStackDepth ((unsigned short) 2048) /* bytes */
/** @brief priority of the controller stack */
#define ctUXPriority (2)

/** @brief name of the controller task */
extern const char * const ct_task_name;

/**
 * @brief function that initializes that controller task
 *
 * @return void
 */
void button_init_task( void );

#endif /* __ct_module_h_ */
