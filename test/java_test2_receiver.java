package test;

import dcmpi.*;

public class java_test2_receiver extends DCFilter {
    public int process() {
        System.out.println("receiver starting");
        try {
            Thread.sleep(3000);
        } catch(InterruptedException ignored) {}
        DCBuffer b;
        int reps;
        for (reps = 0; reps < 3; reps++) {
            try {
                Thread.sleep(3000);
            } catch(InterruptedException ignored) {}
            System.out.println("calling read");
            b = read("0");
            System.out.println("done calling read");
            System.out.println("the numbers I got are: ");
            int i;
            for (i = 0; i < 3; i++) {
                System.out.println(b.ExtractInt());
            }
            for (i = 0; i < 3; i++) {
                System.out.println((int)b.ExtractByte());
            }
            for (i = 0; i < 3; i++) {
                System.out.println(b.ExtractFloat());
            }
            byte[] ar = b.ExtractByteArray(3);
            for (i = 0; i < 3; i++) {
                System.out.println(ar[i]);
            }
            for (i = 0; i < 3; i++) {
                System.out.println(b.ExtractDouble());
            }
            b.consume();
            // b.AppendInt(4); // evil to do this after a consume
        }
        System.gc();
        return 0;
    }
}
