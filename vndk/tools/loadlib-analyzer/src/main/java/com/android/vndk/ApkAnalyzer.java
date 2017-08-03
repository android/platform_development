package com.android.vndk;

import com.android.vndk.Flow;
import com.android.vndk.Formatter;

import java.io.IOException;
import java.util.Collections;
import java.util.Iterator;
import java.util.List;
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
import soot.dexpler.DexClassLoader;
import soot.jimple.AbstractStmtSwitch;
import soot.jimple.InvokeExpr;
import soot.jimple.InvokeStmt;
import soot.jimple.Jimple;
import soot.jimple.StringConstant;
import soot.options.Options;
import soot.util.Chain;
import soot.tagkit.SourceFileTag;
import soot.toolkits.graph.*;
import soot.toolkits.scalar.GuaranteedDefs;

import org.jf.dexlib2.DexFileFactory;
import org.jf.dexlib2.Opcodes;
import org.jf.dexlib2.iface.ClassDef;
import org.jf.dexlib2.iface.DexFile;
import org.jf.dexlib2.iface.Method;

import com.google.common.collect.Lists;

public class ApkAnalyzer {
    public static void main(String[] args) throws IOException {

        String androidPlatformPath = args[0];
        String appPath = args[1];

        Scene.v().setSootClassPath(Scene.v().defaultClassPath());                                        
        Scene.v().extendSootClassPath(System.getProperty("java.class.path"));   

        Options.v().set_src_prec(Options.src_prec_apk);
        Options.v().set_android_jars(androidPlatformPath);
        Options.v().set_process_dir(Collections.singletonList(appPath));
        Options.v().process_multiple_dex(true);
        Options.v().set_keep_line_number(true);

        Scene.v().loadNecessaryClasses();  

        Chain<SootClass> chain = Scene.v().getApplicationClasses();
        //Chain<SootClass> chain = Scene.v().getClasses();
        //System.out.println(chain);
        Formatter formatter = new Formatter();
        for (SootClass sootClass: chain) {
            SourceFileTag tag =
                    (SourceFileTag)sootClass.getTag("SourceFileTag");
            //System.out.println("FILE: " + tag.toString());

            for (SootMethod sootMethod : sootClass.getMethods()) {
                Body b = sootMethod.retrieveActiveBody();
                UnitGraph g = new ExceptionalUnitGraph(b);
                Flow myFlow = new Flow(
                                  g,
                                  formatter,
                                  sootClass.getName(),
                                  sootMethod.getName(),
                                  tag.toString());
            }
        }
        formatter.print();
   }
}

