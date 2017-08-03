package myFlowAnalysis;

import java.util.Iterator;
import java.util.Map;

import soot.Body;
import soot.BodyTransformer;
import soot.Local;
import soot.PackManager;
import soot.PatchingChain;
import soot.RefType;
import soot.Scene;
import soot.SootClass;
import soot.SootMethod;
import soot.Transform;
import soot.Unit;
import soot.jimple.AbstractStmtSwitch;
import soot.jimple.InvokeExpr;
import soot.jimple.InvokeStmt;
import soot.jimple.Jimple;
import soot.jimple.StringConstant;
import soot.options.Options;

import soot.toolkits.graph.*;
import soot.toolkits.scalar.GuaranteedDefs;

import org.jf.dexlib2.DexFileFactory;
import org.jf.dexlib2.iface.ClassDef;
import org.jf.dexlib2.iface.DexFile;
import org.jf.dexlib2.Opcodes;
import org.jf.dexlib2.iface.Method;


import soot.dexpler.DexClassLoader;

import java.io.IOException;
import com.google.common.collect.Lists;
import java.util.List;

import myFlowAnalysis.Flow;

public class DexAnalyzer {
	public static void main(String[] args) throws IOException {
    System.out.println("dexAnalyzer");

    Scene myScene = Scene.v();
    // not sure why do I have to do this
    // If I omit this line, SootClassPath becomes null
    myScene.getSootClassPath();
    myScene.extendSootClassPath("classes");
    System.out.println(myScene.getSootClassPath());
    myScene.loadNecessaryClasses();


    DexClassLoader dxLoader = new DexClassLoader();
    DexFile dexFile = DexFileFactory.loadDexFile(args[0], Opcodes.forApi(26));
    //DexFile dexFile = DexFileFactory.loadDexFile("output.dex", 15);

    for (ClassDef classDef: dexFile.getClasses()) {

      List<Method> methods = Lists.newArrayList();
      System.out.println(classDef);

      //Soot representation of a Java class.
			//They are usually created by a Scene,
			//but can also be constructed manually through the given constructors.

      SootClass c = new SootClass(" ");
      dxLoader.makeSootClass(c, classDef, dexFile);
      System.out.println(c);
      c.setApplicationClass();
       
      SootMethod m = c.getMethodByName("main");
      Body b = m.retrieveActiveBody();
      UnitGraph g = new ExceptionalUnitGraph(b);

      //System.out.println(g);

      Flow myFlow = new Flow(g);
    }

    //soot.Main.main(args);
  }
}


