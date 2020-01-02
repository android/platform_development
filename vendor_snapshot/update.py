#!/usr/bin/env python3
#
# Copyright (C) 2020 The Android Open Source Project
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
"""Installs vendor snapshot under prebuilts/vendor/v{version}."""

import argparse
import glob
import logging
import os
import re
import shutil
import subprocess
import sys
import tempfile
import textwrap
import json

def get_target_arch(json_rel_path):
    return json_rel_path.split('/')[0]

def get_arch(json_rel_path):
    return json_rel_path.split('/')[1].split('-')[1]

def get_variation(json_rel_path):
    return json_rel_path.split('/')[2]

def list_to_prop_value(l, name, ind):
    if len(l) == 0:
        return ''
    values=(',\n' + ind + '    ').join(['"%s"' % d for d in l])
    return (ind+'%s: [\n'+
            ind+'    %s,\n'+
            ind+'],\n') % (name, values)

def filter_invalid_dirs(dirs, bp_dir, module_name):
    ret = []
    for d in dirs:
        dir_path = os.path.join(bp_dir, d)
        if os.path.isdir(dir_path):
            ret.append(d)
        else:
            logging.warning(
                'Exported dir "%s" of module "%s" does not exist' % (d, module_name))
    return ret

def gen_prop(prop, bp_dir, module_name, ind):
    bp = ''
    if 'Path' in prop:
        bp += ind + 'srcs: ["%s"],\n' % prop['Path']
    if 'RelativeInstallPath' in prop:
        bp += ind + 'relative_install_path: "%s",\n' % prop['RelativeInstallPath']
    if 'ExportedDirs' in prop:
        dirs = [os.path.join('include', x) for x in prop['ExportedDirs']]
        bp += list_to_prop_value(
            filter_invalid_dirs(dirs, bp_dir, module_name),
            'export_include_dirs',
            ind)
    if 'ExportedSystemDirs' in prop:
        dirs = [os.path.join('include', x) for x in prop['ExportedSystemDirs']]
        bp += list_to_prop_value(
            filter_invalid_dirs(dirs, bp_dir, module_name),
            'export_system_include_dirs',
            ind)
    if 'ExportedFlags' in prop:
        bp += list_to_prop_value(prop['ExportedFlags'], 'export_flags', ind)
    if 'Symlinks' in prop:
        bp += list_to_prop_value(prop['Symlinks'], 'symlinks', ind)
    return bp

def gen_arch_prop(arch, bp_dir, module_name, prop):
    bp = ''
    propbp = gen_prop(prop, bp_dir, module_name, '            ')
    if propbp != '':
        bp += '        %s: {\n' % arch
        bp += propbp
        bp += '        },\n'
    return bp

def gen_phony_module(name, deps):
    if len(deps) == 0:
        return ''
    bp = 'phony {\n'
    bp += '    name: "%s",\n' % name
    bp += list_to_prop_value(deps, 'required', '    ')
    return bp

def gen_bp_module(variation, name, version, target_arch, arch_props, bp_dir):
    bp = 'vendor_snapshot_%s {\n' % variation
    bp += '    name: "%s",\n' % name
    bp += '    version: "%d",\n' % version
    bp += '    target_arch: "%s",\n' % target_arch
    bp += '    vendor: true,\n'

    common_props = None
    for arch in arch_props:
        if common_props is None:
            common_props = dict()
            for k in arch_props[arch]:
                common_props[k] = arch_props[arch][k]
            continue
        for k in arch_props[arch]:
            if k in common_props and common_props[k] != arch_props[arch][k]:
                del common_props[k]

    bp += gen_prop(common_props, bp_dir, name, '    ')

    archbp = ''
    for arch in arch_props:
        for k in common_props:
            if k in arch_props[arch]:
                del arch_props[arch][k]
        archbp += gen_arch_prop(arch, bp_dir, name, arch_props[arch])

    if archbp != '':
        bp += '    arch: {\n'
        bp += archbp
        bp += '    },\n'
    bp += '}\n\n'
    return bp

def process_prop(prop, arch, variation_dict):
    name = prop['ModuleName']
    if not name in variation_dict:
        variation_dict[name] = dict()
    variation_dict[name][arch] = prop

def get_args():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        'snapshot_version',
        type=int,
        help='Vendor snapshot version to install, e.g. "30".')
    parser.add_argument(
        '-v',
        '--verbose',
        action='count',
        default=0,
        help='Increase output verbosity, e.g. "-v", "-vv".')
    return parser.parse_args()

def main():
    """Program entry point."""
    args = get_args()
    verbose_map = (logging.WARNING, logging.INFO, logging.DEBUG)
    verbosity = min(args.verbose, 2)
    logging.basicConfig(
        format='%(levelname)-8s [%(filename)s:%(lineno)d] %(message)s',
        level=verbose_map[verbosity])
    install_dir = os.path.join('prebuilts', 'vendor', 'v'+str(args.snapshot_version))

    # props[target_arch]["static"|"shared"|"binary"|"header"][name][arch] : json
    props = dict()

    # {target_arch}/{arch}/{variation}/{module}.json
    for root, _, files in os.walk(install_dir):
        for file_name in sorted(files):
            if not file_name.endswith('.json'):
                continue
            full_path = os.path.join(root, file_name)
            rel_path = os.path.relpath(full_path, install_dir)

            target_arch = get_target_arch(rel_path)
            arch = get_arch(rel_path)
            variation = get_variation(rel_path)

            if not target_arch in props:
                props[target_arch] = dict()
            if not variation in props[target_arch]:
                props[target_arch][variation] = dict()

            with open(full_path, 'r') as f:
                prop = json.load(f)
                # Remove .json after parsing?
                # os.unlink(full_path)

            if variation != 'header':
                prop['Path'] = os.path.relpath(
                    rel_path[:-5], # removing .json
                    target_arch)
            process_prop(prop, arch, props[target_arch][variation])

    for target_arch in props:
        androidbp = ''
        bp_dir = os.path.join(install_dir, target_arch)
        for variation in props[target_arch]:
            for name in props[target_arch][variation]:
                androidbp += gen_bp_module(
                    variation,
                    name,
                    args.snapshot_version,
                    target_arch,
                    props[target_arch][variation][name],
                    bp_dir)
        with open(os.path.join(bp_dir, 'Android.bp'), 'w') as f:
            f.write(androidbp)

if __name__ == '__main__':
    main()
