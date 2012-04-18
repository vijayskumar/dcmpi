package dcmpi;

import java.util.Vector;

public class DCMPIJarClassLoader extends JarClassLoader
{
    private String myJarName;
    public DCMPIJarClassLoader(String jarName) throws Exception
	{
        super(jarName);
        myJarName = jarName;
	}
    public Class loadup(String name)
    {
        int i;
        Object[] bytes = jarResources.getClassBytes();
        for (i = 0; i < bytes.length; i++) {
            byte[] b = (byte[])bytes[i];
            Class c = this.defineClass(null, b, 0, b.length);
            //             System.out.println(c.getName());
            System.out.println("saw name " + c.getName());
            if (c.getName().equals(name)) {
                return c;
            }
        }
        System.err.println("ERROR: couldn't load " + name  + " in loadClass()");
        System.exit(1);
        return null;
    }
    public String[] getDCMPIFiltersAvailable()
    {
        Vector v = new Vector(10);
        Object[] bytes = jarResources.getClassBytes();
        int i;
        String[] names = jarResources.getClassNames();
        for (i = 0; i < bytes.length; i++) {
            byte[] b = (byte[])bytes[i];
            Class c;
            try {
                c = this.defineClass(null, b, 0, b.length);
            }
            catch (java.lang.LinkageError e) {
//                 System.out.println("linkage error, size "+b.length);
//                 e.printStackTrace();
                continue;
            }
        }
        for (i = 0; i < names.length; i++) {
            String name = names[i].replace("/",".");
            if (name.endsWith(".class")) {
                name = name.substring(0, name.length()-6);
            }
//             System.out.println(name);
            Class c=null;
            try {
                c = this.findLoadedClass(name);
            }
            catch (Exception e) {
                e.printStackTrace();
            }

            Class uptree = c;
            boolean member = false;
            while (true) {
                if (uptree == null || uptree.getName() == null) {
                    break;
                }
                else if (uptree.getName().equals("java.lang.Object")) {
                    break;
                }
                else if (uptree.getName().equals("dcmpi.DCFilter")) {
                    member = true;
                    break;
                }
                uptree = uptree.getSuperclass();
            }
            if (member) {
                v.add(c.getName());
            }
        }
        String[] avail = new String[v.size()];
        for (i = 0; i < v.size(); i++) {
            avail[i] = (String)v.elementAt(i);
        }
        return avail;
    }

    public static void main(String[] args) throws Exception
    {
        if (args.length != 1)
            {
                System.err.println
                    ("Usage: java JarClassLoader " +
                     "<jar file name>");
                System.exit (1);
            }

        /*
         * Create the .jar class loader and use the first argument
         * passed in from the command line as the .jar file to use.
         */
        DCMPIJarClassLoader jarLoader = new DCMPIJarClassLoader (args [0]);
        String[] filters = jarLoader.getDCMPIFiltersAvailable();
        int i;
        for (i = 0; i < filters.length; i++) {
            System.out.println("next DCFilter: " + filters[i]);
        }
    }
}
