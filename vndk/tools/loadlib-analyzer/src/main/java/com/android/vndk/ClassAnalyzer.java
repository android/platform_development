package com.android.vndk;

import com.android.vndk.Flow;
import com.android.vndk.Formatter;

import java.util.Iterator;
import java.util.Map;

import soot.Body;
import soot.Scene;
import soot.SootClass;
import soot.SootMethod;
import soot.Transform;
import soot.Unit;
import soot.dexpler.DexClassLoader;
import soot.jimple.AbstractStmtSwitch;
import soot.jimple.InvokeExpr;
import soot.jimple.InvokeStmt;
import soot.jimple.Jimple;
import soot.jimple.StringConstant;
import soot.options.Options;
import soot.tagkit.SourceFileTag;
import soot.toolkits.graph.*;

import org.jf.dexlib2.iface.ClassDef;

public class ClassAnalyzer {
    public static void main(String[] args) {

        Options.v().set_keep_line_number(true);

        Scene.v().setSootClassPath(Scene.v().defaultClassPath());                                        
        Scene.v().extendSootClassPath(System.getProperty("java.class.path"));   

        SootClass sootClass = Scene.v().loadClassAndSupport(args[0]);
        Scene.v().loadNecessaryClasses();

        SourceFileTag tag =
                (SourceFileTag)sootClass.getTag("SourceFileTag");

        sootClass.setApplicationClass();
        SootMethod sootMethod = sootClass.getMethodByName(args[1]);
        Body b = sootMethod.retrieveActiveBody();
        UnitGraph g = new ExceptionalUnitGraph(b);

        Formatter formatter = new Formatter();

        Flow myFlow = new Flow(g,
                               formatter,
                               sootClass.getName(),
                               sootMethod.getName(),
                               tag.toString());
        formatter.print();
    }
}
