#ifndef DCMPI_BACKTRACE_H
#define DCMPI_BACKTRACE_H

#include "dcmpi_internal.h"
#include "dcmpi.h"

#ifdef DCMPI_HAVE_BACKTRACE

#include <execinfo.h>
#include <stdio.h>
#include <stdlib.h>

#include <iostream>
#include <string>
#include <vector>

/* Obtain a backtrace and print it to `stdout'. */
inline void dcmpi_print_backtrace(FILE * outf=stdout, bool use_addr2line=true)
{
    void *array[30];
    size_t size;
    char **strings;
    size_t i;
    std::string addr2line;
    if (use_addr2line) {
        addr2line = dcmpi_find_executable("addr2line", false);
    }
    char ptr[64];
    char fn_lineno[PATH_MAX];

    size = backtrace (array, 30);
    strings = backtrace_symbols (array, size);

    if (size > 40) {
        std::cout << "frames trimmed down to 40 from " << size << std::endl;
        size = 40;
    }
    fprintf(outf, "begin stack trace of %zd stack frames.\n", size);

    for (i = 0; i < size; i++) {
        std::string lineno = "\n";
        if (!addr2line.empty()) {
            sprintf(ptr, "%p", array[i]);
            std::vector<std::string> bin_addr = dcmpi_string_tokenize(
                strings[i], "[");
            std::vector<std::string> bin_func = dcmpi_string_tokenize(
                bin_addr[0], "(");
            std::string bin = bin_func[0];
            dcmpi_string_trim(bin);
            std::string cmd = addr2line + " --exe=" + bin + " " + ptr;
            FILE * output = popen(cmd.c_str(), "r");
            fgets(fn_lineno, sizeof(fn_lineno), output);
            lineno += fn_lineno;
            dcmpi_string_trim(lineno);
            lineno = " (" + lineno + ")\n";
            pclose(output);
        }
        fprintf(outf, "    %s%s", strings[i], lineno.c_str());
    }
    free(strings);
}

#else
inline void dcmpi_print_backtrace (FILE * outf=stdout)
{
    std::cerr << "WARNING: cannot use dcmpi_print_backtrace() on a non-GNU system"
              << " at " << __FILE__ << ":" << __LINE__
              << std::endl << std::flush;
}
#endif

#endif /* #ifndef DCMPI_BACKTRACE_H */
