package myFlowAnalysis;

import java.util.*;


import soot.*;
import soot.options.*;
import soot.toolkits.graph.*;
import soot.toolkits.scalar.*;

/**
 * Find all locals guaranteed to be defined at (just before) a given
 * program point.
 *
 * @author Navindra Umanee
 **/
public class Flow {
    protected Map<Unit, List<Value>> unitToGuaranteedDefs;

    public Flow(UnitGraph graph) {
        FlowAnalysis analysis = new FlowAnalysis(graph);

        unitToGuaranteedDefs = new HashMap<Unit, List<Value>>(
              graph.size() * 2 + 1, 0.7f);

        for (Unit s : graph) {
            FlowSet<Value> set = analysis.getFlowBefore(s);
            unitToGuaranteedDefs.put(
                s, Collections.unmodifiableList(set.toList()));
        }

        for (Unit s : graph) {
            System.out.println("Unit\n" + s + " : " + s.getClass());
            for (ValueBox use : s.getUseBoxes()) {
                System.out.println("    Use: " + use + " : " +
                                   use.getClass());

                Value v = use.getValue();
                System.out.println("    ----> " + v + " : " + v.getClass());
            }
            System.out.println("    RegLine: " + getResult(s));
        }
    }

    public String getResult(Unit s) {
        StringBuilder sb = new StringBuilder();
        sb.append('[');
        boolean isFirst = true;
        for (Value v : unitToGuaranteedDefs.get(s)) {
            if (isFirst) {
                isFirst = false;
            } else {
                sb.append(", ");
            }

            sb.append(v.getClass());
            sb.append(": ");
            sb.append(v.toString());
        }
        sb.append(']');
        return sb.toString();
    }
}

class FlowAnalysis extends ForwardFlowAnalysis<Unit, FlowSet<Value>>
{
    FlowSet<Value> emptySet = new ArraySparseSet<Value>();
    Map<Unit, FlowSet<Value>> unitToGenerateSet;

    FlowAnalysis(UnitGraph graph)
    {
        super(graph);

        DominatorsFinder<Unit> df = new MHGDominatorsFinder<Unit>(graph);
        unitToGenerateSet = new HashMap<Unit, FlowSet<Value>>(
            graph.size() * 2 + 1, 0.7f);

        // pre-compute generate sets
        for (Unit s : graph) {
            FlowSet<Value> genSet = emptySet.clone();
            
            for (Unit dom : df.getDominators(s)) {
                for (ValueBox box : dom.getDefBoxes()) {
                    Value v = box.getValue();
                    if (v instanceof Local) {
                        genSet.add(v, genSet);
                    }
                }
            }
            
            unitToGenerateSet.put(s, genSet);
        }

        doAnalysis();
    }

    @Override
    protected FlowSet<Value> newInitialFlow() {
        return emptySet.clone();
    }

    @Override
    protected FlowSet<Value> entryInitialFlow() {
        return emptySet.clone();
    }

    @Override
    protected void flowThrough(FlowSet<Value> in, Unit unit,
                               FlowSet<Value> out) {
        in.union(unitToGenerateSet.get(unit), out);
    }

    @Override
    protected void merge(FlowSet<Value> in1, FlowSet<Value> in2,
                         FlowSet<Value> out) {
        in1.intersection(in2, out);
    }

    @Override
    protected void copy(FlowSet<Value> source, FlowSet<Value> dest) {
        source.copy(dest);
    }
}
