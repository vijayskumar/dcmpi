#ifndef DCCOND_H
#define DCCOND_H

/***************************************************************************
    $Id: DCCond.h,v 1.1.1.1 2005/02/17 13:45:36 rutt Exp $
    Copyright (C) 2004 by BMI Department, The Ohio State University.
    For license info, see LICENSE in root directory of source distribution.
 ***************************************************************************/

#include <pthread.h>

#include "DCMutex.h"

class DCCond
{
private:
    DCMutex mutex;
    pthread_cond_t  cond;
public:
    DCCond()
    {
        if (pthread_cond_init(&cond, NULL)) {
            std::cerr << "ERROR: initializing cond"
                      << " at " << __FILE__ << ":" << __LINE__
                      << std::endl << std::flush;
            exit(1);
        }
    }
    ~DCCond()
    {
        if (pthread_cond_destroy(&cond)) {
            std::cerr << "ERROR: destroying cond"
                      << " at " << __FILE__ << ":" << __LINE__
                      << std::endl << std::flush;
            exit(1);
        }
    }
    void acquire()
    {
        mutex.acquire();
    }
    void release()
    {
        mutex.release();
    }
    void wait()
    {
        int ret = pthread_cond_wait(&cond, &mutex.mutex);
        if (ret != 0) {
            std::cerr << "ERROR: pthread_cond_wait "
                      << " at " << __FILE__ << ":" << __LINE__
                      << std::endl << std::flush;
            exit(1);
        }
    }
    void notify()
    {
        int ret = pthread_cond_signal(&cond);
        if (ret != 0) {
            std::cerr << "ERROR: pthread_cond_signal "
                      << " at " << __FILE__ << ":" << __LINE__
                      << std::endl << std::flush;
            exit(1);
        }
    }
    void notifyAll()
    {
        int ret = pthread_cond_broadcast(&cond);
        if (ret != 0) {
            std::cerr << "ERROR: pthread_cond_broadcast "
                      << " at " << __FILE__ << ":" << __LINE__
                      << std::endl << std::flush;
            exit(1);
        }
    }
};

#endif /* #ifndef DCCOND_H */
