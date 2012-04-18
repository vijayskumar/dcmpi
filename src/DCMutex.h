#ifndef DCMUTEX_H
#define DCMUTEX_H

/***************************************************************************
    $Id: DCMutex.h,v 1.2 2005/07/05 19:34:44 rutt Exp $
    Copyright (C) 2004 by BMI Department, The Ohio State University.
    For license info, see LICENSE in root directory of source distribution.
 ***************************************************************************/

#include <pthread.h>

class DCMutex
{
public:
    pthread_mutex_t mutex;
    DCMutex()
    {
        if (pthread_mutex_init(&mutex, NULL)) {
            std::cerr << "ERROR: initializing mutex"
                      << " at " << __FILE__ << ":" << __LINE__
                      << std::endl << std::flush;
            exit(1);
        }
    }
    ~DCMutex()
    {
        (void)(pthread_mutex_destroy(&mutex));
    }
    void acquire()
    {
        int ret = pthread_mutex_lock(&mutex);
        if (ret != 0) {
            std::cerr << "ERROR: pthread_mutex_lock failed"
                      << " at " << __FILE__ << ":" << __LINE__
                      << std::endl << std::flush;
            exit(1);
        }
    }
    void release()
    {
        int ret = pthread_mutex_unlock(&mutex);
        if (ret != 0) {
            std::cerr << "ERROR: pthread_mutex_unlock failed"
                      << " at " << __FILE__ << ":" << __LINE__
                      << std::endl << std::flush;
            exit(1);
        }
    }
};

#endif /* #ifndef DCMUTEX_H */
