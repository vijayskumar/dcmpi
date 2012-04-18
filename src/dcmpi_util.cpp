/***************************************************************************
    $Id: dcmpi_util.cpp,v 1.28 2006/10/31 12:49:28 rutt Exp $
    Copyright (C) 2004 by BMI Department, The Ohio State University.
    For license info, see LICENSE in root directory of source distribution.
 ***************************************************************************/

#include "dcmpi_clib.h"

#include "dcmpi.h"

#include "dcmpi_util.h"

using namespace std;

std::map<std::string, std::string> file_to_pairs(FILE * f)
{
    std::map<std::string, std::string> out;
    char buf[1024];
    assert(f);
    // force rewind
    if (fseek(f, 0, SEEK_SET) == -1) {
        std::cerr << "ERROR:  calling fseeko()"
                  << " at " << __FILE__ << ":" << __LINE__
                  << std::endl << std::flush;
        exit(1);
    }
    while (fgets(buf, sizeof(buf), f) != NULL) {
        buf[strlen(buf)-1] = 0;
        if (buf[0] != '#') {
            std::string linecp = buf;
            int len = strlen(buf);
            char * space = strchr(buf, ' ');
            if (!space) {
                std::cerr << "ERROR: parsing line '" << linecp
                          << "' at " << __FILE__ << ":" << __LINE__
                          << std::endl << std::flush;
                exit(1);
            }
            *space = 0;
            std::string key = buf;
            dcmpi_string_trim_front(key);
            space++;
            while (*space == ' ') {
                space++;
            }
            if (space == &buf[len]) {
                std::cerr << "ERROR: parsing line '" << linecp
                          << " at " << __FILE__ << ":" << __LINE__
                          << std::endl << std::flush;
                exit(1);
            }
            std::string val = space;
            dcmpi_string_trim_rear(val);
            out[key] = val;
        }
    }
    return out;
}

std::map<std::string, std::string> file_to_pairs(std::string filename)
{
    FILE * f = fopen(filename.c_str(), "r");
    if (!f) {
        std::cerr << "ERROR: opening file " << filename
                  << " at " << __FILE__ << ":" << __LINE__
                  << std::endl << std::flush;
        exit(1);
    }
    std::map<std::string, std::string> out = file_to_pairs(f);
    fclose(f);
    return out;
}

void pairs_to_file(std::map<std::string, std::string> pairs,
                   std::string filename)
{
    FILE * f = fopen(filename.c_str(), "w");
    if (!f) {
        std::cerr << "ERROR: opening file " << filename
                  << " at " << __FILE__ << ":" << __LINE__
                  << std::endl << std::flush;
        exit(1);
    }
    std::map<std::string, std::string>::iterator it;
    for (it = pairs.begin();
         it != pairs.end();
         it++) {
        if (fwrite(it->first.c_str(), it->first.size(), 1, f) < 1) {
            std::cerr << "ERROR: calling fwrite()"
                      << " at " << __FILE__ << ":" << __LINE__
                      << std::endl << std::flush;
            exit(1);
        }
        if (fwrite(" ", 1, 1, f) < 1) {
            std::cerr << "ERROR: calling fwrite()"
                      << " at " << __FILE__ << ":" << __LINE__
                      << std::endl << std::flush;
            exit(1);
        }
        if (fwrite(it->second.c_str(), it->second.size(), 1, f) < 1) {
            std::cerr << "ERROR: calling fwrite()"
                      << " at " << __FILE__ << ":" << __LINE__
                      << std::endl << std::flush;
            exit(1);
        }
        if (fwrite("\n", 1, 1, f) < 1) {
            std::cerr << "ERROR: calling fwrite()"
                      << " at " << __FILE__ << ":" << __LINE__
                      << std::endl << std::flush;
            exit(1);
        }
    }
    fclose(f);
}

std::string dcmpi_file_read_line(FILE * f) // returns non-stripped (has \n)
{
    char c;
    std::string str;
    while (1) {
        if (fread(&c, 1, 1, f) < 1) {
            if (feof(f)) {
                return str;
            }
            else if (ferror(f)) {
                std::cerr << "ERROR: errno=" << errno << " calling fread()"
                          << " at " << __FILE__ << ":" << __LINE__
                          << std::endl << std::flush;
                exit(1);
            }
            else {
                assert(0);
            }
        }
        str.push_back(c);
        if (c=='\n') {
            break;
        }
    }
    return str;
}

std::string shell_quote(const std::string & s)
{
    std::string result = "";
    uint u = s.size();
    const char * s_c = s.c_str();
    for (u = 0; u < s.size(); u++) {
        const char c = s_c[u];
        if ((c >= '0' && c <= '9') ||
            (c >= 'A' && c <= 'Z') ||
            (c >= 'a' && c <= 'z')) {
            result += c;
        }
        else {
            ostr o;
            o << "=" << hex << setw(2) << setfill('0')
              << (int)((unsigned char)(c));
            result += o.str();
        }
    }
    return result;
}
 
std::string shell_unquote(const std::string & s)
{
    std::string result;
    if (!s.empty()) {
        std::vector<std::string> toks = dcmpi_string_tokenize(s, "=");
        result = toks[0];
        for (uint u = 1; u < toks.size(); u++) {
            std::string s2 = toks[u];
            std::string hex = s2.substr(0, 2);
            result += strtol(hex.c_str(), NULL, 16);
            result += s2.substr(2);
        }
    }
    return result;
}

int DC_read_all(int fd, void *buf, size_t count, int * hitEOF)
{
    ssize_t retcode;
    size_t  offset = 0;
    size_t  bytes_read;
    int     rc = 0;

    if (count == 0) {
        return 0;
    }

    if (hitEOF) {
        *hitEOF = 0;
    }

    while (1) {
        retcode = read(fd, (unsigned char*)buf + offset, count);
        if (retcode < 0) {
            rc = errno;
            goto Exit;
        }
        else if (retcode == 0) {
            if (hitEOF)
                *hitEOF = 1;
            rc = (errno != 0)?errno:-1;
            break;
        }
        bytes_read = (size_t)retcode;
        if (count == bytes_read) /* write complete */
            break;
        count -= bytes_read;
        offset += bytes_read;
    }

Exit: 
    return rc;
}

int DC_write_all(int fd, const void * buf, size_t count)
{
    ssize_t retcode;
    size_t  offset = 0;
    size_t  bytes_written;
    int     rc = 0;

    if (count == 0) {
        return 0;
    }

    while (1)
    {
        retcode = write(fd, (unsigned char*)buf + offset, count);
        if (retcode < 0)
        {
            rc = errno;
            if (rc == 0) /* another thread set errno??? */
                assert(0); // rc = EIO;
            goto Exit;
        }
        bytes_written = (size_t)retcode;
        if (count == bytes_written) /* write complete */
            break;
        count -= bytes_written;
        offset += bytes_written;
    }
Exit: 
    return rc;
}

std::string dcmpi_socket_read_line(int s) // returns stripped (no \n)
{
    char c;
    std::string str;
    while (1) {
        int rc = DC_read_all(s, &c, 1);
        if (rc) {
            std::cerr << "ERROR: rc=" << rc
                      << " at " << __FILE__ << ":" << __LINE__
                      << " on " << dcmpi_get_hostname()
                      << std::endl << std::flush;
            dcmpi_print_backtrace();
            assert(0);
        }
        if (c=='\n') {
            break;
        }
        str.push_back(c);
    }
    return str;
}

// message shouldn't contain \n
void dcmpi_socket_write_line(int s, std::string message)
{
    if (!dcmpi_string_ends_with(message, "\n")) {
        message += "\n";
    }
    if (DC_write_all(s, message.c_str(), message.size()) != 0) {
        std::cerr << "ERRORE: writing " << message
                  << " at " << __FILE__ << ":" << __LINE__
                  << std::endl << std::flush;
        exit(1);
    }
}

inline static void printSpaces(int len)
{
    printf("%*s", len, "");
}

void dcmpi_bash_select_emulator(
    int                              indentSpaces,
    const std::vector<std::string> & inChoices,
    const std::string &              description,
    const std::string &              exitDescription,
    bool                             allowMultipleChoices,
    bool                             allowDuplicates,
    std::vector<std::string> &       outChoices,
    std::vector<int> &               outIndexes)
{
    size_t linesz = 1024;
    char * line = new char[linesz];
    char * runner;
    char * numStart;
    unsigned inChoicesLen;
    int widthCounter;
    int widthNumbers = 1;
    unsigned loop;

    printSpaces(indentSpaces);
    printf("%s", description.c_str());

    inChoicesLen = inChoices.size();
    widthCounter = (int)inChoicesLen;
    while ((widthCounter / 10) > 0) {
        widthNumbers++;
        widthCounter /= 10;
    }

    if (inChoicesLen < 20) {
        /* single-column */
        for (loop = 0; loop < inChoicesLen; loop++)
        {
            printSpaces(indentSpaces);
            printf("%*u) %s\n", widthNumbers, (loop+1),inChoices[loop].c_str());
        }
    }
    else {
        /* two-column */

        /* find max width of column 1 */
        int maxCol1 = strlen(exitDescription.c_str());
        char buf[256];

        /* just discover maximum width of the choices */
        for (loop = 0; loop < (inChoicesLen / 2); loop++)
        {
            snprintf(buf, sizeof(buf) - 1, "%*u) %s",
                     widthNumbers, loop+1, inChoices[loop].c_str());
            if (((int)(strlen(buf))) > maxCol1)
                maxCol1 = strlen(buf);
        }

        for (loop = 0; loop < (inChoicesLen / 2); loop++)
        {
            printSpaces(indentSpaces);

            /* print first column */
            snprintf(buf, sizeof(buf) - 1, "%*u) %s", widthNumbers,
                     loop+1, inChoices[loop].c_str());
            printf("%-*s", maxCol1, buf);

            /* gap between columns */
            printf("   ");

            /* print second column */
            printf("%*u) %s\n", widthNumbers, loop + (inChoicesLen/2) + 1,
                   inChoices[loop + (inChoicesLen/2)].c_str());
        }
        if (inChoicesLen % 2) {
            printSpaces(indentSpaces);
            printf("%-*s", maxCol1, " ");

            /* gap between columns */
            printf("   ");

            printf("%*u) %s\n", widthNumbers, loop + (inChoicesLen/2) + 1,
                   inChoices[loop + (inChoicesLen/2)].c_str());
        }
    }
    printSpaces(indentSpaces);
    printf("%*s) %s\n", widthNumbers, "x", exitDescription.c_str());

    bool choosing = true;
    while (choosing) {

        outChoices.clear();
        outIndexes.clear();

        printf("\n");
        printSpaces(indentSpaces);
        printf("Enter choice%s: ",
               allowMultipleChoices
               ?" (separate multiple choices with a space)"
               :"");

        if (fgets(line, linesz - 1, stdin) == NULL) {
            return;
        }
        printf("\n");
        std::string tr = line;
        dcmpi_string_trim(tr);
        char * trimmed = StrDup(tr.c_str());
        if (strcmp(trimmed, "x") == 0) {
            delete[] trimmed;
            goto Exit;
        }

        /* validate input */
        runner = trimmed;
        bool valid = false;
        while (*runner) {
            if (isdigit(*runner)) {
                valid = true;
            }
            else if (allowMultipleChoices && (*runner == ' ')) {
                valid = true;
            }
            else {
                valid = false;
                break;
            }
            runner++;
        }
        if (!valid) {
            fprintf(stderr, "ERROR:  invalid input %s\n", trimmed);
            delete[] trimmed;
            continue;
        }

        bool numbersInRange = true;
        runner = trimmed;
        while (*runner) {
            int v;
            numStart = runner;
            while (*runner && (*runner != ' ')) {
                runner++;
            }
            if (*runner == 0) {
                v = atoi(numStart);
            }
            else {
                *runner = 0;
                v = atoi(numStart);
                runner++;
            }
            if ((v > (int)inChoicesLen) || (v < 1)) {
                fprintf(stderr, "ERROR:  argument %d out of range\n", v);
                numbersInRange = false;
                break;
            }
            outIndexes.push_back(v - 1);
        }

        if (numbersInRange) {
            bool duplicateProblem = false;
            if (!allowDuplicates) {
                /* test for duplicates */
                set<int> intSet;
                unsigned len = outIndexes.size();
                for (loop = 0; loop < len; loop++) {
                    intSet.insert(outIndexes[loop]);
                }
                if (intSet.size() != len) {
                    fprintf(stderr, "ERROR:  duplicate selections "
                            "not allowed\n");
                    duplicateProblem = true;
                }
            }
            if (!duplicateProblem) {
                unsigned len = outIndexes.size();
                for (loop = 0; loop < len; loop++) {
                    outChoices.push_back(inChoices[outIndexes[loop]]);
                }
                choosing = false;
            }
        }
        delete[] trimmed;
    }

Exit:
    delete[] line;
}

#ifdef DCMPI_HAVE_JAVA

int DCJavaFilter::init()
{
    int res;
    static DCMutex m;
    m.acquire();
    init_java();
    m.release();

    // attach to thread here
    res = dcmpi_jni_invocation_jvm->AttachCurrentThread(
        (void**)&separate_thread_env, NULL);
    if (res < 0) {
        std::cerr << "ERROR:  could not attach to jvm"
                  << " at " << __FILE__ << ":" << __LINE__
                  << std::endl << std::flush;
        exit(1);
    }
    JNIEnv * env = separate_thread_env;

    if (dcmpi_verbose()) {
        cout <<  "JNI: loading dcmpi/Loader\n";
    }
    
    char helper_class[256];
    strcpy(helper_class, "dcmpi/Loader");
    cls = env->FindClass(helper_class);
    if (!cls) {
        std::cerr << "ERROR:  could not find java class "
                  << helper_class
                  << " at " << __FILE__ << ":" << __LINE__
                  << std::endl << std::flush;
    }
    DCMPI_JNI_EXCEPTION_CHECK(env);

    // load up instance here
    jmethodID cid_constructor = env->GetMethodID(
        cls, "<init>", "(Ljava/lang/String;Ljava/lang/String;)V");
    DCMPI_JNI_EXCEPTION_CHECK(env);
    assert(cid_constructor);
    jstring s1 = env->NewStringUTF(lib_name.c_str());
    jstring s2 = env->NewStringUTF(class_name.c_str());
    assert(s1 && s2);
    obj = env->NewObject(cls, cid_constructor, s1, s2);
    DCMPI_JNI_EXCEPTION_CHECK(env);

    if (dcmpi_verbose()) {
        cout << "JNI: loading filter " << class_name << " from " << lib_name
             << "\n";
    }
    jmethodID loader = env->GetMethodID(
        cls, "load", "()Ljava/lang/Object;");
    assert(loader);
    jobject filter = env->CallObjectMethod(obj, loader);
    obj = filter;

    cls = env->GetObjectClass(obj);
    DCMPI_JNI_EXCEPTION_CHECK(env);
    assert(cls);

    // call handoff method
    jmethodID cid_handoff = env->GetMethodID(cls, "handoff", "(J)V");
    if (!cid_handoff) {
        std::cerr << "ERROR:  could not find method 'handoff' for class "
                  << class_name << ", does it extend class dcmpi.DCFilter?"
                  << " at " << __FILE__ << ":" << __LINE__
                  << std::endl << std::flush;
        exit(1);
    }
    DCMPI_JNI_EXCEPTION_CHECK(env);
    env->CallVoidMethod(obj, cid_handoff,
                        DCMPI_JNI_CAST_FROM_POINTER_TO_JLONG(this));
    DCMPI_JNI_EXCEPTION_CHECK(env);
    
    if (dcmpi_verbose()) {
        cout << "JNI: calling init method for filter " << class_name
             << "\n";
    }

    // call init method
    jmethodID init_method = env->GetMethodID(cls, "init", "()I");
    DCMPI_JNI_EXCEPTION_CHECK(env);
    assert(init_method);
    res = (int)(env->CallIntMethod(obj, init_method));
    DCMPI_JNI_EXCEPTION_CHECK(env);
    return res;
}

int DCJavaFilter::process()
{
    int res;

    JNIEnv * env = separate_thread_env;

    // call process method
    jmethodID process_method = env->GetMethodID(cls, "process", "()I");
    DCMPI_JNI_EXCEPTION_CHECK(env);
    res = (int)(env->CallIntMethod(obj, process_method));
    DCMPI_JNI_EXCEPTION_CHECK(env);
    return res;
}

int DCJavaFilter::fini()
{
    int res;

    JNIEnv * env = separate_thread_env;

    // call fini method
    jmethodID fini_method =
        env->GetMethodID(cls, "fini", "()I");
    DCMPI_JNI_EXCEPTION_CHECK(env);
    res = (int)(env->CallIntMethod(obj, fini_method));
    DCMPI_JNI_EXCEPTION_CHECK(env);
    env->DeleteLocalRef(obj);
    env->DeleteLocalRef(cls);
    dcmpi_jni_invocation_jvm->DetachCurrentThread();
    return res;
}
#endif

#ifdef DCMPI_HAVE_PYTHON
int DCPyFilter::init()
{
    static DCMutex m;
    m.acquire();
    init_python();
    m.release();

    int res = 0;
    PyInterpreterState * mainInterpreterState = dcmpi_pyMainThreadState->interp;
    myThreadState = PyThreadState_New(mainInterpreterState);
    PyEval_AcquireLock();
    PyThreadState_Swap(myThreadState);

    PyObject * modname = Py_BuildValue("s", "dcmpi_pyhelpers");
    PyObject * pmod = PyImport_Import(modname);
    assert(pmod);
    Py_DECREF(modname);
    PyObject * pfunc = PyObject_GetAttrString(pmod, "load_filter");
    Py_DECREF(pmod);
    PyObject * pargs = Py_BuildValue(
        "(ss)", class_name.c_str(), lib_name.c_str());
    assert(pargs);
    pyclass = PyEval_CallObject(pfunc, pargs);
    assert(pyclass);
    Py_DECREF(pfunc);
    Py_DECREF(pargs);

    pfunc = PyObject_GetAttrString(pyclass, "_handoff");
    assert(pfunc);
    PyObject * peer = PyCObject_FromVoidPtr(this, NULL);
    pargs = Py_BuildValue("(O)", peer);
    PyObject * out = PyEval_CallObject(pfunc, pargs);
    Py_DECREF(pfunc);
    Py_DECREF(pargs);
    Py_XDECREF(out);
    
    PyObject * method_init = PyObject_GetAttrString(pyclass, "init");
    assert (method_init);
    pargs = Py_BuildValue("()");
    PyObject * ires = PyEval_CallObject(method_init, pargs);
    PYSHOWTRACEBACK();
    Py_DECREF(method_init);
    Py_DECREF(pargs);
    Py_DECREF(ires);

    PyThreadState_Swap(NULL);
    PyEval_ReleaseLock();
    return res;
}

int DCPyFilter::process()
{
    int res = 0;

    PyEval_AcquireLock();
    PyThreadState_Swap(myThreadState);

    PyObject * method_process = PyObject_GetAttrString(pyclass, "process");
    assert(method_process);
    PyObject * pargs = Py_BuildValue("()");
    PyObject * ires = PyEval_CallObject(method_process, pargs);
    PYSHOWTRACEBACK();
    Py_DECREF(method_process);
    Py_DECREF(pargs);
    Py_DECREF(ires);

    PyThreadState_Swap(NULL);
    PyEval_ReleaseLock();

    return res;
}

int DCPyFilter::fini()
{
    int res = 0;

    PyEval_AcquireLock();
    PyThreadState_Swap(myThreadState);

    PyObject * method_fini = PyObject_GetAttrString(pyclass, "fini");
    assert (method_fini);
    PyObject * pargs = Py_BuildValue("()");
    PyObject * ires = PyEval_CallObject(method_fini, pargs);
    PYSHOWTRACEBACK();
    Py_DECREF(method_fini);
    Py_DECREF(pargs);
    Py_DECREF(ires);
    
    assert(pyclass);
    Py_DECREF(pyclass);
    
    PyThreadState_Swap(NULL);
    PyThreadState_Clear(myThreadState);
    PyEval_ReleaseLock();
    PyThreadState_Delete(myThreadState);
    return res;
}
#endif
