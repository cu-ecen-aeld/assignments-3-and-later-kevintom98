/****************************************************************************************************
  File name   : threading.c

  Created     : 15-Sep-2022

  Author      : Kevin Tom
                MS-ECEE-ESE
                CU Boulder, CO

  Email       : keto9919@colorado.edu

  Description : Creates thread as requested and locks on a mutex till time expires.


  Reference   : * Linux System Programming (Second Edition)
                  Robert Love
                * usleep() : https://linux.die.net/man/3/usleep
******************************************************************************************************/
#include "threading.h"
#include <unistd.h> 
#include <stdlib.h>
#include <stdio.h>
#include <errno.h> //For error handling
#include <time.h>  //Used for sleep() functions


// Optional: use these functions to add debug or error prints to your application
#define DEBUG_LOG(msg,...)
//#define DEBUG_LOG(msg,...) printf("threading: " msg "\n" , ##__VA_ARGS__)
#define ERROR_LOG(msg,...) printf("threading ERROR: " msg "\n" , ##__VA_ARGS__)


#define CONVERT_SEC_TO_NS 1000*1000
#define CONVERT_MS_TO_NS 1000

void* threadfunc(void* thread_param)
{
    struct thread_data * thread_args = (struct thread_data *) thread_param;
    struct timespec wait_rem; //Structure for remaining time
    

    nanosleep(&(thread_args->wait_to_obtain_ms),  &wait_rem);
    
    if(pthread_mutex_lock(thread_args->thread_mutex) != 0)
    {
        perror("pthread_mutex_lock");
        thread_args->thread_complete_success = false;
        return thread_param;
    }

    nanosleep(&(thread_args->wait_to_release_ms),  &wait_rem);

    if(pthread_mutex_unlock(thread_args->thread_mutex) != 0)
    {
        perror("pthread_mutex_unlock");
        thread_args->thread_complete_success = false;
        return thread_param;
    }

    thread_args->thread_complete_success = true;
    return thread_param;
}


bool start_thread_obtaining_mutex(pthread_t *thread, pthread_mutex_t *mutex,int wait_to_obtain_ms, int wait_to_release_ms)
{

    if((thread == NULL) || (mutex == NULL))
    {
        ERROR_LOG("Pointer passed is NULL");
        return false;
    }
    else if((wait_to_obtain_ms < 0) || (wait_to_release_ms < 0))
    {
        ERROR_LOG("wait_to_obtain_ms/wait_to_release_ms is less than 0");
        return false;
    }



    int p_ret, t_sec, t_ns;
    struct thread_data * t_d;


    t_d = malloc(sizeof(struct thread_data));
    
    //Checking if malloc failed
    if(t_d == NULL) 
    {
        perror("malloc");
        ERROR_LOG("malloc() failed");
        return false;
    }

    //Assigining values to structure
    t_d->t_data_dy_memory   = t_d;
    t_d->thread_mutex       = mutex;


    //conversion for waiting
    if(wait_to_obtain_ms > 1000)
    {
        t_sec = (int)wait_to_obtain_ms/1000;
        t_ns  = (((float)(wait_to_obtain_ms/1000)) - (t_sec)) * CONVERT_SEC_TO_NS;

        t_d->wait_to_obtain_ms.tv_sec  = t_sec;
        t_d->wait_to_obtain_ms.tv_nsec = t_ns;
    }
    else
    {
        t_d->wait_to_obtain_ms.tv_sec = 0;
        t_d->wait_to_obtain_ms.tv_nsec = wait_to_obtain_ms * CONVERT_MS_TO_NS;
    }


    if (wait_to_release_ms > 1000)
    {
        t_sec = (int)wait_to_release_ms/1000;
        t_ns  = (((float)(wait_to_release_ms/1000)) - (t_sec)) * CONVERT_SEC_TO_NS;

        t_d->wait_to_release_ms.tv_sec  = t_sec;
        t_d->wait_to_release_ms.tv_nsec = t_ns; 

    }
    else
    {
        t_d->wait_to_release_ms.tv_sec = 0;
        t_d->wait_to_release_ms.tv_nsec = wait_to_release_ms * CONVERT_MS_TO_NS;
    }



    p_ret = pthread_create(thread, NULL, threadfunc, t_d);
    
    //Checking if pthread_create failed
    if(p_ret != 0)
    {
        errno = p_ret;
        perror("pthread_create");
        free(t_d);
        ERROR_LOG("pthread_create() failed");
        return false;
    }

    t_d->thread_id          = (*thread);

    return true;
}
