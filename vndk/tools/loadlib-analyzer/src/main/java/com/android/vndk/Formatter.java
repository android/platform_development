package com.android.vndk;

import java.util.SortedMap;
import java.util.SortedSet;
import java.util.TreeMap;
import java.util.TreeSet;

public class Formatter {
    private SortedMap<String, Method> map = 
            new TreeMap<String, Method>();

    public void add(String clas, String method, String invoke, String lib) {
        if (map.containsKey(clas)) {
            map.get(clas).add(method, invoke, lib);
        } else {
            Method set = new Method();
            set.add(method, invoke, lib);
            map.put(clas, set);
        }
    }
    public void print() {
        for(SortedMap.Entry<String, Method> entry: map.entrySet()) {
            System.out.println(entry.getKey());
            entry.getValue().print();
        }
    }
    public boolean isEmpty() {
        return map.isEmpty();
    }
}

class Method {
    private SortedMap<String, Invocation> map = 
            new TreeMap<String, Invocation>();

    public void add(String method, String invoke, String lib) {
        if (map.containsKey(method)) {
            map.get(method).add(invoke, lib);
        } else {
            Invocation set = new Invocation();
            set.add(invoke, lib);
            map.put(method, set);
        }
    }
    public void print() {
        for(SortedMap.Entry<String, Invocation> entry: map.entrySet()) {
            System.out.println("\t" + entry.getKey());
            entry.getValue().print();
        }
    }
}

class Invocation {
    private SortedMap<String, SortedSet<String>> map = 
            new TreeMap<String, SortedSet<String>>();
    public void add(String invoke, String lib) {
        if (map.containsKey(invoke)) {
            map.get(invoke).add(lib);
        } else {
            SortedSet<String> set = new TreeSet<String>();
            set.add(lib);
            map.put(invoke, set);
        }
    }
    public void print() {
        for(SortedMap.Entry<String, SortedSet<String>> entry: map.entrySet()) {
            System.out.println("\t\t"+entry.getKey());
            for (String lib: entry.getValue()) {
                System.out.println("\t\t\t"+lib);
            }
        }
    }
}
