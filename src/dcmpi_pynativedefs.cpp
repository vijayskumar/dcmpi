#include <Python.h>
#include <cobject.h>
#include <dcmpi.h>

using namespace std;

/* Needed for Python versions earlier than 2.3. */
#ifndef PyMODINIT_FUNC
#define PyMODINIT_FUNC DL_EXPORT(void)
#endif 

// begin DCBuffer methods

static void DCBuffer_destroy(void * obj)
{
//     printf("DCBuffer_destroy: called on %p\n", obj);
    delete (DCBuffer*)obj;
}

static PyObject * DCBuffer_new(PyObject * self, PyObject * args)
{
    int sz = -1;
    int arglen = PyTuple_Size(args);
    if (arglen == 1) {
        if (!PyArg_ParseTuple(args, "i", &sz)) {
            std::cerr << "ERROR: calling DCBuffer_new"
                      << " at " << __FILE__ << ":" << __LINE__
                      << std::endl << std::flush;
            exit(1);
        }
    }
    DCBuffer * out;
    if (sz == -1) {
        out = new DCBuffer;
    }
    else {
        out = new DCBuffer(sz);
    }
    return PyCObject_FromVoidPtr(out, DCBuffer_destroy);
}

static PyObject * DCBuffer_appendint(PyObject * self, PyObject * args)
{
    int arglen = PyTuple_Size(args);
    if (arglen != 2) {
        std::cerr << "ERROR:"
                  << " at " << __FILE__ << ":" << __LINE__
                  << std::endl << std::flush;
        exit(1);
    }
    PyObject * b1;
    int val;
    if (!PyArg_ParseTuple(args, "Oi", &b1, &val)) {
        std::cerr << "ERROR:"
                  << " at " << __FILE__ << ":" << __LINE__
                  << std::endl << std::flush;
        exit(1);
    }
    DCBuffer * b2 = (DCBuffer*)PyCObject_AsVoidPtr(b1);
    b2->Append((int4)val);
    return Py_BuildValue("");
}

static PyObject * DCBuffer_extractint(PyObject * self, PyObject * args)
{
    int arglen = PyTuple_Size(args);
    if (arglen != 1) {
        std::cerr << "ERROR:"
                  << " at " << __FILE__ << ":" << __LINE__
                  << std::endl << std::flush;
        exit(1);
    }
    PyObject * b1;
    if (!PyArg_ParseTuple(args, "O", &b1)) {
        std::cerr << "ERROR:"
                  << " at " << __FILE__ << ":" << __LINE__
                  << std::endl << std::flush;
        exit(1);
    }
    DCBuffer * b2 = (DCBuffer*)PyCObject_AsVoidPtr(b1);
    int4 out;
    b2->Extract(&out);
    return PyInt_FromLong((long)out);
}

static PyObject * DCBuffer_appendstr(PyObject * self, PyObject * args)
{
    int arglen = PyTuple_Size(args);
    if (arglen != 2) {
        std::cerr << "ERROR:"
                  << " at " << __FILE__ << ":" << __LINE__
                  << std::endl << std::flush;
        exit(1);
    }
    PyObject * b1;
    char * val;
    if (!PyArg_ParseTuple(args, "Os", &b1, &val)) {
        std::cerr << "ERROR:"
                  << " at " << __FILE__ << ":" << __LINE__
                  << std::endl << std::flush;
        exit(1);
    }
    DCBuffer * b2 = (DCBuffer*)PyCObject_AsVoidPtr(b1);
    b2->Append(std::string(val));
    return Py_BuildValue("");
}

static PyObject * DCBuffer_extractstr(PyObject * self, PyObject * args)
{
    int arglen = PyTuple_Size(args);
    if (arglen != 1) {
        std::cerr << "ERROR:"
                  << " at " << __FILE__ << ":" << __LINE__
                  << std::endl << std::flush;
        exit(1);
    }
    PyObject * b1;
    if (!PyArg_ParseTuple(args, "O", &b1)) {
        std::cerr << "ERROR:"
                  << " at " << __FILE__ << ":" << __LINE__
                  << std::endl << std::flush;
        exit(1);
    }
    DCBuffer * b2 = (DCBuffer*)PyCObject_AsVoidPtr(b1);
    std::string out;
    b2->Extract(&out);
    return PyString_FromString(out.c_str());
}

static PyObject * DCBuffer_appendfloat(PyObject * self, PyObject * args)
{
    int arglen = PyTuple_Size(args);
    if (arglen != 2) {
        std::cerr << "ERROR:"
                  << " at " << __FILE__ << ":" << __LINE__
                  << std::endl << std::flush;
        exit(1);
    }
    PyObject * b1;
    float val;
    if (!PyArg_ParseTuple(args, "Of", &b1, &val)) {
        std::cerr << "ERROR:"
                  << " at " << __FILE__ << ":" << __LINE__
                  << std::endl << std::flush;
        exit(1);
    }
    DCBuffer * b2 = (DCBuffer*)PyCObject_AsVoidPtr(b1);
    b2->Append(val);
    return Py_BuildValue("");
}

static PyObject * DCBuffer_extractfloat(PyObject * self, PyObject * args)
{
    int arglen = PyTuple_Size(args);
    if (arglen != 1) {
        std::cerr << "ERROR:"
                  << " at " << __FILE__ << ":" << __LINE__
                  << std::endl << std::flush;
        exit(1);
    }
    PyObject * b1;
    if (!PyArg_ParseTuple(args, "O", &b1)) {
        std::cerr << "ERROR:"
                  << " at " << __FILE__ << ":" << __LINE__
                  << std::endl << std::flush;
        exit(1);
    }
    DCBuffer * b2 = (DCBuffer*)PyCObject_AsVoidPtr(b1);
    float out;
    b2->Extract(&out);
    return PyFloat_FromDouble((double)out);
}

static PyObject * DCBuffer_appenddouble(PyObject * self, PyObject * args)
{
    int arglen = PyTuple_Size(args);
    if (arglen != 2) {
        std::cerr << "ERROR:"
                  << " at " << __FILE__ << ":" << __LINE__
                  << std::endl << std::flush;
        exit(1);
    }
    PyObject * b1;
    double val;
    if (!PyArg_ParseTuple(args, "Od", &b1, &val)) {
        std::cerr << "ERROR:"
                  << " at " << __FILE__ << ":" << __LINE__
                  << std::endl << std::flush;
        exit(1);
    }
    DCBuffer * b2 = (DCBuffer*)PyCObject_AsVoidPtr(b1);
    b2->Append(val);
    return Py_BuildValue("");
}

static PyObject * DCBuffer_extractdouble(PyObject * self, PyObject * args)
{
    int arglen = PyTuple_Size(args);
    if (arglen != 1) {
        std::cerr << "ERROR:"
                  << " at " << __FILE__ << ":" << __LINE__
                  << std::endl << std::flush;
        exit(1);
    }
    PyObject * b1;
    if (!PyArg_ParseTuple(args, "O", &b1)) {
        std::cerr << "ERROR:"
                  << " at " << __FILE__ << ":" << __LINE__
                  << std::endl << std::flush;
        exit(1);
    }
    DCBuffer * b2 = (DCBuffer*)PyCObject_AsVoidPtr(b1);
    double out;
    b2->Extract(&out);
    return PyFloat_FromDouble(out);
}

static PyObject * DCBuffer_appendbytearray(PyObject * self, PyObject * args)
{
    int arglen = PyTuple_Size(args);
    if (arglen != 2) {
        std::cerr << "ERROR:"
                  << " at " << __FILE__ << ":" << __LINE__
                  << std::endl << std::flush;
        exit(1);
    }
    PyObject * b1;
    char * val;
    int len;
    if (!PyArg_ParseTuple(args, "Os#", &b1, &val, &len)) {
        std::cerr << "ERROR:"
                  << " at " << __FILE__ << ":" << __LINE__
                  << std::endl << std::flush;
        exit(1);
    }
    DCBuffer * b2 = (DCBuffer*)PyCObject_AsVoidPtr(b1);
    b2->Append(val, len);
    return Py_BuildValue("");
}

static PyObject * DCBuffer_getraw(PyObject * self, PyObject * args)
{
    int arglen = PyTuple_Size(args);
    if (arglen != 1) {
        std::cerr << "ERROR:"
                  << " at " << __FILE__ << ":" << __LINE__
                  << std::endl << std::flush;
        exit(1);
    }
    PyObject * b1;
    if (!PyArg_ParseTuple(args, "O", &b1)) {
        std::cerr << "ERROR:"
                  << " at " << __FILE__ << ":" << __LINE__
                  << std::endl << std::flush;
        exit(1);
    }
    DCBuffer * b2 = (DCBuffer*)PyCObject_AsVoidPtr(b1);
    return PyString_FromStringAndSize(b2->getPtr(),
                                      b2->getUsedSize());
}

static PyObject * DCBuffer_getrawextract(PyObject * self, PyObject * args)
{
    int arglen = PyTuple_Size(args);
    if (arglen != 1) {
        std::cerr << "ERROR:"
                  << " at " << __FILE__ << ":" << __LINE__
                  << std::endl << std::flush;
        exit(1);
    }
    PyObject * b1;
    if (!PyArg_ParseTuple(args, "O", &b1)) {
        std::cerr << "ERROR:"
                  << " at " << __FILE__ << ":" << __LINE__
                  << std::endl << std::flush;
        exit(1);
    }
    DCBuffer * b2 = (DCBuffer*)PyCObject_AsVoidPtr(b1);
    return PyString_FromStringAndSize(b2->getPtrExtract(),
                                      b2->getExtractAvailSize());
}

static PyObject * DCBuffer_setusedsize(PyObject * self, PyObject * args)
{
    int arglen = PyTuple_Size(args);
    if (arglen != 2) {
        std::cerr << "ERROR:"
                  << " at " << __FILE__ << ":" << __LINE__
                  << std::endl << std::flush;
        exit(1);
    }
    PyObject * b1;
    int len;
    if (!PyArg_ParseTuple(args, "Oi", &b1, &len)) {
        std::cerr << "ERROR:"
                  << " at " << __FILE__ << ":" << __LINE__
                  << std::endl << std::flush;
        exit(1);
    }
    DCBuffer * b2 = (DCBuffer*)PyCObject_AsVoidPtr(b1);
    b2->setUsedSize(len);
    return Py_BuildValue("");
}

static PyObject * DCBuffer_getusedsize(PyObject * self, PyObject * args)
{
    int arglen = PyTuple_Size(args);
    if (arglen != 1) {
        std::cerr << "ERROR:"
                  << " at " << __FILE__ << ":" << __LINE__
                  << std::endl << std::flush;
        exit(1);
    }
    PyObject * b1;
    if (!PyArg_ParseTuple(args, "O", &b1)) {
        std::cerr << "ERROR:"
                  << " at " << __FILE__ << ":" << __LINE__
                  << std::endl << std::flush;
        exit(1);
    }
    DCBuffer * b2 = (DCBuffer*)PyCObject_AsVoidPtr(b1);
    return PyLong_FromLong((long)b2->getUsedSize());
}

static PyObject * DCBuffer_increment_extract_ptr(
    PyObject * self, PyObject * args)
{
    int arglen = PyTuple_Size(args);
    if (arglen != 2) {
        std::cerr << "ERROR:"
                  << " at " << __FILE__ << ":" << __LINE__
                  << std::endl << std::flush;
        exit(1);
    }
    PyObject * b1;
    int len;
    if (!PyArg_ParseTuple(args, "Oi", &b1, &len)) {
        std::cerr << "ERROR:"
                  << " at " << __FILE__ << ":" << __LINE__
                  << std::endl << std::flush;
        exit(1);
    }
    DCBuffer * b2 = (DCBuffer*)PyCObject_AsVoidPtr(b1);
    b2->incrementExtractPointer(len);
    return Py_BuildValue("");
}

// begin DCFilter methods

static PyObject * DCFilter_get_param(PyObject * self, PyObject * args)
{
    int arglen = PyTuple_Size(args);
    if (arglen != 2) {
        std::cerr << "ERROR: calling DCFilter_write"
                  << " at " << __FILE__ << ":" << __LINE__
                  << std::endl << std::flush;
        exit(1);
    }
    PyObject * peer;
    char * key;
    if (!PyArg_ParseTuple(args, "Os", &peer, &key)) {
        std::cerr << "ERROR: parsing tuple"
                  << " at " << __FILE__ << ":" << __LINE__
                  << std::endl << std::flush;
        exit(1);
    }
    DCFilter * filter = (DCFilter*)PyCObject_AsVoidPtr(peer);
    std::string val = filter->get_param(key);
    return PyString_FromString(val.c_str());
}

static PyObject * DCFilter_read(PyObject * self, PyObject * args)
{
    int arglen = PyTuple_Size(args);
    if (arglen != 2) {
        std::cerr << "ERROR: calling DCFilter_write"
                  << " at " << __FILE__ << ":" << __LINE__
                  << std::endl << std::flush;
        exit(1);
    }
    PyObject * peer;
    char * port;
    if (!PyArg_ParseTuple(args, "Os", &peer, &port)) {
        std::cerr << "ERROR: parsing tuple"
                  << " at " << __FILE__ << ":" << __LINE__
                  << std::endl << std::flush;
        exit(1);
    }
    DCFilter * filter = (DCFilter*)PyCObject_AsVoidPtr(peer);
    DCBuffer * out;
    Py_BEGIN_ALLOW_THREADS
    out = filter->read(port);
    Py_END_ALLOW_THREADS
    return PyCObject_FromVoidPtr(out, DCBuffer_destroy);
}

static PyObject * DCFilter_read_nonblocking(PyObject * self, PyObject * args)
{
    int arglen = PyTuple_Size(args);
    if (arglen != 2) {
        std::cerr << "ERROR: calling DCFilter_write"
                  << " at " << __FILE__ << ":" << __LINE__
                  << std::endl << std::flush;
        exit(1);
    }
    PyObject * peer;
    char * port;
    if (!PyArg_ParseTuple(args, "Os", &peer, &port)) {
        std::cerr << "ERROR: parsing tuple"
                  << " at " << __FILE__ << ":" << __LINE__
                  << std::endl << std::flush;
        exit(1);
    }
    DCFilter * filter = (DCFilter*)PyCObject_AsVoidPtr(peer);
    DCBuffer * out;
    Py_BEGIN_ALLOW_THREADS
    out = filter->read_nonblocking(port);
    Py_END_ALLOW_THREADS
    if (!out) {
        return Py_BuildValue("");
    }
    else {
        return PyCObject_FromVoidPtr(out, DCBuffer_destroy);
    }
}

static PyObject * DCFilter_read_until_upstream_exit(
    PyObject * self, PyObject * args)
{
    int arglen = PyTuple_Size(args);
    if (arglen != 2) {
        std::cerr << "ERROR: calling DCFilter_write"
                  << " at " << __FILE__ << ":" << __LINE__
                  << std::endl << std::flush;
        exit(1);
    }
    PyObject * peer;
    char * port;
    if (!PyArg_ParseTuple(args, "Os", &peer, &port)) {
        std::cerr << "ERROR: parsing tuple"
                  << " at " << __FILE__ << ":" << __LINE__
                  << std::endl << std::flush;
        exit(1);
    }
    DCFilter * filter = (DCFilter*)PyCObject_AsVoidPtr(peer);
    DCBuffer * out;
    Py_BEGIN_ALLOW_THREADS
    out = filter->read_until_upstream_exit(port);
    Py_END_ALLOW_THREADS
    if (!out) {
        return Py_BuildValue("");
    }
    else {
        return PyCObject_FromVoidPtr(out, DCBuffer_destroy);
    }
}

static PyObject * DCFilter_write(PyObject * self, PyObject * args)
{
    int arglen = PyTuple_Size(args);
    if (arglen != 4) {
        std::cerr << "ERROR: calling DCFilter_write"
                  << " at " << __FILE__ << ":" << __LINE__
                  << std::endl << std::flush;
        exit(1);
    }
    PyObject * peer;
    PyObject * buffer;
    char * port;
    char * label;
    if (!PyArg_ParseTuple(args, "OOss", &peer, &buffer, &port, &label)) {
        std::cerr << "ERROR: parsing tuple"
                  << " at " << __FILE__ << ":" << __LINE__
                  << std::endl << std::flush;
        exit(1);
    }
    DCFilter * filter = (DCFilter*)PyCObject_AsVoidPtr(peer);
    DCBuffer * buffer2 = (DCBuffer*)PyCObject_AsVoidPtr(buffer);
    Py_BEGIN_ALLOW_THREADS
    filter->write(buffer2, port, label);
    Py_END_ALLOW_THREADS
    return Py_BuildValue("");
}

static PyMethodDef method_table[] =
{
    {"DCBuffer_new", DCBuffer_new, METH_VARARGS, ""},
    {"DCBuffer_appendint", DCBuffer_appendint, METH_VARARGS, ""},
    {"DCBuffer_extractint", DCBuffer_extractint, METH_VARARGS, ""},
    {"DCBuffer_appendstr", DCBuffer_appendstr, METH_VARARGS, ""},
    {"DCBuffer_extractstr", DCBuffer_extractstr, METH_VARARGS, ""},
    {"DCBuffer_appendfloat", DCBuffer_appendfloat, METH_VARARGS, ""},
    {"DCBuffer_extractfloat", DCBuffer_extractfloat, METH_VARARGS, ""},
    {"DCBuffer_appenddouble", DCBuffer_appenddouble, METH_VARARGS, ""},
    {"DCBuffer_extractdouble", DCBuffer_extractdouble, METH_VARARGS, ""},
    {"DCBuffer_appendbytearray", DCBuffer_appendbytearray, METH_VARARGS, ""},
    {"DCBuffer_getraw", DCBuffer_getraw, METH_VARARGS, ""},
    {"DCBuffer_getrawextract", DCBuffer_getrawextract, METH_VARARGS, ""},
    {"DCBuffer_setusedsize", DCBuffer_setusedsize, METH_VARARGS, ""},
    {"DCBuffer_getusedsize", DCBuffer_getusedsize, METH_VARARGS, ""},
    {"DCBuffer_increment_extract_ptr", DCBuffer_increment_extract_ptr, METH_VARARGS, ""},
    {"DCFilter_get_param", DCFilter_get_param, METH_VARARGS, ""},
    {"DCFilter_read", DCFilter_read, METH_VARARGS, ""},
    {"DCFilter_read_nonblocking", DCFilter_read_nonblocking, METH_VARARGS, ""},
    {"DCFilter_read_until_upstream_exit", DCFilter_read_until_upstream_exit, METH_VARARGS, ""},
    {"DCFilter_write", DCFilter_write, METH_VARARGS, ""},
    {NULL,NULL,0,NULL}
};

extern "C" void initdcmpi_pynativedefs()
{
    Py_InitModule("dcmpi_pynativedefs", method_table);
}
