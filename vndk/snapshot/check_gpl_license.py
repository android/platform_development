#!/usr/bin/env python
#
# Copyright (C) 2017 The Android Open Source Project
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

import argparse
import glob
import os
import subprocess
import xml.etree.ElementTree as xml_tree

from gen_buildfiles import find


class GPLChecker(object):
    """Checks that all GPL projects in a VNDK snapshot have released sources.

    Makes sure that the current source tree have the sources for all GPL
    licensed prebuilt libraries in a specified VNDK snapshot version.
    """
    MANIFEST_XML = 'manifest.xml'
    MODULE_PATHS_TXT = 'module_paths.txt'

    def __init__(self, install_dir, ANDROID_BUILD_TOP):
        self._android_build_top = ANDROID_BUILD_TOP
        self._install_dir = install_dir
        self._manifest_file = os.path.join(install_dir, self.MANIFEST_XML)
        self._notice_files_dir = os.path.join(install_dir, 'NOTICE_FILES')

        if not os.path.isfile(self._manifest_file):
            raise RuntimeError('{manifest} not found in {install_dir}'.format(manifest=MANIFEST_XML, install_dir=install_dir))

    def _parse_module_paths(self):
        module_paths = dict()
        for file in find(self._install_dir, [self.MODULE_PATHS_TXT]):
            file_path = os.path.join(self._install_dir, file)
            with open(file_path, 'r') as f:
                for line in f.read().strip().split('\n'):
                    paths = line.split(' ')
                    if len(paths) > 1:
                        if paths[0] not in module_paths:
                            module_paths[paths[0]] = paths[1]
        return module_paths

    def _parse_manifest(self):
        root = xml_tree.parse(self._manifest_file).getroot()
        return root.findall('project')

    def _get_sha(self, module_path, manifest_projects):
        sha = None
        for project in manifest_projects:
            path = project.get('path')
            if module_path.startswith(path):
                sha = project.get('revision')
                break
        return sha

    def _check_sha_exists(self, sha, git_project_path):
        os.chdir(git_project_path)
        try:
            subprocess.check_call(['git', 'rev-list', 'HEAD..{}'.format(sha)])
            return True
        except subprocess.CalledProcessError:
            return False

    def check_gpl_projects(self):
        """
        Return:
          (bool, unreleased_projects list)
        """
        cmd = ['grep', '-l', 'GENERAL PUBLIC LICENSE']

        notice_files = glob.glob('{}/*'.format(self._notice_files_dir))
        if len(notice_files) == 0:
            return True, []

        cmd.extend(notice_files)

        unreleased_projects = []
        try:
            gpl_projects = subprocess.check_output(cmd)
            module_paths = self._parse_module_paths()
            manifest_projects = self._parse_manifest()
            for project in gpl_projects.strip().split('\n'):
                lib = os.path.splitext(os.path.basename(project))[0]
                if lib in module_paths:
                    module_path = module_paths[lib]
                    sha = self._get_sha(module_path, manifest_projects)
                    if not sha:
                        raise RuntimeError('No project found for module path, {}, in manifest.xml.'.format(module_path))
                    git_project_path = os.path.join(self._android_build_top, module_path)
                    sha_exists = self._check_sha_exists(sha, git_project_path)
                    if not sha_exists:
                        unreleased_projects.append((lib, module_path))
                else:
                    raise RuntimeError('No module path was found for {} in module_paths.txt.'.format(lib))
        except subprocess.CalledProcessError:
            print 'No GPL projects found.'

        result = len(unreleased_projects) == 0
        return result, unreleased_projects


def get_args():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        'vndk_version', type=int,
        help='VNDK snapshot version to check, e.g. "27".')
    return parser.parse_args()


def main():
    """For local testing purposes.

    Note: VNDK snapshot must be already installed under
      prebuilts/vndk/v{version}.
    """
    ANDROID_BUILD_TOP = os.getenv('ANDROID_BUILD_TOP')
    if not ANDROID_BUILD_TOP:
        print('Error: Missing ANDROID_BUILD_TOP env variable. Please run '
              '\'. build/envsetup.sh; lunch <build target>\'. Exiting script.')
        sys.exit(1)
    PREBUILTS_VNDK_DIR = os.path.realpath(
        os.path.join(ANDROID_BUILD_TOP, 'prebuilts/vndk'))

    args = get_args()
    vndk_version = args.vndk_version
    install_dir = os.path.join(PREBUILTS_VNDK_DIR, 'v{}'.format(vndk_version))
    if not os.path.isdir(install_dir):
        raise ValueError('Please provide valid VNDK version. {} does not exist.'.format(install_dir))

    license_checker = GPLChecker(install_dir, ANDROID_BUILD_TOP)
    check, unreleased_projects = license_checker.check_gpl_projects()
    if check:
        print 'PASS: All GPL projects have been released.'
    else:
        print 'FAIL: The following GPL projects have NOT been released in current source tree: {}'.format(unreleased_projects)
        sys.exit(1)


if __name__ == '__main__':
    main()
