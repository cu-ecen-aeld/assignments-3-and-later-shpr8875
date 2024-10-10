#include "threading.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

// Optional: use these functions to add debug or error prints to your application
//#define DEBUG_LOG(msg,...)
#define DEBUG_LOG(msg,...) printf("threading: " msg "\n" , ##__VA_ARGS__)
#define ERROR_LOG(msg,...) printf("threading ERROR: " msg "\n" , ##__VA_ARGS__)

void* threadfunc(void* thread_param)
{

    // TODO: wait, obtain mutex, wait, release mutex as described by thread_data structure
    // hint: use a cast like the one below to obtain thread arguments from your parameter
    //struct thread_data* thread_func_args = (struct thread_data *) thread_param;
    struct thread_data* thread_func_args = (struct thread_data *) thread_param;
    
    // Initilisation
    thread_func_args->thread_complete_success=false;
    
    // Wait to obtain the mutex
    usleep(thread_func_args->wait_to_obtain_ms*1000);
    
    if (pthread_mutex_lock(thread_func_args->mutex)==0)
    {
    
        DEBUG_LOG("Successful mutex lock");
        // wait for mutex release in microsecs
    	usleep(thread_func_args->wait_to_release_ms*1000);
    	
    	//Unlock mutex
    	if (pthread_mutex_unlock(thread_func_args->mutex)==0)
    	{
        	DEBUG_LOG("Successful mutex unlock");
   		thread_func_args->thread_complete_success=true;
    	}
    	else
    	{
    		ERROR_LOG("Mutex unlock unsuccessful");
    	}
    
    }  
    else
    {
    	ERROR_LOG("Mutex lock unsuccessful");
    }
    
    return thread_param;
}


bool start_thread_obtaining_mutex(pthread_t *thread, pthread_mutex_t *mutex,int wait_to_obtain_ms, int wait_to_release_ms)
{
    /**
     * TODO: allocate memory for thread_data, setup mutex and wait arguments, pass thread_data to created thread
     * using threadfunc() as entry point.
     *
     * return true if successful.
     *
     * See implementation details in threading.h file comment block
     */
     
    // Allocate meemory to thread data structure 
    struct thread_data *thread_func_args = (struct thread_data *) malloc(sizeof(struct thread_data)); 
    if(thread_func_args == NULL)
    {
    	ERROR_LOG("Failed to allocate memory");
    	return false;
    }
    
    thread_func_args->mutex = mutex;
    thread_func_args->wait_to_obtain_ms = wait_to_obtain_ms;
    thread_func_args->wait_to_release_ms= wait_to_release_ms;
    thread_func_args->thread_complete_success=false;
    
    if(pthread_create(thread,NULL,threadfunc,thread_func_args)==0)
    {
        DEBUG_LOG("Thread created successfully");
    	return true;
    }
    
    ERROR_LOG("Thread couldn't obtain mutex");
    return false;
}

