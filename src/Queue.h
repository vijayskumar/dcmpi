#ifndef QUEUE_H
#define QUEUE_H

/***************************************************************************
    $Id: Queue.h,v 1.3 2008/02/07 05:53:34 krishnan Exp $
    Copyright (C) 2004 by BMI Department, The Ohio State University.
    For license info, see LICENSE in root directory of source distribution.
 ***************************************************************************/

#include <list>

#include "DCCond.h"

// thread-safe queue

template <class T>
class Queue
{
public:
    std::list<T> q;
    DCCond cv;
    virtual ~Queue() {}
    virtual T get()
    {
        cv.acquire();
        while (q.empty()) {
            cv.wait();
        }
        T outval = q.front();
        q.pop_front();
#ifdef DEBUG2
        std::cout << "Queue::get() returning " << outval << std::endl;
#endif
        cv.release();
        return outval;
    }
    virtual void put(T obj)
    {
        cv.acquire();
#ifdef DEBUG2
        std::cout << "Queue.put(" << obj << ")" << std::endl;
#endif
        q.push_back(obj);
        cv.notify();
        cv.release();
    }
    bool empty()
    {
        cv.acquire();
        bool out = q.empty();
        cv.release();
        return out;
    }
    bool getall_if_item_present(std::vector<T> & out_items)
    {
        bool out;
        cv.acquire();
        if (q.empty()) {
            out = false;
        }
        else {
            out_items.clear();
            typename std::list<T>::iterator it;
            for (it = q.begin(); it != q.end(); it++) {
                out_items.push_back(*it);
            }
            q.clear();
            out = true;
        }
        cv.release();
        return out;
    }
    bool get_if_item_present(T & out_item)
    {
        bool out;
        cv.acquire();
        if (q.empty()) {
            out = false;
#ifdef DEBUG2
            std::cout << "Queue.get_if_item_present() not returning anything"
                      << std::endl;
#endif
        }
        else {
            out = true;
            out_item = q.front();
#ifdef DEBUG2
            std::cout << "Queue.get_if_item_present() returning " << out_item
                      << std::endl;
#endif
            q.pop_front();
        }
        cv.release();
        return out;
    }
    int size()
    {
        cv.acquire();
        int out = q.size();
        cv.release();
        return out;
    }

    // "track putters" interface
    std::set<int> putters;
    virtual bool has_more_putters() {
        return !(putters.empty());
    }
    // called from single thread
    virtual void set_putters(std::set<int> putters) {
        this->putters = putters;
    }
    virtual void remove_putter(int putter)
    {
        cv.acquire();
        putters.erase(putter);
        cv.release();
    }
};

#endif /* #ifndef QUEUE_H */
