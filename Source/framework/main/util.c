/**
 * @file util.c
 *
 * @brief utilty functions
 *
 * @author Vikram Shanker (vshanker@cmu.edu)
 */

#include <string.h> // memcpy
#include "util.h"

/**
 * @brief get a copy of the system state
 *
 * @param dest - memory region to copy the system state to
 *
 * @return void
 */
void get_system_state( system_state_t *dest ) {
  memcpy(dest, &gb_system_state, sizeof(gb_system_state));
}

/**
 * @brief set the system state to the desired value
 *
 * @param src - memory region containing the desired states
 *
 * @return void
 */
void set_system_state( system_state_t *src ) {
  memcpy(&gb_system_state, src, sizeof(gb_system_state));
}
