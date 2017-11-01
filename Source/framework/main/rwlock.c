/**
 * @file rwlock.c
 *
 * @brief implementation of a reader writer lock that prioritizes writes
 *
 * We choose to prioritize writes since consumers of data want the freshest
 * possible data.
 *
 * @author Vikram Shanker (vshanker@cmu.edu)
 */

#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "rwlock.h"
#include "semphr.h"

#define BLOCK_INDEFINITE portMAX_DELAY

int rwlock_init( rwlock_t *lock ) {

    if ( lock == NULL ) {
        return RWL_INIT_ERROR;
    }

    lock->read_count = 0;
    lock->write_count = 0;

    /*
     * Initialize to NULL to make error handling easier.
     */
    lock->read_lock = NULL;
    lock->write_lock = NULL;
    lock->resource_lock = NULL;
    lock->block_readers_lock = NULL;

    lock->read_lock = xSemaphoreCreateMutexStatic(lock->read_lock_mem);
    if (lock->read_lock == NULL) {
        goto ERROR;
    }

    lock->write_lock = xSemaphoreCreateMutexStatic(lock->write_lock_mem);
    if (lock->write_lock == NULL) {
        goto ERROR;
    }

    /* this has be a binary semaphore and not a mutex */
    lock->resource_lock = xSemaphoreCreateBinaryStatic(lock->resource_lock_mem);
    if (lock->resource_lock == NULL) {
        goto ERROR;
    }

    /* initialize as available */
    xSemaphoreGive(lock->resource_lock);

    /* this has be a binary semaphore and not a mutex */
    lock->block_readers_lock = xSemaphoreCreateBinaryStatic(lock->block_readers_lock_mem);
    if (lock->block_readers_lock == NULL) {
        goto ERROR;
    }

    /* initialize as available */
    xSemaphoreGive(lock->block_readers_lock);

    return RWL_SUCCESS;

    /* Handle initialization error cases */
ERROR:
    if (!lock->read_lock) {
        vSemaphoreDelete(lock->read_lock);
    }

    if (!lock->write_lock) {
        vSemaphoreDelete(lock->read_lock);
    }

    if (!lock->resource_lock) {
        vSemaphoreDelete(lock->read_lock);
    }

    if (!lock->block_readers_lock) {
        vSemaphoreDelete(lock->read_lock);
    }

    return RWL_INIT_ERROR;
}


void rwlock_free( rwlock_t *lock ) {

    vSemaphoreDelete(lock->read_lock);
    vSemaphoreDelete(lock->write_lock);
    vSemaphoreDelete(lock->resource_lock);
    vSemaphoreDelete(lock->block_readers_lock);
}


void rwlock_reader_lock( rwlock_t *lock ) {

    xSemaphoreTake(lock->block_readers_lock, BLOCK_INDEFINITE);
    xSemaphoreTake(lock->read_lock, BLOCK_INDEFINITE);

    lock->read_count++;
    if (lock->read_count == 1) {
        xSemaphoreTake(lock->resource_lock, BLOCK_INDEFINITE);
    }

    xSemaphoreGive(lock->read_lock);
    xSemaphoreGive(lock->block_readers_lock);
}


void rwlock_reader_unlock( rwlock_t *lock ) {

    xSemaphoreTake(lock->read_lock, BLOCK_INDEFINITE);

    lock->read_count--;
    if (lock->read_count == 0) {
        xSemaphoreGive(lock->resource_lock);
    }

    xSemaphoreGive(lock->read_lock);
}


void rwlock_writer_lock( rwlock_t *lock ) {

    xSemaphoreTake(lock->write_lock, BLOCK_INDEFINITE);

    lock->write_count++;
    if (lock->write_count == 1) {
        xSemaphoreTake(lock->block_readers_lock, BLOCK_INDEFINITE);
    }

    xSemaphoreGive(lock->write_lock);

    xSemaphoreTake(lock->resource_lock, BLOCK_INDEFINITE);
}


void rwlock_writer_unlock( rwlock_t *lock ) {

    xSemaphoreGive(lock->resource_lock);
    xSemaphoreTake(lock->write_lock, BLOCK_INDEFINITE);

    lock->write_count--;
    if (lock->write_count == 0) {
        xSemaphoreGive(lock->block_readers_lock);
    }

    xSemaphoreGive(lock->write_lock);
}
