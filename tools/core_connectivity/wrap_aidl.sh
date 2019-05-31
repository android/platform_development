#!/bin/bash

# Generates a Java class for the specified a frozen AIDL interface. All methods must be oneway void.
#
# Example usage:
# ./wrap_aidl.sh frameworks/base/services/net/aidl/networkstack/3/android/net/INetworkMonitor.aidl \
#     > frameworks/base/services/net/java/android/net/NetworkMonitorManager.java

set -e

if [[ -z "$1" ]] || ! [[ "$1" =~ [.]aidl$ ]]; then
    echo "Usage: $0 <path/to/file.aidl>"
    exit 1
fi

file=$1
cat $file > /dev/null || exit

pkgstatement=$(cat $file | egrep "^package ")
imports="android.annotation.NonNull android.os.Binder android.os.RemoteException android.util.Log"

function collectImports() {
    local line=$1
    local args=$(echo $line | sed -re 's/.*\(//' -e 's/\).*//')

    # Strip in/out/inout modifiers.
    args=$(echo $args | sed -re 's/(^|, )(in|out|inout) /\1/g')

    # Find imports.
    for arg in $args; do
       if [[ $arg =~ [a-zA-Z0-9]+[.][a-zA-Z0-9]+ ]]; then
           imports="$imports $arg"
       fi
    done
}

function makeMethod() {
    local line=$1

    local returntype=$(echo $line | awk '{ print $1; }')
    local methodname=$(echo $line | awk '{ print $2; }' | sed -re 's/\(.*//')
    if [[ "$returntype" != "void" ]]; then
        echo "Invalid return type $returntype" >&2
        exit 1
    fi
    local args=$(echo $line | sed -re 's/.*\(//' -e 's/\).*//')

    # Strip in/out/inout modifiers.
    args=$(echo $args | sed -re 's/(^|, )(in|out|inout) /\1/g')

    # Find imports.
    for arg in $args; do
       if [[ $arg =~ [a-zA-Z0-9]+[.][a-zA-Z0-9]+ ]]; then
           imports="$imports $arg"
       fi
    done
    args=$(echo $args | sed -re 's/([a-zA-Z0-9]+[.])+([a-zA-Z0-9]+)/\2/g')

    argnames=$(echo $args | sed -re 's/(^|, )([0-9A-Za-z]+) ([0-9A-Za-z]+)/\1\3/g')

cat << EOF
    public boolean $methodname(${args}) {
        final long token = Binder.clearCallingIdentity();
        try {
            $wrappedmember.$methodname($argnames);
            return true;
        } catch (RemoteException e) {
            log("Error in $methodname", e);
            return false;
        } finally {
            Binder.restoreCallingIdentity(token);
        }
    }

EOF
}

file=$1
ifacename=$(basename $file .aidl)                       # IFoo
wrappedclass=$(echo $ifacename | sed -re 's/^I//')      # Foo
wrappedmember=m$wrappedclass                            # mFoo
classname=$(echo $ifacename | sed -re 's/^I//')Manager  # FooManager
varname=$(echo $classname | sed -r 's/^(\w)/\L\1/')     # fooManager

exec {fd}<${file}

while read -u $fd line; do
    if echo $line | egrep -q "\(.*\)"; then
        line=$(echo $line | sed -re 's/oneway //')
        collectImports "$line"
    fi
done

cat << EOF
/*
 * Copyright (C) $(date +%Y) The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

$pkgstatement

EOF

for import in $imports; do
    echo "import $import;"
done | sort | uniq
echo

cat << EOF
/**
 * A convenience wrapper for $ifacename.
 *
 * Wraps $ifacename calls, making them a bit more friendly to use. Currently handles:
 * - Clearing calling identity
 * - Ignoring RemoteExceptions
 * - Converting to stable parcelables
 *
 * By design, all methods on $ifacename are asynchronous oneway IPCs and are thus void. All the
 * wrapper methods in this class return a boolean that callers can use to determine whether
 * RemoteException was thrown.
 */
public class $classname {

    @NonNull private final $ifacename $wrappedmember;
    @NonNull private final String mTag;

    public $classname(@NonNull $ifacename $varname, @NonNull String tag) {
        $wrappedmember = $varname;
        mTag = tag;
    }

    public $classname(@NonNull $ifacename $varname) {
        this($varname, $classname.class.getSimpleName());
    }

    private void log(String s, Throwable e) {
        Log.e(mTag, s, e);
    }

EOF

exec {fd}<${file}

while read -u $fd line; do
    if echo $line | egrep -q "\(.*\)"; then
        line=$(echo $line | sed -re 's/oneway //')
        makeMethod "$line"
    fi
done

echo "}"
