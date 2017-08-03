#!/bin/bash -ex

mvn  compile package 

# Use dx to convert .class to .dex
cd target/classes && ../../dx --dex --output=output.dex com/android/vndk/Test.class
mv output.dex ../.. && cd ../..
cp output.dex classes.dex
zip output.apk classes.dex

set +e

JAR="target/loadlib-analyzer-1.0-SNAPSHOT-jar-with-dependencies.jar"

# java -jar ${JAR} DEX_FILE
# java -jar ${JAR} output.dex > dex.txt
java -cp ${JAR} com.android.vndk.DexAnalyzer output.dex > dex.txt
# java -cp ${JAR} com.android.vndk.ClassAnalyzer CLASS_NAME METHOD_NAME
java -cp ${JAR} com.android.vndk.ClassAnalyzer com.android.vndk.Test main > class.txt

java -cp ${JAR} com.android.vndk.ApkAnalyzer android-platforms output.apk > apk.txt

