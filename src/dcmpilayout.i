%module dcmpilayout
%{
#include "dcmpi.h"
%}

%include "dcmpi_typedefs.h"
%include "std_string.i"

class DCFilterInstance
{
public:
    DCFilterInstance(std::string filtername,
                     std::string instancename);
    ~DCFilterInstance();

    // explicit filter directives
    void bind_to_host(const std::string & host);
    void add_label(const std::string & label);

    // transparent
    void make_transparent(int num_copies);

    void set_param(std::string key, std::string val);
    bool has_param(std::string key);
    std::string get_param(const std::string & key);
};

class DCLayout
{
public:
    DCLayout();
    ~DCLayout();
    void use_filter_library(const std::string & library_name);
    void set_param_all(std::string key, std::string value);
    void add(DCFilterInstance * filter_instance);
    void add_port(DCFilterInstance * filter1, std::string port1,
                  DCFilterInstance * filter2, std::string port2);
    void set_exec_host_pool_file(const char * hostfile); // read hosts from
                                                         // file, 1 host per
                                                         // line
    void set_exec_host_pool(const std::vector<std::string> & hosts);
    int execute();
    int execute_finish();
};
