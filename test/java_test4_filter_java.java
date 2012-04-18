package test;

import java.io.*;
import java.net.*;

import dcmpi.*;

public class java_test4_filter_java extends DCFilter {
    public int init() {
        return 0;
    }

    public int process() {
        String hostname = "<foo>";
        try {
            InetAddress addr = InetAddress.getLocalHost();
            hostname = addr.getHostName();
        }
        catch (UnknownHostException e) {
            ;
        }
        String mode = get_param("mode");
        String myadd = "hello (Java) from " + hostname + "\\";
        int numfilters = Integer.parseInt(get_param("numfilters"));
        if (numfilters == 1) {
            System.out.println("NOPping (Java) for 1 filter");
            return 0;
        }
        String s;
        if (mode.equals("inonly") || mode.equals("inout")) {
            DCBuffer in = read("0");
            s = in.ExtractString();
            System.out.println(hostname + " (Java): got " + s);
            s += myadd;
            in.consume();
        }
        else {
            s = myadd;
        }
        System.out.flush();
        try {
            Thread.sleep(2000);
        } catch(InterruptedException ignored) {}
        if (mode.equals("outonly") || mode.equals("inout")) {
            DCBuffer out;
            out = new DCBuffer();
            out.AppendString(s);
            write(out, "0");
        }
        return 0;
    }

    public int fini() {
        return 0;
    }
}

