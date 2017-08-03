#!/bin/bash -ex

javac -Xlint:unchecked -cp lib/soot-trunk.jar -d classes \
  src/myFlowAnalysis/ClassAnalyzer.java \
  src/myFlowAnalysis/DexAnalyzer.java \
  src/myFlowAnalysis/Test.java \
  src/myFlowAnalysis/Flow.java

# Use dx to convert .class to .dex
cd classes && ./../dx  --dex --output=output.dex myFlowAnalysis/Test.class
mv output.dex ../ && cd ../

set +e
# java -cp lib/soot-trunk.jar:classes myFlowAnalysis.dexAnalyzer DEX_FILE
java -cp lib/soot-trunk.jar:classes myFlowAnalysis.DexAnalyzer output.dex > dex.txt
# java -cp lib/soot-trunk.jar:classes myFlowAnalysis.classAnalyzer CLASS_NAME METHOD_NAME
java -cp lib/soot-trunk.jar:classes myFlowAnalysis.ClassAnalyzer myFlowAnalysis.Test main > java.txt
