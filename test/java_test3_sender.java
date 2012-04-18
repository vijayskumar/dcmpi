package test;

import dcmpi.*;

public class java_test3_sender extends DCFilter {
    class java_test3_sender_inner  {
        public int i;
    }

    public int process() {
        System.out.println("sender starting in java");
        DCBuffer b = new DCBuffer();
        b.AppendInt(1);
        b.AppendInt(2);
        b.AppendInt(3);
        write(b, "0");
        System.out.println("sender wrote, now exiting");
        return 0;
    }
}
