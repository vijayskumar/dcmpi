#include "dcmpi_util.h"

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <list>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <queue>
#include <vector>

#include <dcmpi.h>

#include "dcmpi_backtrace.h"

#include "native.h"

using namespace std;

/* --- DCBuffer defs --- */
jfieldID cached_FID_DCBuffer_peer_ptr;
JNIEXPORT void JNICALL Java_dcmpi_DCBuffer_initIDs
  (JNIEnv * env, jclass cls)
{
    cached_FID_DCBuffer_peer_ptr = env->GetFieldID(cls, "peer_ptr", "J");
}

#define DCBUFFER_FUNC_INIT()                                            \
    jlong peer_ptr = env->GetLongField(self, cached_FID_DCBuffer_peer_ptr); \
    DCMPI_JNI_EXCEPTION_CHECK(env);                                     \
    DCBuffer * b = DCMPI_JNI_CAST_FROM_JLONG_TO_POINTER(peer_ptr, DCBuffer*);


JNIEXPORT void JNICALL Java_dcmpi_DCBuffer_AppendByte
  (JNIEnv * env, jobject self, jbyte arg)
{
    DCBUFFER_FUNC_INIT();
    b->Append(arg);
}

JNIEXPORT void JNICALL Java_dcmpi_DCBuffer_AppendByteArray
  (JNIEnv * env, jobject self, jbyteArray arg)
{
    DCBUFFER_FUNC_INIT();
    jbyte * carr = env->GetByteArrayElements(arg, NULL);
    DCMPI_JNI_EXCEPTION_CHECK(env);
    b->Append(carr, env->GetArrayLength(arg));
    env->ReleaseByteArrayElements(arg, carr, 0);
    DCMPI_JNI_EXCEPTION_CHECK(env);
}

JNIEXPORT void JNICALL Java_dcmpi_DCBuffer_AppendInt
    (JNIEnv * env, jobject self, jint i)
{
    DCBUFFER_FUNC_INIT();
    b->Append(i);
}

JNIEXPORT void JNICALL Java_dcmpi_DCBuffer_OverwriteInt
  (JNIEnv * env, jobject self, jint byteOffset, jint i)
{
    DCBUFFER_FUNC_INIT();
    if (byteOffset+4 > b->getUsedSize()) {
        fprintf(stderr, "ERROR: overwrote buffer at %s:%d\n", __FILE__, __LINE__);
        exit(1);
    }
    int4 i4 = (int4)i;
    memcpy(b->getPtr() + byteOffset, &i4, sizeof(i4));
}

JNIEXPORT void JNICALL Java_dcmpi_DCBuffer_AppendLong
    (JNIEnv * env, jobject self, jlong l)
{
    DCBUFFER_FUNC_INIT();
    b->Append(l);
}

JNIEXPORT void JNICALL Java_dcmpi_DCBuffer_AppendFloat
    (JNIEnv * env, jobject self, jfloat f)
{
    DCBUFFER_FUNC_INIT();
    b->Append(f);
}

JNIEXPORT void JNICALL Java_dcmpi_DCBuffer_AppendDouble
  (JNIEnv * env, jobject self, jdouble d)
{
    DCBUFFER_FUNC_INIT();
    b->Append(d);
}

JNIEXPORT void JNICALL Java_dcmpi_DCBuffer_AppendString
  (JNIEnv * env, jobject self, jstring s)
{
    DCBUFFER_FUNC_INIT();
    const char * s2 = (const char*)env->GetStringUTFChars(s, NULL);
    b->Append(s2);
    env->ReleaseStringUTFChars(s, s2);
}
    
JNIEXPORT jbyte JNICALL Java_dcmpi_DCBuffer_ExtractByte
  (JNIEnv * env, jobject self)
{
    DCBUFFER_FUNC_INIT();
    uint1 u;
    b->Extract(&u);
    return (jbyte)u;
}

JNIEXPORT jbyteArray JNICALL Java_dcmpi_DCBuffer_ExtractByteArray
  (JNIEnv * env, jobject self, jint num_elements)
{
    DCBUFFER_FUNC_INIT();
    jbyteArray array = env->NewByteArray(num_elements);
    DCMPI_JNI_EXCEPTION_CHECK(env);
    jbyte * a = env->GetByteArrayElements(array, NULL);
    DCMPI_JNI_EXCEPTION_CHECK(env);
    b->Extract((uint1*)a, (int)num_elements);
    env->ReleaseByteArrayElements(array, a, 0);
    return array;
}

JNIEXPORT jint JNICALL Java_dcmpi_DCBuffer_ExtractInt
  (JNIEnv * env, jobject self)
{
    DCBUFFER_FUNC_INIT();
    int4 i;
    b->Extract(&i);
    return (jint)i;
}

JNIEXPORT jlong JNICALL Java_dcmpi_DCBuffer_ExtractLong
  (JNIEnv * env, jobject self)
{
        DCBUFFER_FUNC_INIT();
    int8 l;
    b->Extract(&l);
    return (jlong)l;
}

JNIEXPORT jfloat JNICALL Java_dcmpi_DCBuffer_ExtractFloat
    (JNIEnv * env, jobject self)
{
    DCBUFFER_FUNC_INIT();
    float f;
    b->Extract(&f);
    return (jfloat)f;
}

JNIEXPORT jdouble JNICALL Java_dcmpi_DCBuffer_ExtractDouble
  (JNIEnv * env, jobject self)
{
    DCBUFFER_FUNC_INIT();
    double d;
    b->Extract(&d);
    return (jdouble)d;
}

JNIEXPORT jstring JNICALL Java_dcmpi_DCBuffer_ExtractString
  (JNIEnv *env, jobject self)
{
    DCBUFFER_FUNC_INIT();
    std::string s;
    b->Extract(&s);
    jstring s2 = env->NewStringUTF(s.c_str());
    return s2;
}

JNIEXPORT void JNICALL Java_dcmpi_DCBuffer_compress
  (JNIEnv * env, jobject self)
{
    DCBUFFER_FUNC_INIT();
    b->compress();
}

JNIEXPORT void JNICALL Java_dcmpi_DCBuffer_decompress
  (JNIEnv * env, jobject self)
{
    DCBUFFER_FUNC_INIT();
    b->decompress();
}

JNIEXPORT void JNICALL Java_dcmpi_DCBuffer_construct
    (JNIEnv * env, jobject self, jint size)
{
    DCBuffer * b = new DCBuffer(size);
//     printf("allocated DCBuffer %p\n", b);
//     dcmpi_print_backtrace();
    env->SetLongField(self, cached_FID_DCBuffer_peer_ptr, DCMPI_JNI_CAST_FROM_POINTER_TO_JLONG(b));
    DCMPI_JNI_EXCEPTION_CHECK(env);
}

JNIEXPORT void JNICALL Java_dcmpi_DCBuffer_handoff
  (JNIEnv * env, jobject self, jlong peer_handoff)
{
    DCBUFFER_FUNC_INIT(); // eliminate previously held DCBuffer
    b->consume();
//     printf("handoff consumed DCBuffer %p\n", b);
    env->SetLongField(self, cached_FID_DCBuffer_peer_ptr, peer_handoff);
    DCMPI_JNI_EXCEPTION_CHECK(env);
}

JNIEXPORT void JNICALL Java_dcmpi_DCBuffer_consume
  (JNIEnv * env, jobject self)
{
    DCBUFFER_FUNC_INIT();
    if (b) {
        b->consume();
        env->SetLongField(self, cached_FID_DCBuffer_peer_ptr,
                          // 0 in lieu of NULL
                          DCMPI_JNI_CAST_FROM_POINTER_TO_JLONG(0));
        // dcmpi_print_backtrace();
        // printf("consumed DCBuffer %p from peer %p\n", b, self);
    }
    else {
        // printf("not re-consuming DCBuffer from peer %p\n", self);
    }
}

/*
 * Class:     dcmpi_DCBuffer
 * Method:    writeToDisk
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_dcmpi_DCBuffer_writeToDisk
(JNIEnv *env, jobject self, jstring file) {
    DCBUFFER_FUNC_INIT();
    const char * fileptr = (const char*)env->GetStringUTFChars(file, NULL);
    DCMPI_JNI_EXCEPTION_CHECK(env);
    b->saveToDisk(fileptr);
    DCMPI_JNI_EXCEPTION_CHECK(env);
    env->ReleaseStringUTFChars(file, fileptr);
}

/*
 * Class:     dcmpi_DCBuffer
 * Method:    readFromDisk
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_dcmpi_DCBuffer_readFromDisk
(JNIEnv * env, jobject self, jstring file) {
    DCBUFFER_FUNC_INIT();
    const char * fileptr = (const char*)env->GetStringUTFChars(file, NULL);
    DCMPI_JNI_EXCEPTION_CHECK(env);
    b->restoreFromDisk(fileptr);
    DCMPI_JNI_EXCEPTION_CHECK(env);
    env->ReleaseStringUTFChars(file, fileptr);

}



/* --- DCFilter defs --- */
jfieldID cached_FID_DCFilter_peer_ptr;
JNIEXPORT void JNICALL Java_dcmpi_DCFilter_initIDs
  (JNIEnv * env, jclass cls)
{
    cached_FID_DCFilter_peer_ptr = env->GetFieldID(cls, "peer_ptr", "J");
}

#define DCFILTER_FUNC_INIT()                                            \
    jlong peer_ptr = env->GetLongField(self, cached_FID_DCFilter_peer_ptr); \
    DCMPI_JNI_EXCEPTION_CHECK(env);                                     \
    DCFilter * f = DCMPI_JNI_CAST_FROM_JLONG_TO_POINTER(peer_ptr, DCFilter*);

JNIEXPORT jobject JNICALL Java_dcmpi_DCFilter_read
  (JNIEnv * env, jobject self, jstring port)
{
    DCFILTER_FUNC_INIT();
    env->PushLocalFrame(10);
    DCMPI_JNI_EXCEPTION_CHECK(env);
    const char * port_cstr = env->GetStringUTFChars(port, NULL);
    DCMPI_JNI_EXCEPTION_CHECK(env);
    DCBuffer * b = f->read(port_cstr);
    env->ReleaseStringUTFChars(port, port_cstr);
    DCMPI_JNI_EXCEPTION_CHECK(env);
    jclass cls_DCBuffer = env->FindClass("dcmpi/DCBuffer");
    DCMPI_JNI_EXCEPTION_CHECK(env);
    jmethodID cid_constructor = env->GetMethodID(cls_DCBuffer, "<init>", "()V");
    DCMPI_JNI_EXCEPTION_CHECK(env);
    jobject newbuffer = env->NewObject(cls_DCBuffer, cid_constructor);
    DCMPI_JNI_EXCEPTION_CHECK(env);
    jclass newcls = env->GetObjectClass(newbuffer);
    DCMPI_JNI_EXCEPTION_CHECK(env);
    jmethodID cid_handoff = env->GetMethodID(newcls, "handoff", "(J)V");
    DCMPI_JNI_EXCEPTION_CHECK(env);
    env->CallVoidMethod(newbuffer, cid_handoff, DCMPI_JNI_CAST_FROM_POINTER_TO_JLONG(b));
    DCMPI_JNI_EXCEPTION_CHECK(env);
//     printf("read() handed off elsewhere-allocated buffer %p\n", b);
    newbuffer = env->PopLocalFrame(newbuffer);
    DCMPI_JNI_EXCEPTION_CHECK(env);
    return newbuffer;
}

JNIEXPORT jobject JNICALL Java_dcmpi_DCFilter_read_1until_1upstream_1exit
    (JNIEnv * env, jobject self, jstring port)
{
    DCFILTER_FUNC_INIT();
    DCMPI_JNI_EXCEPTION_CHECK(env);
    const char * port_cstr = env->GetStringUTFChars(port, NULL);
    DCMPI_JNI_EXCEPTION_CHECK(env);
    DCBuffer * b = f->read_until_upstream_exit(port_cstr);
    env->ReleaseStringUTFChars(port, port_cstr);
    DCMPI_JNI_EXCEPTION_CHECK(env);
    if (b == NULL) {
        return NULL;
    }
    env->PushLocalFrame(10);
    jclass cls_DCBuffer = env->FindClass("dcmpi/DCBuffer");
    DCMPI_JNI_EXCEPTION_CHECK(env);
    jmethodID cid_constructor = env->GetMethodID(cls_DCBuffer, "<init>", "()V");
    DCMPI_JNI_EXCEPTION_CHECK(env);
    jobject newbuffer = env->NewObject(cls_DCBuffer, cid_constructor);
    DCMPI_JNI_EXCEPTION_CHECK(env);
    jclass newcls = env->GetObjectClass(newbuffer);
    DCMPI_JNI_EXCEPTION_CHECK(env);
    jmethodID cid_handoff = env->GetMethodID(newcls, "handoff", "(J)V");
    DCMPI_JNI_EXCEPTION_CHECK(env);
    env->CallVoidMethod(newbuffer, cid_handoff, DCMPI_JNI_CAST_FROM_POINTER_TO_JLONG(b));
    DCMPI_JNI_EXCEPTION_CHECK(env);
//     printf("read_until_upstream_exit() handed off elsewhere-allocated buffer %p\n", b);
    newbuffer = env->PopLocalFrame(newbuffer);
    DCMPI_JNI_EXCEPTION_CHECK(env);
    return newbuffer;
}

/* by k2
 */
JNIEXPORT jobject JNICALL Java_dcmpi_DCFilter_get_1init_1filter_1broadcast
(JNIEnv *env, jobject self) {
    DCFILTER_FUNC_INIT();
    DCMPI_JNI_EXCEPTION_CHECK(env);
    DCBuffer * b = f->get_init_filter_broadcast();
    DCMPI_JNI_EXCEPTION_CHECK(env);
    if (b == NULL) {
      return NULL;
    }
    env->PushLocalFrame(10);
    jclass cls_DCBuffer = env->FindClass("dcmpi/DCBuffer");
    DCMPI_JNI_EXCEPTION_CHECK(env);
    jmethodID cid_constructor = env->GetMethodID(cls_DCBuffer, "<init>", "()V");
    DCMPI_JNI_EXCEPTION_CHECK(env);
    jobject newbuffer = env->NewObject(cls_DCBuffer, cid_constructor);
    DCMPI_JNI_EXCEPTION_CHECK(env);
    jclass newcls = env->GetObjectClass(newbuffer);
    DCMPI_JNI_EXCEPTION_CHECK(env);
    jmethodID cid_handoff = env->GetMethodID(newcls, "handoff", "(J)V");
    DCMPI_JNI_EXCEPTION_CHECK(env);
    env->CallVoidMethod(newbuffer, cid_handoff, DCMPI_JNI_CAST_FROM_POINTER_TO_JLONG(b));
    DCMPI_JNI_EXCEPTION_CHECK(env);
    newbuffer = env->PopLocalFrame(newbuffer);
    DCMPI_JNI_EXCEPTION_CHECK(env);
    return newbuffer;

}



JNIEXPORT jobject JNICALL Java_dcmpi_DCFilter_readany
  (JNIEnv * env, jobject self)
{
    DCFILTER_FUNC_INIT();
    DCMPI_JNI_EXCEPTION_CHECK(env);
    DCBuffer * b = f->readany();
    DCMPI_JNI_EXCEPTION_CHECK(env);
    if (b == NULL) {
      return NULL;
    }
    env->PushLocalFrame(10);
    jclass cls_DCBuffer = env->FindClass("dcmpi/DCBuffer");
    DCMPI_JNI_EXCEPTION_CHECK(env);
    jmethodID cid_constructor = env->GetMethodID(cls_DCBuffer, "<init>", "()V");
    DCMPI_JNI_EXCEPTION_CHECK(env);
    jobject newbuffer = env->NewObject(cls_DCBuffer, cid_constructor);
    DCMPI_JNI_EXCEPTION_CHECK(env);
    jclass newcls = env->GetObjectClass(newbuffer);
    DCMPI_JNI_EXCEPTION_CHECK(env);
    jmethodID cid_handoff = env->GetMethodID(newcls, "handoff", "(J)V");
    DCMPI_JNI_EXCEPTION_CHECK(env);
    env->CallVoidMethod(newbuffer, cid_handoff, DCMPI_JNI_CAST_FROM_POINTER_TO_JLONG(b));
    DCMPI_JNI_EXCEPTION_CHECK(env);
    newbuffer = env->PopLocalFrame(newbuffer);
    DCMPI_JNI_EXCEPTION_CHECK(env);
    return newbuffer;
}

/** author: k2 */
JNIEXPORT void JNICALL Java_dcmpi_DCFilter_write_1broadcast
(JNIEnv *env, jobject self, jobject buffer, jstring port) {
    DCFILTER_FUNC_INIT();
    jlong peer_ptr_b = env->GetLongField(buffer, cached_FID_DCBuffer_peer_ptr);
    DCBuffer * b = DCMPI_JNI_CAST_FROM_JLONG_TO_POINTER(peer_ptr_b, DCBuffer*);
    const char * port_cstr = env->GetStringUTFChars(port, NULL);
    DCMPI_JNI_EXCEPTION_CHECK(env);
    f->write_broadcast(b, port_cstr);
    env->ReleaseStringUTFChars(port, port_cstr);
    DCMPI_JNI_EXCEPTION_CHECK(env);
}


JNIEXPORT jobject JNICALL Java_dcmpi_DCFilter_read_1nonblocking
  (JNIEnv * env, jobject self, jstring port)
{
    DCFILTER_FUNC_INIT();
    DCMPI_JNI_EXCEPTION_CHECK(env);
    const char * port_cstr = env->GetStringUTFChars(port, NULL);
    DCMPI_JNI_EXCEPTION_CHECK(env);
    DCBuffer * b = f->read_nonblocking(port_cstr);
    env->ReleaseStringUTFChars(port, port_cstr);
    DCMPI_JNI_EXCEPTION_CHECK(env);
    if (b == NULL) {
        return NULL;
    }
    env->PushLocalFrame(10);
    jclass cls_DCBuffer = env->FindClass("dcmpi/DCBuffer");
    DCMPI_JNI_EXCEPTION_CHECK(env);
    jmethodID cid_constructor = env->GetMethodID(cls_DCBuffer, "<init>", "()V");
    DCMPI_JNI_EXCEPTION_CHECK(env);
    jobject newbuffer = env->NewObject(cls_DCBuffer, cid_constructor);
    DCMPI_JNI_EXCEPTION_CHECK(env);
    jclass newcls = env->GetObjectClass(newbuffer);
    DCMPI_JNI_EXCEPTION_CHECK(env);
    jmethodID cid_handoff = env->GetMethodID(newcls, "handoff", "(J)V");
    DCMPI_JNI_EXCEPTION_CHECK(env);
    env->CallVoidMethod(newbuffer, cid_handoff, DCMPI_JNI_CAST_FROM_POINTER_TO_JLONG(b));
    DCMPI_JNI_EXCEPTION_CHECK(env);
    newbuffer = env->PopLocalFrame(newbuffer);
    DCMPI_JNI_EXCEPTION_CHECK(env);
    return newbuffer;
}

JNIEXPORT void JNICALL Java_dcmpi_DCFilter_write
    (JNIEnv * env, jobject self, jobject buffer, jstring port)
{
    DCFILTER_FUNC_INIT();
    jlong peer_ptr_b = env->GetLongField(buffer, cached_FID_DCBuffer_peer_ptr);
    DCBuffer * b = DCMPI_JNI_CAST_FROM_JLONG_TO_POINTER(peer_ptr_b, DCBuffer*);
    const char * port_cstr = env->GetStringUTFChars(port, NULL);
    DCMPI_JNI_EXCEPTION_CHECK(env);
    f->write(b, port_cstr);
    env->ReleaseStringUTFChars(port, port_cstr);
    DCMPI_JNI_EXCEPTION_CHECK(env);
}

JNIEXPORT jboolean JNICALL Java_dcmpi_DCFilter_has_1param
  (JNIEnv * env, jobject self, jstring param)
{
    DCFILTER_FUNC_INIT();
    const char * k = env->GetStringUTFChars(param, NULL);
    bool b = f->has_param(k);
    env->ReleaseStringUTFChars(param, k);
    jboolean out = false;
    if (b) {
        out = true;
    }
    return out;
}

JNIEXPORT jstring JNICALL Java_dcmpi_DCFilter_get_1param
    (JNIEnv * env, jobject self, jstring param)
{
    DCFILTER_FUNC_INIT();
    const char * k = env->GetStringUTFChars(param, NULL);
    std::string s = f->get_param(k);
    env->ReleaseStringUTFChars(param, k);
    return env->NewStringUTF(s.c_str());
}

JNIEXPORT jstring JNICALL Java_dcmpi_DCFilter_get_1bind_1host
  (JNIEnv * env, jobject self)
{
    DCFILTER_FUNC_INIT();
    std::string s = f->get_bind_host();
    return env->NewStringUTF(s.c_str());
}

JNIEXPORT void JNICALL Java_dcmpi_DCFilter_handoff
  (JNIEnv * env, jobject self, jlong peer_ptr)
{
    env->SetLongField(self, cached_FID_DCFilter_peer_ptr, peer_ptr);
    DCMPI_JNI_EXCEPTION_CHECK(env);
}
