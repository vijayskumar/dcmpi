#include "dcmpi.h"

class cxx_test3_receiver : public DCFilter {
public:
    int process() {
        printf("receiver starting\n");
        DCBuffer * b = read("0");
        printf("the numbers I got in c++ are:\n");
        for (int i = 0; i < 3; i++) {
            int4 val;
            b->Extract(&val);
            printf("%d\n", val);
        }
        b->consume();
        return 0;
    }
};

dcmpi_provide1(cxx_test3_receiver);
