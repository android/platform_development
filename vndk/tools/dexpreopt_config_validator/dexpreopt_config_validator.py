#!/usr/bin/env python
#
# Copyright (C) 2021 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

import numbers
import json
import argparse
import pathlib


def NonNull(pred):
    def check(o):
        if o is None:
            raise ValueError("The field must be non-null")
        return pred(o)
    return check


def ObjectOf(schema):
    def check(obj):
        if obj is None:
            return True
        if not isinstance(obj, dict):
            raise ValueError("Expect dict, but", obj)
        json_only = set(obj.keys()) - set(schema.keys())

        if len(json_only) > 0:
            raise ValueError("Unexpected fields: ", json_only)

        for key, checker in schema.items():
            if key not in obj:
                continue
            if not checker(obj[key]):
                raise ValueError((key, obj[key]), "is problematic.")
        return True
    return check


def ListOf(elem_pred):
    def check(l):
        if l is None:
            return True
        if not isinstance(l, list):
            raise ValueError("Expect list, but", l)
        for item in l:
            if not elem_pred(item):
                raise ValueError(item, " in ", l, "is problematic.")
        return True
    return check


def DictOf(key_pred, val_pred):
    def check(d):
        if d is None:
            return True
        if not isinstance(d, dict):
            raise ValueError("Expect dict, but", d)
        for key, value in d.items():
            if not (key_pred(key) and val_pred(value)):
                raise ValueError((key, value), " in ", d, "is problematic.")
        return True
    return check


def String(s):
    if s is None:
        return True
    if not isinstance(s, str):
        print("Expect string, but", s)
        raise ValueError("Expect string, but", s)
    return True


def Number(n):
    if n is None:
        return True
    if not isinstance(n, numbers.Number):
        raise ValueError("Expect number, but", n)
    return True


def Bool(b):
    if b is None:
        return True
    if not isinstance(b, bool):
        raise ValueError("Expect bool, but", b)
    return True



ClassLoaderContexts = ObjectOf({
    "Name": String,
    "Host": String,
    "Device": String,
    "Subcontexts": ListOf(lambda x: ClassLoaderContexts(x)),
})

validateDexpreoptConfig = ObjectOf(
    {
        "BuildPath": String,
        "DexPath": String,
        "ManifestPath": String,
        "ProfileClassListing": String,
        "ProfileBootListing": String,
        "EnforceUsesLibrariesStatusFile": String,
        "ClassLoaderContexts": DictOf(String, ListOf(ClassLoaderContexts)),
        "DexPreoptImages": ListOf(String),
        "DexPreoptImagesDeps": ListOf(ListOf(String)),
        "PreoptBootClassPathDexFiles": ListOf(String),
        "Name": String,
        "DexLocation": String,
        "UncompressedDex": Bool,
        "HasApkLibraries": Bool,
        "PreoptFlags": ListOf(String),
        "ProfileIsTextListing": Bool,
        "EnforceUsesLibraries": Bool,
        "ProvidesUsesLibrary": String,
        "Archs": ListOf(String),
        "DexPreoptImageLocations": ListOf(String),
        "PreoptBootClassPathDexLocations": ListOf(String),
        "PreoptExtractedApk": Bool,
        "NoCreateAppImage": Bool,
        "ForceCreateAppImage": Bool,
        "PresignedPrebuilt": Bool,
        "ExtrasOutputPath": String,
        "Privileged": Bool
    }
)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        prog="dexpreopt_config_validator",
        usage="dexpreopt_config_validator path [path]..."
    )
    parser.add_argument('paths', nargs='+', type=pathlib.Path)
    args = parser.parse_args()
    for path in args.paths:
        with open(path) as fin:
            obj = json.load(fin)
            try:
                validateDexpreoptConfig(obj)
            except ValueError as e:
                print("invalid config:", path)
                raise e
