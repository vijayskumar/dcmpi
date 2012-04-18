package dcmpi;

public class DCFilter
{
    public int init() { 
	//Always call DCFilter.init() in all your filters!! - k2
	Thread.currentThread().setContextClassLoader(this.getClass().getClassLoader());
	return 0; }
    public int process() { return -1; }
    public int fini() { return 0; }

    // blocks until returns, always returns non-NULL
    public native DCBuffer read(String port);

    // may return non-NULL if all upstream filters (not just on this port)
    // exit
    public native DCBuffer read_until_upstream_exit(String port);

    // read any input, may return NULL if upstream exits
    public native DCBuffer readany();

    // may return NULL if nothing is available at the time it is called
    public native DCBuffer read_nonblocking(String port);

    public native void write(DCBuffer buf, String port);

    public native boolean has_param(String key);
    public native String get_param(String key);

    public native String get_bind_host();

    // by k2: calls similar method in dcfilter
    public native DCBuffer get_init_filter_broadcast();
    public native void write_broadcast(DCBuffer buf, String port);


    // internal details
    private static native void initIDs();
    private long peer_ptr; 
    private native void handoff(long peer_ptr);
    static {
        System.loadLibrary("dcmpijni");
        initIDs();
    }
};
