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

"""Installs VNDK snapshot build artifact under prebuilts/vndk."""

import argparse
import glob
import logging
import os
import subprocess
import textwrap


THIS_DIR = os.path.realpath(os.path.dirname(__file__))
ANDROID_BUILD_TOP = os.path.realpath(os.path.join(THIS_DIR, '../../..'))
DIST_DIR = os.path.realpath(os.path.join(ANDROID_BUILD_TOP, 'out/dist'))
PREBUILTS_VNDK_DIR = os.path.realpath(os.path.join(ANDROID_BUILD_TOP, 'prebuilts/vndk'))


def logger():
    return logging.getLogger(__name__)


def check_call(cmd):
    logger().debug('Running `%s`', ' '.join(cmd))
    subprocess.check_call(cmd)


def fetch_artifact(branch, build, pattern):
    fetch_artifact_path = '/google/data/ro/projects/android/fetch_artifact'
    cmd = [fetch_artifact_path, '--branch', branch, '--target=linux',
           '--bid', build, pattern]
    check_call(cmd)


def start_branch(build):
    branch_name = 'update-' + (build or 'latest')
    logger().info('Creating branch %s', branch_name)
    check_call(['repo', 'start', branch_name, '.'])


def install_snapshot(branch, build):
    artifact_pattern = 'android-vndk-*.zip'
    artifact_root = ''
    if build != 'local':
        logger().info('Fetching %s from %s (artifacts matching %s)', build,
            branch, artifact_pattern)
        fetch_artifact(branch, build, artifact_pattern)
    else:
        logger().info('Fetching local VNDK snapshot from out/dist')
        local_snapshot = os.path.realpath(os.path.join(DIST_DIR, artifact_pattern))
        artifact_root = DIST_DIR
    artifacts = glob.glob(os.path.join(artifact_root, artifact_pattern))
    try:
        artifact_cnt = len(artifacts)
        assert artifact_cnt == 4, ('Only {} artifacts found. '
                                   'There should be four.'.format(artifact_cnt))
        for artifact in artifacts:
            logger().info('Unzipping VNDK snapshot: {}'.format(artifact))
            check_call(['unzip', '-q', artifact])
    finally:
        if artifact_root == '':
            for artifact in artifacts:
                os.unlink(artifact)


def commit(branch, build, version):
    logger().info('Making commit')
    check_call(['git', 'add', '.'])
    message = textwrap.dedent("""\
        Update VNDK snapshot v{version} to build {build}.

        Taken from branch {branch}.""").format(version=version, branch=branch, build=build)
    check_call(['git', 'commit', '-m', message])


def get_args():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        '-b', '--branch', default='local',
        help='Branch to pull build from.')
    parser.add_argument(
        'vndk_version', help='VNDK snapshot version to install, e.g. "27".')
    parser.add_argument(
        '--build', required=True,
        help='Build number to pull. Use "local" to pull a local build from out/dist.')
    parser.add_argument(
        '--use-current-branch', action='store_true',
        help='Perform the update in the current branch. Do not repo start.')
    parser.add_argument(
        '-v', '--verbose', action='count', default=0,
        help='Increase output verbosity, e.g. "-v", "-vv".')
    return parser.parse_args()


def main():
    args = get_args()

    vndk_version = args.vndk_version
    if not vndk_version.isdigit():
        raise ValueError('Invalid VNDK version: {}, must be an integer.'
                         .format(vndk_version))

    install_dir = os.path.join(PREBUILTS_VNDK_DIR, 'v{}'.format(vndk_version))
    if not os.path.isdir(install_dir):
        raise RuntimeError(
            'The directory for VNDK snapshot version {ver} does not exist.\n'
            'Please request a new git project for prebuilts/vndk/v{ver} '
            'before installing new snapshot.'
            .format(ver=vndk_version))

    verbose_map = (logging.WARNING, logging.INFO, logging.DEBUG)
    verbosity = args.verbose
    if verbosity > 2:
        verbosity = 2
    logging.basicConfig(level=verbose_map[verbosity])

    os.chdir(install_dir)

    if not args.use_current_branch:
        start_branch(args.build)
    install_snapshot(args.branch, args.build)

    if args.branch != 'local':
        commit(args.branch, args.build, vndk_version)


if __name__ == '__main__':
    main()
