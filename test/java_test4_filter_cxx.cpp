#include <dcmpi.h>

#include <unistd.h>

using namespace std;

std::string get_hostname(bool force_short_name=false)
{
    std::string out;
    char localhost[1024];
    gethostname(localhost, sizeof(localhost));
    localhost[sizeof(localhost)-1] = 0;
    out = localhost;
    if (force_short_name) {
       std::string::size_type dotpos = out.find(".");
       if (dotpos != std::string::npos) {
           out.erase(dotpos, out.size() - dotpos);
       }
    }
    return out;
}

class java_test4_filter_cxx : public DCFilter
{
public:
    int process() {
        std::string hostname = get_hostname();
        std::string mode = get_param("mode");
        std::string myadd = "hello (C++) from " + hostname + "\\";
        int numfilters = atoi(get_param("numfilters").c_str());
        if (numfilters == 1) {
            cout << "NOPping (C++) for 1 filter\n";
            return 0;
        }
        std::string s;
        if (mode == "inonly" || mode == "inout") {
            DCBuffer * in = read("0");
            in->Extract(&s);
            cout << hostname << " (C++): got " << s << endl;
            s += myadd;
        }
        else {
            s = myadd;
        }
        fflush(stdout);
        sleep(2);
        if (mode == "outonly" || mode == "inout") {
            DCBuffer * out;
            out = new DCBuffer();
            out->Append(s);
            write(out, "0");
        }

        return 0;
    }
};

dcmpi_provide1(java_test4_filter_cxx);
