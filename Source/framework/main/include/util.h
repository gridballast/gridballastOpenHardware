/**
 * @file util.h
 *
 * @brief utilty function declarations
 *
 * @author Vikram Shanker (vshanker@cmu.edu)
 */

#include "system_state.h"
#include "rwlock.h"
/** @brief state of the system as defined in grid_ballast_main.c */
extern system_state_t gb_system_state;
extern rwlock_t system_state_lock;
extern rwlock_t i2c_lock;


/**
 * @brief get a copy of the system state
 *
 * @param dest - memory region to copy the system state to
 *
 * @return void
 */
void get_system_state( system_state_t *dest );

/**
 * @brief set the system state to the desired value
 *
 * @param src - memory region containing the desired states
 *
 * @return void
 */
void set_system_state( system_state_t *src );
