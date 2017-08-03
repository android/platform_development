package myFlowAnalysis;
// A simple example that uses a native method. 
public class Test { 
	int i; 
	public static void main(String args[]) { 
    String LIB = "haha";
    if ("haha".equals(LIB))
      System.loadLibrary(LIB);
    else
      LIB = "whatever";
  }
} 
