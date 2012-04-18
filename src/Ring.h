/***************************************************************************
    $Id: Ring.h,v 1.2 2005/07/05 20:44:02 rutt Exp $
    Copyright (C) 2004 by BMI Department, The Ohio State University.
    For license info, see LICENSE in root directory of source distribution.
 ***************************************************************************/
#include <vector>
#include <list>

template <typename T>
class Ring
{
private:
    typename std::list<T> contents;
public:
    Ring(std::vector<T> items)
    {
        unsigned int i;
        for (i = 0; i < items.size(); i++) {
            contents.push_back(items[i]);
        }
    }
    T next()
    {
        if (contents.empty()) {
            std::cerr << "ERROR:  no items in ring, "
                      << "but the next method was called"
                      << " at " << __FILE__ << ":" << __LINE__
                      << std::endl << std::flush;
            exit(1);
        }
        T outval = contents.front();
        contents.pop_front();
        contents.push_back(outval);
        return outval;
    }
    int size()
    {
        return (int)contents.size();
    }
};
