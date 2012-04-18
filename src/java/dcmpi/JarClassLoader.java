package dcmpi;

public class JarClassLoader extends MultiClassLoader
{
    protected JarResources jarResources;

    public JarClassLoader (String jarName) throws Exception
	{
        // Create the JarResource and suck in the .jar file.
        jarResources = new JarResources (jarName);
	}

    public void loadClasses()
    {
        Object[] bytes = jarResources.getClassBytes();
        int i;
        for (i = 0; i < bytes.length; i++) {
            byte[] b = (byte[])bytes[i];
            Class c = this.defineClass(null, b, 0, b.length);
            System.out.println("class name: " + c.getName());
        }
    }

        protected byte[] loadClassBytes (String className)
	{
        // Support the MultiClassLoader's class name munging facility.
        className = formatClassName (className);

        // Attempt to get the class data from the JarResource.
        return (jarResources.getResource (className));
	}


    /*
     * Internal Testing application.
     */
    public static void main(String[] args) throws Exception
    {
        if (args.length != 1)
            {
                System.err.println
                    ("Usage: java JarClassLoader " +
                     "<jar file name> <class name>");
                System.exit (1);
            }

        /*
         * Create the .jar class loader and use the first argument
         * passed in from the command line as the .jar file to use.
         */
        JarClassLoader jarLoader = new JarClassLoader (args [0]);

        jarLoader.loadClasses();
    }
}
