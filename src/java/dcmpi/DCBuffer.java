package dcmpi;

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.ObjectInputStream;
import java.io.ObjectOutputStream;
import java.io.Serializable;

public class DCBuffer {

    public DCBuffer() {
        construct(0);
    }
    public DCBuffer(int capacity) {
        construct(capacity);
    }

    public native void AppendByte(byte b);
    public native void AppendByteArray(byte[] b);
    public native void AppendInt(int i);
    public native void AppendLong(long l);
    public native void AppendFloat(float f);
    public native void AppendDouble(double f);
    public native void AppendString(String s);


    public native byte   ExtractByte();
    public native byte[] ExtractByteArray(int num_elements);
    public native int    ExtractInt();
    public native long   ExtractLong();
    public native float  ExtractFloat();
    public native double ExtractDouble();
    public native String ExtractString();

    public void AppendObject(Serializable s) throws IOException {
	ByteArrayOutputStream baos = new ByteArrayOutputStream();
	ObjectOutputStream oos = new ObjectOutputStream(baos);
	oos.writeObject(s);
	byte [] byteArray = baos.toByteArray();
	this.AppendInt(byteArray.length);
	this.AppendByteArray(byteArray);
    }

    public Object ExtractObject() throws IOException, ClassNotFoundException {
	int nbytes = this.ExtractInt();
	ByteArrayInputStream bis = new ByteArrayInputStream(this.ExtractByteArray(nbytes));
	ObjectInputStream ois = new ObjectInputStream(bis);
	return ois.readObject();
    }

    public native void OverwriteInt(int byteOffset, int i);
    
    // call this to release C-memory after you've read a buffer and you're
    // done with it (GC would call this too, but perhaps not right away)
    public native void   consume();
    
    protected void finalize() {
        consume();
    }
    private static native void initIDs();
    private long peer_ptr; 
    private native void construct(int wMax_in);
    private native void handoff(long peer_ptr);
    static {
        System.loadLibrary("dcmpijni");
        initIDs();
    }


    /** added by k2 */
    public native void writeToDisk(String filename);
    public native void readFromDisk(String filename);


    public static void main(String[] args) {
        DCBuffer b = new DCBuffer(1000);
        System.out.println("hello world");
        b.AppendInt(314);
        System.out.println(b.ExtractInt());
        b.AppendInt(314);
        System.out.println(b.ExtractInt());
        b = null;
        System.gc();
    }
}
