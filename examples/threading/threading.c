#include "threading.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

// Optional: use these functions to add debug or error prints to your application
#define DEBUG_LOG(msg,...)
//#define DEBUG_LOG(msg,...) printf("threading: " msg "\n" , ##__VA_ARGS__)
#define ERROR_LOG(msg,...) printf("threading ERROR: " msg "\n" , ##__VA_ARGS__)

void* threadfunc(void* thread_param)
{
    struct thread_data* data = (struct thread_data*) thread_param;
    
    usleep(data->wait_to_obtain_ms * 1000);
    
    int lock_status = pthread_mutex_lock(data->mutex);
    if(lock_status == 0){
    	usleep(data->wait_to_release_ms * 1000);
    	pthread_mutex_unlock(data->mutex);
    	data-> thread_complete_success = true;
    } else {
	data->thread_complete_success = false;
    }
    return thread_param;
}


bool start_thread_obtaining_mutex(pthread_t *thread, pthread_mutex_t *mutex,int wait_to_obtain_ms, int wait_to_release_ms)
{
    struct thread_data* args = (struct thread_data*)malloc(sizeof(struct thread_data));
    args->mutex = mutex;
    args->wait_to_obtain_ms = wait_to_obtain_ms;
    args->wait_to_release_ms = wait_to_release_ms;
    args->thread_complete_success = false;

    int ret = pthread_create(thread,NULL,threadfunc,(void*)args);    
    return ret == 0;
}

