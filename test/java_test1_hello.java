package test;

import java.io.*;
import java.net.*;
import dcmpi.*;

public class java_test1_hello extends DCFilter {
    String hostname;
    public java_test1_hello()
    {
        try {
            InetAddress addr = InetAddress.getLocalHost();
            hostname = addr.getHostName();
        }
        catch (UnknownHostException e) {
            ;
        }
    }

    public int init() {
        System.out.println("java_test1_hello: inited in java on " + hostname);
        return 0;
    }

    public int process() {
        System.out.println("java_test1_hello: processed in java on " + hostname);
        DCBuffer b = new DCBuffer(1024);
        b = new DCBuffer(1024);
        b = new DCBuffer(1024);
        b = null;
        System.gc();
        return 0;
    }

    public int fini() {
        System.out.println("java_test1_hello: finied in java on " + hostname);
        return 0;
    }
}
