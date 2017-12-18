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

import utils


class GPLChecker(object):
    """Checks that all GPL projects in a VNDK snapshot have released sources.

    Makes sure that the current source tree have the sources for all GPL
    prebuilt libraries in a specified VNDK snapshot version.
    """
    MANIFEST_XML = 'manifest.xml'
    MODULE_PATHS_TXT = 'module_paths.txt'

    def __init__(self, install_dir, android_build_top):
        """GPLChecker constructor.

        Args:
          install_dir: string, absolute path to the prebuilts/vndk/v{version}
            directory where the build files will be generated.
          android_build_top: string, absolute path to ANDROID_BUILD_TOP
        """
        self._android_build_top = android_build_top
        self._install_dir = install_dir
        self._manifest_file = os.path.join(install_dir, self.MANIFEST_XML)
        self._notice_files_dir = os.path.join(install_dir, 'NOTICE_FILES')

        if not os.path.isfile(self._manifest_file):
            raise RuntimeError('{manifest} not found in {install_dir}'.format(
                manifest=self.MANIFEST_XML, install_dir=install_dir))

    def _parse_module_paths(self):
        """Parses the module_path.txt files into a dictionary,

        Returns:
          module_paths: dict, e.g. {libfoo.so: some/path/here}
        """
        module_paths = dict()
        for file in utils.find(self._install_dir, [self.MODULE_PATHS_TXT]):
            file_path = os.path.join(self._install_dir, file)
            with open(file_path, 'r') as f:
                for line in f.read().strip().split('\n'):
                    paths = line.split(' ')
                    if len(paths) > 1:
                        if paths[0] not in module_paths:
                            module_paths[paths[0]] = paths[1]
        return module_paths

    def _parse_manifest(self):
        """Parses manifest.xml file and returns list of 'project' tags."""

        root = xml_tree.parse(self._manifest_file).getroot()
        return root.findall('project')

    def _get_sha(self, module_path, manifest_projects):
        """Returns SHA value recorded in manifest.xml for given project path.

        Args:
          module_path: string, project path relative to ANDROID_BUILD_TOP
          manifest_projects: list of xml_tree.Element, list of 'project' tags
        """
        sha = None
        for project in manifest_projects:
            path = project.get('path')
            if module_path.startswith(path):
                sha = project.get('revision')
                break
        return sha

    def _check_sha_exists(self, sha, git_project_path):
        """Checks whether a SHA value is found in a git project of current tree.

        Args:
          sha: string, SHA revision value recorded in manifest.xml
          git_project_path: string, path relative to ANDROID_BUILD_TOP
        """
        os.chdir(
            utils.join_realpath(self._android_build_top, git_project_path))
        try:
            subprocess.check_call(['git', 'rev-list', 'HEAD..{}'.format(sha)])
            return True
        except subprocess.CalledProcessError:
            return False

    def check_gpl_projects(self):
        """Checks that all GPL projects have released sources.

        Raises:
          AssertionError: There are GPL projects with unreleased sources.
        """
        print 'Starting license check for GPL projects...'
        cmd = ['grep', '-l', 'GENERAL PUBLIC LICENSE']

        notice_files = glob.glob('{}/*'.format(self._notice_files_dir))
        if len(notice_files) == 0:
            raise RuntimeError(
                'No license files found in {}'.format(self._notice_files_dir))

        cmd.extend(notice_files)

        gpl_projects = []
        released_projects = []
        unreleased_projects = []
        try:
            gpl_files = subprocess.check_output(cmd)
            gpl_projects = []
            for file in gpl_files.strip().split('\n'):
                gpl_projects.append(os.path.splitext(os.path.basename(file))[0])
            print 'List of GPL projects found:', gpl_projects

            module_paths = self._parse_module_paths()
            manifest_projects = self._parse_manifest()

            for lib in gpl_projects:
                if lib in module_paths:
                    module_path = module_paths[lib]
                    sha = self._get_sha(module_path, manifest_projects)
                    if not sha:
                        raise RuntimeError(
                            'No project found for {} in {}'.
                            format(module_path, self.MANIFEST_XML))
                    sha_exists = self._check_sha_exists(sha, module_path)
                    if not sha_exists:
                        unreleased_projects.append((lib, module_path))
                    else:
                        released_projects.append((lib, module_path))
                else:
                    raise RuntimeError(
                        'No module path was found for {} in {}'.
                        format(lib, self.MODULE_PATHS_TXT))
        except subprocess.CalledProcessError:
            print 'No GPL projects found.'
            return

        if len(released_projects) > 0:
            print 'Released GPL projects:', released_projects

        assert len(unreleased_projects) == 0, ('FAIL: The following GPL '
            'projects have NOT been released in current source tree: {}'
            .format(unreleased_projects))

        print 'PASS: All GPL projects have source in current tree.'
        return


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
    ANDROID_BUILD_TOP = utils.get_android_build_top()
    PREBUILTS_VNDK_DIR = utils.join_realpath(ANDROID_BUILD_TOP,
                                             'prebuilts/vndk')

    args = get_args()
    vndk_version = args.vndk_version
    install_dir = os.path.join(PREBUILTS_VNDK_DIR, 'v{}'.format(vndk_version))
    if not os.path.isdir(install_dir):
        raise ValueError('Please provide valid VNDK version. {} does not exist.'
            .format(install_dir))

    license_checker = GPLChecker(install_dir, ANDROID_BUILD_TOP)
    try:
        license_checker.check_gpl_projects()
    except AssertionError as error:
        print error
        raise


if __name__ == '__main__':
    main()
