package com.android.vndk;
import java.util.*;
import soot.*;
import soot.jimple.AssignStmt;
import soot.jimple.InvokeExpr;
import soot.jimple.InvokeStmt;
import soot.jimple.StringConstant;
import soot.options.*;
import soot.toolkits.graph.*;
import soot.toolkits.scalar.*;

import com.android.vndk.Formatter;

public class Flow {
    private static final String SYSTEM_LOAD_LIBRARY =
            "<java.lang.System: void loadLibrary(java.lang.String)>";
    public Flow(UnitGraph graph, Formatter formatter, String clas, String method, String sourceFile) {
        FlowAnalysis analysis = new FlowAnalysis(graph);
        for (Unit s : graph) {
            if (!(s instanceof InvokeStmt)) {
                continue;
            }
            InvokeStmt stmt = (InvokeStmt)s;
            InvokeExpr expr = stmt.getInvokeExpr();
            SootMethodRef methodRef = expr.getMethodRef();
            if (!SYSTEM_LOAD_LIBRARY.equals(methodRef.toString())) {
                continue;
            }
            //System.out.println("# " + sourceFile + " : " +
            //                   stmt.getJavaSourceStartLineNumber());
            Value libNameArg = expr.getArg(0);
            if (libNameArg instanceof StringConstant) {
                //System.out.println("Arg: " + libNameArg.toString() + " : " +
                //                   libNameArg.getClass());
                formatter.add(
                        clas,
                        method,
                        sourceFile + ":" + stmt.getJavaSourceStartLineNumber(), 
                        libNameArg.toString());
                continue;
            }
            if (libNameArg instanceof Local) {
                Local local = (Local)libNameArg;
                HashMap<Local, HashSet<String>> vs = analysis.getFlowBefore(s);
                HashSet<String> ps = vs.get(libNameArg);
                if (ps == null) {
                    System.out.println("Arg: <undefined-or-bug>");
                } else {

                    for (String pss : ps) {
                        //System.out.println("PossibleArg: " + pss);
                        formatter.add(
                         clas,
                         method,
                         sourceFile + ":" + stmt.getJavaSourceStartLineNumber(), 
                         pss);
                    }
                }
                continue;
            }
        }
    }
}
class FlowAnalysis
        extends ForwardFlowAnalysis<Unit, HashMap<Local, HashSet<String>>> {
    HashMap<Local, HashSet<String>> emptySet =
            new HashMap<Local, HashSet<String>>();
    HashMap<Unit, HashMap<Local, HashSet<String>>> unitToGenSet;
    private static void upsertLocalSetValue(HashMap<Local, HashSet<String>> map,
                                            Local local, String value) {
        if (map.containsKey(local)) {
            map.get(local).add(value);
        } else {
            HashSet<String> set = new HashSet<String>();
            set.add(value);
            map.put(local, set);
        }
    }
    private static void upsertLocalSet(HashMap<Local, HashSet<String>> map,
                                       Local local, HashSet<String> updateSet) {
        HashSet<String> existSet = map.get(local);
        if (existSet == null) {
            map.put(local, (HashSet<String>)updateSet.clone());
        } else {
            existSet.addAll(updateSet);
        }
    }
    private static void mergeLocalSetMap(HashMap<Local, HashSet<String>> in,
                                         HashMap<Local, HashSet<String>> out) {
        for (Map.Entry<Local, HashSet<String>> ent : in.entrySet()) {
            upsertLocalSet(out, ent.getKey(), ent.getValue());
        }
    }
    FlowAnalysis(UnitGraph graph) {
        super(graph);
        unitToGenSet = new HashMap<Unit, HashMap<Local, HashSet<String>>>();
        for (Unit s : graph) {
            HashMap<Local, HashSet<String>> genSet =
                    (HashMap<Local, HashSet<String>>)emptySet.clone();
            for (ValueBox box : s.getDefBoxes()) {
                Value v = box.getValue();
                if (v instanceof Local) {
                    Local local = (Local)v;
                    if (s instanceof AssignStmt) {
                        AssignStmt assign = (AssignStmt)s;
                        Value rhs = assign.getRightOp();
                        if (rhs instanceof StringConstant) {
                            upsertLocalSetValue(genSet, local, rhs.toString());
                        } else {
                            upsertLocalSetValue(genSet, local, "#");
                        }
                    } else {
                        upsertLocalSetValue(genSet, local, "*");
                    }
                }
            }
            unitToGenSet.put(s, genSet);
        }
        doAnalysis();
    }
    @Override
    protected HashMap<Local, HashSet<String>> newInitialFlow() {
        return (HashMap<Local, HashSet<String>>)emptySet.clone();
    }
    @Override
    protected HashMap<Local, HashSet<String>> entryInitialFlow() {
        return (HashMap<Local, HashSet<String>>)emptySet.clone();
    }
    @Override
    protected void flowThrough(HashMap<Local, HashSet<String>> in, Unit unit,
                               HashMap<Local, HashSet<String>> out) {
        out.putAll(in);
        out.putAll(unitToGenSet.get(unit));
    }
    @Override
    protected void merge(HashMap<Local, HashSet<String>> in1,
                         HashMap<Local, HashSet<String>> in2,
                         HashMap<Local, HashSet<String>> out) {
        mergeLocalSetMap(in1, out);
        mergeLocalSetMap(in2, out);
    }
    @Override
    protected void copy(HashMap<Local, HashSet<String>> source,
                        HashMap<Local, HashSet<String>> dest) {
        dest.clear();
        dest.putAll(source);
    }
}

