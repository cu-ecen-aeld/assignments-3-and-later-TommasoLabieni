#include "threading.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

// Implementation by TL

// Optional: use these functions to add debug or error prints to your application
#define DEBUG_LOG(msg,...)
//#define DEBUG_LOG(msg,...) printf("threading: " msg "\n" , ##__VA_ARGS__)
#define ERROR_LOG(msg,...) printf("threading ERROR: " msg "\n" , ##__VA_ARGS__)

void* threadfunc(void* thread_param)
{

    struct thread_data *data = (struct thread_data*) thread_param;

    /* Wait */
    usleep(data->wait_to_obtain_ms * 1000);
    
    /* Lock Mutex */
    pthread_mutex_lock(data->lock);

    /* Wait */
    usleep(data->wait_to_release_ms * 1000);

    /* Release Mutex */
    pthread_mutex_unlock(data->lock);

    data->thread_complete_success = true;

    return data;
}


bool start_thread_obtaining_mutex(pthread_t *thread, pthread_mutex_t *mutex,int wait_to_obtain_ms, int wait_to_release_ms)
{
    /**
     * allocate memory for thread_data, 
     */

    struct thread_data* thread_data = (struct thread_data*) malloc(sizeof(struct thread_data));
    if (thread_data == NULL)
    {
        perror("Malloc error");
        return false;
    }

    /**
     * Set thread struct data
    */
    thread_data->thread_complete_success = false;
    thread_data->wait_to_obtain_ms = wait_to_obtain_ms;
    thread_data->wait_to_release_ms = wait_to_release_ms;
    thread_data->lock = mutex;

    /**
     * pass thread_data to created thread using threadfunc() as entry point.
    */
    if (pthread_create(thread, NULL, threadfunc, thread_data) != 0)
    {
        perror("Pthread Create error");
        return false;
    }
    
    return true;
}

