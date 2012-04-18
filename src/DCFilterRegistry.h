/***************************************************************************
    $Id: DCFilterRegistry.h,v 1.25 2006/04/21 20:17:16 rutt Exp $
    Copyright (C) 2004 by BMI Department, The Ohio State University.
    For license info, see LICENSE in root directory of source distribution.
 ***************************************************************************/

#include "dcmpi_clib.h"

#include "dcmpi.h"

#include "dcmpi_util.h"
#include "dcmpi_dll.h"
#include "dcmpi_home.h"

class DCFilterRegistry : public DCSerializable
{
public:
    std::map<std::string, std::string> filter_to_libfile_unresolved;
    std::map<std::string, std::string> filter_to_libfile;

    std::vector<std::string> dcmpi_filter_path;

    typedef DCFilter * (*get_filter_func)(const char * filterName);
    typedef void (*get_contained_filters_func)(std::vector<std::string> & outval);

    DCFilterRegistry()
    {
        dcmpi_filter_path = get_dcmpi_filter_path();

//         // add default values here
//         std::string builtins = "libdcmpibuiltins.so";
//         std::string builtins_resolved = builtins;
//         resolve_library(builtins_resolved);
//         filter_to_libfile["dcmpi_recon_filter"] = builtins_resolved;
//         filter_to_libfile["dcmpi_availmem"] = builtins_resolved;
//         filter_to_libfile_unresolved["dcmpi_recon_filter"] = builtins;
//         filter_to_libfile_unresolved["dcmpi_availmem"] = builtins;
    }
    ~DCFilterRegistry()
    {
        ;
    }

    void serialize(DCBuffer * buf) const
    {
        buf->Append((int4)(filter_to_libfile_unresolved.size()));
        std::map<std::string, std::string>::const_iterator it;
        for (it = filter_to_libfile_unresolved.begin();
             it != filter_to_libfile_unresolved.end();
             it++) {
//             printf("%s %s\n", it->first.c_str(), it->second.c_str());
            buf->Append(it->first);
            buf->Append(it->second);
        }
    }
    void deSerialize(DCBuffer * buf)
    {
        int4 sz;
        std::string s1, s2;
        buf->Extract(&sz);
        for (int i = 0; i < sz; i++) {
            buf->Extract(&s1);
            buf->Extract(&s2);
            filter_to_libfile_unresolved[s1] = s2;
            resolve_library(s2);
//             printf("%s comes from %s\n", s1.c_str(), s2.c_str());
            filter_to_libfile[s1] = s2;
        }
        assert(dcmpi_is_local_console==false);
    }

    void resolve_library(std::string & libfile)
    {
        if (libfile[0] != '/') {
            // search for library in dcmpi_filter_path if not absolute path
            for (uint u = 0; u < dcmpi_filter_path.size(); u++) {
                std::string path = dcmpi_filter_path[u] + "/" + libfile;
                if (dcmpi_file_exists(path)) {
//                     std::cout << "converting " << libfile
//                               << " to " << path << std::endl;
                    libfile = path;
                    break;
                }
            }
        }
    }

    void add_filter_lib(const std::string & library_file)
    {
        assert(dcmpi_is_local_console);
        std::string libfile = library_file;
        resolve_library(libfile);
        // C++ filter library
        if (libfile.rfind(".so") == (libfile.size() - 3)) {
            std::vector<std::string> contained_filters;
            get_contained_filters_func fu =
                (get_contained_filters_func)dcmpi_dll_get_function(
                    libfile, "dcmpi_get_contained_filters");
            if (!fu) {
                std::cerr << "ERROR: dynamic library " << libfile
                          << " could not be loaded.  Perhaps it is either "
                          << "not a fully qualified path, or it is not "
                          << "contained in a directory listed in "
                          << "the DCMPI_FILTER_PATH environment variable, "
                          << "or it is not contained in a directory listed in "
                          << "the dcmpi_filter_path configuration file."
                          << std::endl << std::flush;
                exit(1);
            }
            else {
                fu(contained_filters);
                for (uint u = 0; u < contained_filters.size(); u++) {
                    std::string fn = contained_filters[u];
                    filter_to_libfile[fn] = libfile;
                    filter_to_libfile_unresolved[fn] = library_file;
                    if (dcmpi_verbose()) {
                        std::cout << "noticed filter "
                                  << fn
                                  << " from library "
                                  << libfile
                                  << std::endl;
                    }
                }
            }
        }
#ifdef DCMPI_HAVE_JAVA
        // Java filter library
        else if (libfile.rfind(".jar") == (libfile.size() - 4)) {
            // call method to see what filters this jar contains
            dcmpi_jni_mainthread_env_mutex.acquire();
            if (dcmpi_is_local_console) {
                init_java();
            }
            JNIEnv * env = dcmpi_jni_mainthread_env;

            char helper_class[256];
            strcpy(helper_class, "dcmpi/DCMPIJarClassLoader");
            jclass cls = env->FindClass(helper_class);
            if (!cls) {
                std::cerr << "ERROR:  could not find java class "
                          << helper_class
                          << " at " << __FILE__ << ":" << __LINE__
                          << std::endl << std::flush;
            }
            DCMPI_JNI_EXCEPTION_CHECK(env);

            // load up instance here
            jmethodID cid_constructor = env->GetMethodID(
                cls, "<init>", "(Ljava/lang/String;)V");
            DCMPI_JNI_EXCEPTION_CHECK(env);
            assert(cid_constructor);
            jstring s = env->NewStringUTF(libfile.c_str());
            assert(s);
            jobject obj =
                env->NewObject(cls, cid_constructor, s);
            if (env->ExceptionOccurred()) {
                std::cerr << "ERROR:  could not load file "
                          << libfile
                          << " at " << __FILE__ << ":" << __LINE__
                          << ", here is the Java exception:\n";
                env->ExceptionDescribe();
                exit(1);
            }

            jmethodID get_avail_filters = env->GetMethodID(
                cls, "getDCMPIFiltersAvailable", "()[Ljava/lang/String;");
            DCMPI_JNI_EXCEPTION_CHECK(env);
            
            jobjectArray str_array = (jobjectArray)
                env->CallObjectMethod(obj, get_avail_filters);
            DCMPI_JNI_EXCEPTION_CHECK(env);

            jsize len = env->GetArrayLength(str_array);
            jsize i;
            for (i = 0; i < len; i++) {
                jstring s =
                    (jstring)env->GetObjectArrayElement(
                        str_array, i);
                const char * s2 = env->GetStringUTFChars(s, NULL);
                filter_to_libfile[s2] = libfile;
                filter_to_libfile_unresolved[s2] = library_file;
                if (dcmpi_verbose()) {
                    std::cout << "noticed filter "
                              << s2
                              << " from library "
                              << libfile
                              << std::endl;
                }
                env->ReleaseStringUTFChars(s, s2);
            }
            dcmpi_jni_mainthread_env_mutex.release();
        }
#endif
#ifdef DCMPI_HAVE_PYTHON
        // python filter library
        else if (libfile.rfind(".py") == (libfile.size() - 3)) {
            // call method to see what filters this jar contains
            if (dcmpi_is_local_console) {
                init_python();
            }

            if (libfile[0] != '/') {
                std::cerr << "ERROR: could not locate python library file "
                          << library_file
                          << ", it isn't a fully qualified path and is not "
                          << "found on DCMPI_FILTER_PATH or dcmpi_filter_path "
                          << "configuration file."
                          << std::endl << std::flush;
                exit(1);
            }

            PyObject * modname = Py_BuildValue("s", "dcmpi_pyhelpers");
            PyObject * pmod = PyImport_Import(modname);
            assert(pmod);
            Py_DECREF(modname);
            PyObject * pfunc = PyObject_GetAttrString(
                pmod, "scan_library_for_filters");
            Py_DECREF(pmod);
            PyObject * pargs = Py_BuildValue("(s)", libfile.c_str());
            assert(pargs);
            PyObject * classes = PyEval_CallObject(pfunc, pargs);
            PYSHOWTRACEBACK();
            assert(classes);
            Py_DECREF(pfunc);
            Py_DECREF(pargs);
            
            int nclasses = PyObject_Length(classes);
            for (int ci = 0; ci < nclasses; ci++) {
                PyObject * nameo = PySequence_Fast_GET_ITEM(classes, ci);
                char * name = PyString_AS_STRING(nameo);
                filter_to_libfile[name] = libfile;
                filter_to_libfile_unresolved[name] = library_file;
                if (dcmpi_verbose()) {
                    std::cout << "noticed filter "
                              << name
                              << " from library "
                              << libfile
                              << std::endl;
                }
                Py_DECREF(nameo);
            }
            Py_DECREF(classes);
        }
#endif
        else {
            std::cerr << "ERROR: invalid filter library "
                      << library_file << ", use .so"
#ifdef DCMPI_HAVE_JAVA
                      << " or .jar"
#endif
#ifdef DCMPI_HAVE_PYTHON
                      << " or .py"
#endif
                      << " file."
                      << std::endl << std::flush;
            exit(1);
        }
    }

    DCFilter * get_filter(std::string filterName)
    {
         // special handling for <console> filter, which has no process()
         // method (it's effectively moved to the main() method that does the
         // layout and execute_start())
        if (filterName == "<console>")
        {
            return new DCFilter();
        }
        std::string libfile;
        if (filter_to_libfile.count(filterName) == 0) {
            char line[256];
            std::cerr << "ERROR (on " << dcmpi_get_hostname()
                      << ") : I don't know anything about a filter named "
                      << filterName << std::endl;
            std::cerr << "  These are the filters I know about:\n";
            std::map<std::string, std::string>::iterator it;
            for (it = filter_to_libfile.begin();
                 it != filter_to_libfile.end();
                 it++) {
                snprintf(line, sizeof(line),
                        "    %20s in library file %s\n",
                        it->first.c_str(), it->second.c_str());
                std::cerr << line;
            }
            std::cerr << "  To fix, use the method DCLayout::"
                      << "use_filter_library() or adjust your dcmpi_provideX() "
                      << "statement (in any C++ filter library you are "
                      << "building)\n";
            exit(1);
        }
        std::string library_file = filter_to_libfile[filterName];
        if (library_file.rfind(".so") == (library_file.size() - 3)) {
            get_filter_func func = (get_filter_func)
                dcmpi_dll_get_function(library_file, "dcmpi_get_filter");
            if (!func) {
                return NULL;
            }
            DCFilter * out;
            out = func(filterName.c_str());
            return out;
        }
#ifdef DCMPI_HAVE_JAVA
        else if (library_file.rfind(".jar") == (library_file.size() - 4)) {
            return new DCJavaFilter(filterName, library_file);
        }
#endif
#ifdef DCMPI_HAVE_PYTHON
        else if (library_file.rfind(".py") == (library_file.size() - 3)) {
            return new DCPyFilter(filterName, library_file);
        }
#endif
        else {
            return NULL;
        }
    }
};
