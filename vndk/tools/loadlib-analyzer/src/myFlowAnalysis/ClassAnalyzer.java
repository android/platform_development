package myFlowAnalysis;

import java.util.Iterator;
import java.util.Map;

import soot.Body;
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

import org.jf.dexlib2.iface.ClassDef;
import soot.dexpler.DexClassLoader;

import myFlowAnalysis.Flow;

public class ClassAnalyzer {
	
	public static void main(String[] args) {
    System.out.println("classAnalyzer");
    Scene myScene = Scene.v();
    myScene.getSootClassPath();
    myScene.extendSootClassPath("classes");
    System.out.println(myScene.getSootClassPath());
    SootClass c = myScene.loadClassAndSupport(args[0]);
    myScene.loadNecessaryClasses();
    c.setApplicationClass();
    SootMethod m = c.getMethodByName(args[1]);
    Body b = m.retrieveActiveBody();
    UnitGraph g = new ExceptionalUnitGraph(b);
    Flow myFlow = new Flow(g);
  }
}
