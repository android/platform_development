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
import sys
import textwrap

from gen_buildfiles import GenBuildFile

ANDROID_BUILD_TOP = os.getenv('ANDROID_BUILD_TOP')
if not ANDROID_BUILD_TOP:
    print('Error: Missing ANDROID_BUILD_TOP env variable. Please run '
          '\'. build/envsetup.sh; lunch <build target>\'. Exiting script.')
    sys.exit(1)

DIST_DIR = os.getenv('DIST_DIR')
if not DIST_DIR:
    OUT_DIR = os.getenv('OUT_DIR')
    if OUT_DIR:
        DIST_DIR = os.path.realpath(os.path.join(OUT_DIR, 'dist'))
    else:
        DIST_DIR = os.path.realpath(os.path.join(ANDROID_BUILD_TOP, 'out/dist'))

PREBUILTS_VNDK_DIR = os.path.realpath(
    os.path.join(ANDROID_BUILD_TOP, 'prebuilts/vndk'))


def logger():
    return logging.getLogger(__name__)


def check_call(cmd):
    logger().debug('Running `{}`'.format(' '.join(cmd)))
    subprocess.check_call(cmd)


def fetch_artifact(branch, build, pattern):
    fetch_artifact_path = '/google/data/ro/projects/android/fetch_artifact'
    cmd = [
        fetch_artifact_path, '--branch', branch, '--target=vndk', '--bid',
        build, pattern
    ]
    check_call(cmd)


def start_branch(build):
    branch_name = 'update-' + (build or 'local')
    logger().info('Creating branch {branch} in {dir}'.format(
        branch=branch_name, dir=os.getcwd()))
    check_call(['repo', 'start', branch_name, '.'])


def install_snapshot(branch, build):
    artifact_pattern = 'android-vndk-*.zip'
    artifact_dir = os.getcwd()
    if branch and build:
        logger().info('Fetching {pattern} from {branch} (bid: {build})'.format(
            pattern=artifact_pattern, branch=branch, build=build))
        fetch_artifact(branch, build, artifact_pattern)

        manifest_file = 'manifest_{}.xml'.format(build)
        logger().info('Fetching {file} from {branch} (bid: {build})'.format(
            file=manifest_file, branch=branch, build=build))
        fetch_artifact(branch, build, manifest_file)
    else:
        logger().info('Fetching local VNDK snapshot from {}'.format(DIST_DIR))
        artifact_dir = DIST_DIR
    artifacts = glob.glob(os.path.join(artifact_dir, artifact_pattern))
    try:
        artifact_cnt = len(artifacts)
        if artifact_cnt < 4:
            raise RuntimeError('Number of artifacts found in {path} is {cnt}. '
                               'There should be four.'.format(
                                   path=artifact_dir, cnt=artifact_cnt))
        for artifact in artifacts:
            logger().info('Unzipping VNDK snapshot: {}'.format(artifact))
            check_call(['unzip', '-q', artifact])
    finally:
        if artifact_dir == os.getcwd():
            for artifact in artifacts:
                os.unlink(artifact)


def update_buildfiles(buildfile_generator):
    logger().info('Updating Android.mk')
    buildfile_generator.generate_android_mk()

    logger().info('Updating Android.bp')
    buildfile_generator.generate_android_bp()


def commit(branch, build, version):
    logger().info('Making commit')
    check_call(['git', 'add', '.'])
    message = textwrap.dedent("""\
        Update VNDK snapshot v{version} to build {build}.

        Taken from branch {branch}.""").format(
        version=version, branch=branch, build=build)
    check_call(['git', 'commit', '-m', message])


def get_args():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        'vndk_version',
        type=int,
        help='VNDK snapshot version to install, e.g. "27".')
    parser.add_argument(
        '-m',
        '--mode',
        required=True,
        choices=['rel', 'dev'],
        help=(
            'Choose between release ("rel") or develop mode ("dev"). '
            'In release mode, branch and build number must be provided and a '
            'git commit is automatically uploaded to Gerrit for review. '
            'In develop mode, the --local option can be set to '
            'install a local VNDK snapshot build from out/dist.'))
    parser.add_argument('-b', '--branch', help='Branch to pull build from.')
    parser.add_argument('--build', help='Build number to pull.')
    parser.add_argument(
        '--local',
        action='store_true',
        help=
        'Fetch local VNDK snapshot build from out/dist. This option is only available in "dev" mode.'
    )
    parser.add_argument(
        '--use-current-branch',
        action='store_true',
        help='Perform the update in the current branch. Do not repo start.')
    parser.add_argument(
        '-v',
        '--verbose',
        action='count',
        default=1,
        help='Increase output verbosity, e.g. "-v", "-vv".')
    return parser.parse_args()


def main():
    """Program entry point."""
    args = get_args()

    if args.mode == 'rel':
        if args.local:
            raise ValueError(
                'In release mode, the --local option cannot be used.')
        if not args.branch or not args.build:
            raise ValueError(
                'In release mode, both branch and build are required.')
    elif args.mode == 'dev':
        if bool(args.branch) != bool(args.build):
            raise ValueError(
                'In developer mode, if branch is specified then build is required, and vice versa.'
            )
        elif not (args.branch and args.build or args.local):
            raise ValueError(
                'In developer mode, please provide branch and build or set --local option.'
            )

    if args.local and not os.path.isdir(DIST_DIR):
        raise RuntimeError(
            'The --local option is set, but DIST_DIR={} does not exist.'.
            format(DIST_DIR))

    vndk_version = str(args.vndk_version)

    install_dir = os.path.join(PREBUILTS_VNDK_DIR, 'v{}'.format(vndk_version))
    if not os.path.isdir(install_dir):
        raise RuntimeError(
            'The directory for VNDK snapshot version {ver} does not exist.\n'
            'Please request a new git project for prebuilts/vndk/v{ver} '
            'before installing new snapshot.'.format(ver=vndk_version))

    verbose_map = (logging.WARNING, logging.INFO, logging.DEBUG)
    verbosity = min(args.verbose, 2)
    logging.basicConfig(level=verbose_map[verbosity])

    os.chdir(install_dir)

    if not args.use_current_branch:
        start_branch(args.build)

    install_snapshot(args.branch, args.build)

    buildfile_generator = GenBuildFile(install_dir, vndk_version)
    update_buildfiles(buildfile_generator)

    if args.mode == 'rel':
        commit(args.branch, args.build, vndk_version)


if __name__ == '__main__':
    main()
