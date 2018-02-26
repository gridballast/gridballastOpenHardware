/**
 * @file rs485_module.h
 *
 * @brief Defines the RS485 Module API
 *
 * @author Vikram Shanker (vshanker@cmu.edu)
 * author Rohit Garg (rohitg1@andrew.cmu.edu)
 */

#ifndef __rs_485_module_h_
#define __rs_485_module_h_

/** @brief depth of the controller stack */
#define frqUSStackDepth ((unsigned short) 2048) /* bytes */
/** @brief priority of the controller stack */
#define frqUXPriority (3)

/** @brief name of the controller task */
extern const char * const rs485_task;

/**
 * @brief function that initializes that controller task
 *
 * @return void
 */
void rs485_init_task( void );


#endif /* __frq_module_h_ */