package dcmpi;

import java.net.*;
import java.io.*;

public class Loader {
    ClassLoader cl;
    String classname;
    public Loader(String jarname, String _classname)
    {
        try {
            classname = _classname;
            URL[] files = new URL[] { new File(jarname).toURL() };
            cl = new URLClassLoader(files);            
        }
        catch (Exception e) {
            e.printStackTrace();
        }
    }
    public Object load()
    {
        Object o = null;
        try {
            Class c = cl.loadClass(classname);
//             System.out.println("load: loaded " + c.getName());
            o = c.newInstance();
//             System.out.println("load: newInstance returned " + o);
            return o;
        }
        catch (Exception e) {
            System.out.println("load: exception!\n");
            e.printStackTrace();
            System.out.println("load: exception!\n");
            System.exit(1);
            return null;
        }
    }

    public static void main(String[] args) {
        if (args.length != 2) {
            System.out.println("usage: <jar> <class>");
            System.exit(1);
        }
        Loader l = new Loader(args[0], args[1]);
        l.load();
    }
}
