/**
 * @file system_state.h
 *
 * @brief contains the defintion of data structure that keeps track
 *        of the system state
 *
 * @author Vikram Shanker (vshanker@cmu.edu)
 */

#ifndef __system_state_h_
#define __system_state_h_

#define MIC_BUFFER_SIZE 64

/** @brief defines the overall state of the grid ballast system */
typedef struct {
  int timestamp;
  int power;
  int mic[MIC_BUFFER_SIZE];
  int leak_sensor;
  int temp_bottom;
  int temp_top;
  int grid_freq;
  int gps_location;
  int set_point;
  int heating_status;
  int mode; // This should be converted to an enum?
} system_state_t;

#endif /* __system_state_h_ */
