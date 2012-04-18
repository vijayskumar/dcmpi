#ifndef REFCOUNTED_H
#define REFCOUNTED_H

class RefCounted
{
    int count;
public:
    virtual ~RefCounted(){}
    RefCounted()
    {
        count = 0;
    }
    void addref() {
        count++;
//         std::cout << this << ": addref() to " << count << std::endl;
    }
    int getrefcount() { return count; }
    void delref() {
        count--;
//         std::cout << this << ": delref() to " << count << std::endl;
#ifdef DEBUG
        if (count < 0) {
            std::cerr << "ERROR: ref count went negative"
                      << " at " << __FILE__ << ":" << __LINE__
                      << std::endl << std::flush;
            exit(1);
        }
#endif
        if (count == 0) {
            delete this;
        }
    }
};


#endif /* #ifndef REFCOUNTED_H */
