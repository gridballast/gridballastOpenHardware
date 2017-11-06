/**
 * @file rwlock.h
 *
 * @brief A reader writer lock that prioritizes writes. We choose to prioritize
 * writes since consumers of data want the freshest possible data.
 *
 * @author Vikram Shanker (vshanker@cmu.edu)
 */

#ifndef __reader_writer_lock_h_
#define __reader_writer_lock_h_

#include <freertos/semphr.h>

/** @brief success code */
#define RWL_SUCCESS 0
/** @brief initialization error code */
#define RWL_INIT_ERROR (-1)

/** @brief type of a readers - writers lock that prioritizes writers */
typedef struct rwlock {

    /** @brief active readers count */
    int read_count;

    /** @brief mutual exclusion for readers count */
    StaticSemaphore_t read_lock_mem;
    SemaphoreHandle_t read_lock;

    /** @brief active writers count */
    int write_count;

    /** @brief mutual exclusion for writers count */
    StaticSemaphore_t write_lock_mem;
    SemaphoreHandle_t write_lock;

    /** @brief resource lock to control access to resource */
    StaticSemaphore_t resource_lock_mem;
    SemaphoreHandle_t resource_lock;

    /** @brief lock to prioritize writers */
    StaticSemaphore_t block_readers_lock_mem;
    SemaphoreHandle_t block_readers_lock;

} rwlock_t;

/**
 *  @brief create a reader writer lock that prioritizes writes
 *
 *  @param lock - pointer to uninitialized rwlock structure
 *
 *  @return RWL_SUCCESS on success and RWL_INIT_ERROR on failure
 */
int rwlock_init( rwlock_t *lock );

/**
 *  @brief free an initialized reader writer lock
 *
 *  @param lock - the lock to be freed
 */
void rwlock_free( rwlock_t *lock );


/**
 *  @brief lock as a reader
 *
 *  guarantees shared read access
 *
 *  @param lock - the lock to be locked
 *
 *  @return void
 */
void rwlock_reader_lock( rwlock_t *lock );

/**
 *  @brief unlock as a reader
 *
 *  requires that caller previously locked as reader
 *
 *  @param lock - the lock to be unlocked
 *
 *  @return void
 */
void rwlock_reader_unlock(rwlock_t *lock);

/**
 *  @brief lock as a writer
 *
 *  guarantees exclusive write and read access
 *
 *  @param lock - the lock to be locked
 *
 *  @return void
 */
void rwlock_writer_lock(rwlock_t *lock);

/**
 *  @brief unlock as a writer
 *
 *  requires that caller previously locked as writer
 *
 *  @param lock - the lock to be unlocked
 *
 *  @return void
 */
void rwlock_writer_unlock(rwlock_t *lock);


#endif /* __reader_writer_lock_h_ */
