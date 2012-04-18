#include "dcmpi_backtrace.h"
#include <dcmpi.h>

using namespace std;

void f2(void)
{
    dcmpi_print_backtrace();
}

void f1(void)
{
    f2();
}

int main(int argc, char * argv[])
{
    f1();
}
