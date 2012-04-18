/***************************************************************************
    $Id: DCThread.cpp,v 1.9 2005/06/02 21:34:16 rutt Exp $
    Copyright (C) 2004 by BMI Department, The Ohio State University.
    For license info, see LICENSE in root directory of source distribution.
 ***************************************************************************/

using namespace std;

#include "dcmpi_util.h"

static inline void * dcthread_start(void * arg)
{
    DCThread * t = (DCThread*)arg;
    t->run();
    pthread_exit(NULL);
    return NULL;
}

void DCThread::start()
{
    int rc;
    thread_started = true;
    int tries = 0;
    while (1) {
        rc = pthread_create(&thread, NULL, dcthread_start, this);
        if (rc == 0) {
            break;
        }
        else if (rc == EAGAIN) {
            pthread_attr_t attr;
            pthread_attr_init(&attr);
            pthread_attr_setscope(&attr, PTHREAD_SCOPE_PROCESS);
            rc = pthread_create(&thread, &attr, dcthread_start, this);
            pthread_attr_destroy(&attr);
            if (rc == 0) {
                break;
            }   
            tries++;
            if (tries == 50) {
                std::cerr << "ERROR:  pthread_create() tried 50 times, "
                          << "returning EAGAIN every time.\n"
                          << "I even tried setting PTHREAD_SCOPE_PROCESS, "
                          << "to no avail\n"
                          << "It is time to give up"
                          << " at " << __FILE__ << ":" << __LINE__
                          << std::endl << std::flush;
                exit(1);
            }
            dcmpi_doublesleep(0.001);
        }
        else {
            std::cerr << "ERROR: pthread_create() returned " << rc
                      << " at " << __FILE__ << ":" << __LINE__
                      << std::endl << std::flush;
            exit(1);
        }
    }
}
