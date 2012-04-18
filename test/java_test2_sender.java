package test;

import dcmpi.*;

public class java_test2_sender extends DCFilter {
    public int process() {
        System.out.println("sender starting");
        for (int i = 0; i < 3; i++) {
            DCBuffer b = new DCBuffer();
            b.AppendInt(1);
            b.AppendInt(2);
            b.AppendInt(3);
            b.AppendByte((byte)4);
            b.AppendByte((byte)5);
            b.AppendByte((byte)6);
            b.AppendFloat(7.0f);
            b.AppendFloat(8.0f);
            b.AppendFloat(9.0f);
            byte[] ar = {10, 11, 12};
            b.AppendByteArray(ar);
            b.AppendDouble(13.0);
            b.AppendDouble(14.0);
            b.AppendDouble(15.0);
            write(b, "0");
        }
        System.gc();
        System.out.println("sender wrote 3, now exiting");
        return 0;
    }
}
