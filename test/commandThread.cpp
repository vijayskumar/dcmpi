/***************************************************************************
    $Id: commandThread.cpp,v 1.6 2006/10/31 10:02:00 rutt Exp $
    Copyright (C) 2004 by BMI Department, The Ohio State University.
    For license info, see LICENSE in root directory of source distribution.
 ***************************************************************************/

#include "dcmpi_util.h"

#include "dcmpi_home.h"

using namespace std;

int main(int argc, char * argv[])
{
    std::string home = dcmpi_get_home();
    cout << "home is " << home
         << ", hostname is '" << dcmpi_get_hostname()
         << "', shorthostname is '" << dcmpi_get_hostname(true)
         << "'" << endl;

    std::vector<std::string> ips = dcmpi_get_ip_addrs();
    std::copy(ips.begin(), ips.end(), ostream_iterator<string>(cout, " ")); cout << endl;
    
    DCCommandThread ct("sleep 1", true);
    ct.start();
    ct.join();
    std::string s1;
    s1 += "abc";
    s1 += '\4';
    s1 += '\0';
    std::cout << s1 << endl;

    DCBuffer b, b2;
    int i;
    b.pack("is", 4, "hello, world");
    std::cout << b  << endl;
    b.unpack("is", &i, &s1);
    std::cout << i << endl;
    std::cout << s1 << endl;
}
