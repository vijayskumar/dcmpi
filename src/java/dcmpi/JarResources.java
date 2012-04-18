package dcmpi;

import java.io.*;
import java.util.*;
import java.util.zip.*;

/**
 * JarResources: JarResources maps all resources included in a
 * Zip or Jar file. Additionaly, it provides a method to extract one
 * as a blob.
 */
public final class JarResources
{

    // external debug flag
    public boolean debugOn=false;

    // jar resource mapping tables
    private Hashtable htSizes=new Hashtable();  
    private Hashtable htJarContents=new Hashtable();

    // a jar file
    private String jarFileName;

    /**
     * creates a JarResources. It extracts all resources from a Jar
     * into an internal hashtable, keyed by resource names.
     * @param jarFileName a jar or zip file
     */
    public JarResources(String jarFileName) throws Exception
	{
        this.jarFileName=jarFileName;
        init();
	}

    /**
     * Extracts a jar resource as a blob.
     * @param name a resource name.
     */
    public byte[] getResource(String name)
	{
        return (byte[])htJarContents.get(name);
	}

    public Object[] getClassBytes() {
        Object[] outval = new Object[htJarContents.size()];
        int i = 0;
        Enumeration keys = htJarContents.keys();
        java.lang.ClassLoader cl = this.getClass().getClassLoader();
        while (keys.hasMoreElements()) {
            Object k = keys.nextElement();
            String name = (String)k;
            byte[] b = (byte[])htJarContents.get(k);
            String className;
            outval[i++] = (Object)b;
        }
        return outval;
    }

    public String[] getClassNames()
    {
        String [] outval = new String[htJarContents.size()];
        int i = 0;
        Enumeration keys = htJarContents.keys();
        java.lang.ClassLoader cl = this.getClass().getClassLoader();
        while (keys.hasMoreElements()) {
            Object k = keys.nextElement();
            String name = (String)k;
            outval[i++] = name;
        }
        return outval;
    }

    /** initializes internal hash tables with Jar file resources.  */
    private void init() throws Exception
    {
        // extracts just sizes only. 
        ZipFile zf=new ZipFile(jarFileName);
        Enumeration e=zf.entries();
        while (e.hasMoreElements()) {
            ZipEntry ze=(ZipEntry)e.nextElement();
            if (ze.getName().endsWith(".class")) {
                htSizes.put(ze.getName(),
                            new Integer((int)ze.getSize()));
            }
        }
        zf.close();

        // extract resources and put them into the hashtable.
        FileInputStream fis=new FileInputStream(jarFileName);
        BufferedInputStream bis=new BufferedInputStream(fis);
        ZipInputStream zis=new ZipInputStream(bis);
        ZipEntry ze=null;
        while ((ze=zis.getNextEntry())!=null) {
            if (ze.isDirectory()) {
                continue;
            }
            if (!ze.getName().endsWith(".class")) {
                continue;
            }

            int size=(int)ze.getSize();
            // -1 means unknown size.
            if (size==-1)
                {
                    size=((Integer)htSizes.get(ze.getName())).intValue();
                }

            byte[] b=new byte[(int)size];
            int rb=0;
            int chunk=0;
            while (((int)size - rb) > 0)
                {
                    chunk=zis.read(b,rb,(int)size - rb);
                    if (chunk==-1)
                        {
                            break;
                        }
                    rb+=chunk;
                }

            // add to internal resource hashtable
            htJarContents.put(ze.getName(),b);
        }
//         catch (NullPointerException e) {
//             System.out.println("NullPointerException");
//             e.printStackTrace();
//         }
//         catch (FileNotFoundException e) {
//             System.out.println("FileNotFoundException");
//             e.printStackTrace();
//         }
//         catch (IOException e) {
//             System.out.println("IOException");
//             e.printStackTrace();
//         }
    }

    /**
     * Dumps a zip entry into a string.
     * @param ze a ZipEntry
     */
    private String dumpZipEntry(ZipEntry ze)
    {
        StringBuffer sb=new StringBuffer();
        if (ze.isDirectory())
            {
                sb.append("d ");
            }
        else
            {
                sb.append("f ");
            }

        if (ze.getMethod()==ZipEntry.STORED)
            {
                sb.append("stored   ");
            }
        else
            {
                sb.append("defalted ");
            }

        sb.append(ze.getName());
        sb.append("\t");
        sb.append(""+ze.getSize());
        if (ze.getMethod()==ZipEntry.DEFLATED)
            {
                sb.append("/"+ze.getCompressedSize());
            }

        return (sb.toString());
    }

    /**
     * Is a test driver. Given a jar file and a resource name, it trys to
     * extract the resource and then tells us whether it could or not.
     *
     * <strong>Example</strong>
     * Let's say you have a JAR file which jarred up a bunch of gif image
     * files. Now, by using JarResources, you could extract, create, and
     * display those images on-the-fly.
     * <pre>
     *     ...
     *     JarResources JR=new JarResources("GifBundle.jar");
     *     Image image=Toolkit.createImage(JR.getResource("logo.gif");
     *     Image logo=Toolkit.getDefaultToolkit().createImage(
     *                   JR.getResources("logo.gif")
     *                   );
     *     ...
     * </pre>
     */
    public static void main(String[] args) throws Exception
    {
        if (args.length < 1)
            {
                System.err.println(
                    "usage: java JarResources <jar file name>"
                    );
                System.exit(1);
            }

        JarResources jr=new JarResources(args[0]);
        String[] classes = jr.getClassNames();
        System.out.println("classes are:");
        for (int i = 0; i < classes.length; i++) {
            System.out.println(classes[i]);
        }

        if (args.length == 2) {
            if (jr.getResource(args[1]) != null) {
                System.out.println("found class " + args[1]);
            }
            else {
                System.out.println("couldn't find class "+ args[1]);
            }
        }
    }

}	// End of JarResources class.
