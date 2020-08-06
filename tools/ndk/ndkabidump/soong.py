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
"""APIs for interacting with Soong."""
import json
import os
from pathlib import Path
import shutil
import subprocess
from typing import Any, Dict, List


class Soong:
    """Interface for interacting with Soong."""

    def __init__(self, build_top: Path, out_dir: Path) -> None:
        self.out_dir = out_dir
        self.soong_ui_path = build_top / 'build/soong/soong_ui.bash'

    def soong_ui(self, args: List[str], capture_output: bool = False) -> str:
        """Executes soong_ui.bash and returns the output on success.

        Args:
            args: List of string arguments to pass to soong_ui.bash.
            capture_output: True if the output of the command should be
                captured and returned. If not, the output will be printed to
                stdout/stderr and an empty string will be returned.

        Raises:
            subprocess.CalledProcessError: The subprocess failure if the soong
            command failed.

        Returns:
            The interleaved contents of stdout/stderr if capture_output is
            True, else an empty string.
        """
        exec_env = dict(os.environ)
        exec_env.update({'OUT_DIR': str(self.out_dir.resolve())})
        result = subprocess.run([str(self.soong_ui_path)] + args,
                                check=True,
                                capture_output=capture_output,
                                encoding='utf-8',
                                env=exec_env)
        return result.stdout

    def get_make_var(self, name: str) -> str:
        """Queries the build system for the value of a make variable.

        Args:
            name: The name of the build variable to query.

        Returns:
            The value of the build variable in string form.
        """
        return self.soong_ui(['--dumpvar-mode', name],
                             capture_output=True).rstrip('\n')

    def clean(self) -> None:
        """Removes the output directory, if it exists."""
        if self.out_dir.exists():
            shutil.rmtree(self.out_dir)

    def configure(self, additional_config: Dict[str, Any]) -> None:
        """Creates the soong.variables file for the build.

        Note that, while Soong does have defaults, the defaults are not used if
        custom configuration is required. As such, calling this function even
        with an empty argument may cause Soong to run with different defaults
        than if the function were not called, as we must populate the
        configuration with our own "defaults", that may not be complete or
        up-to-date with Soong's.

        The out directory will be created if it does not already exist.
        Existing contents will not be removed, but soong.variables will be
        overwritten.

        Args:
            additional_config: A JSON-compatible dict that will be merged into
                the default build configuration.
        """
        self.out_dir.mkdir(parents=True, exist_ok=True)
        soong_variables_path = self.out_dir / 'soong.variables'
        platform_sdk_version = self.get_make_var('PLATFORM_SDK_VERSION')
        # Comma-separated list of preview codenames the build is aware of.
        all_codenames = self.get_make_var(
            'PLATFORM_VERSION_ALL_CODENAMES').split(',')

        config = {
            'Platform_sdk_version': platform_sdk_version,
            'Platform_version_active_codenames': all_codenames,
            'DeviceName': 'generic_arm64',
            'HostArch': 'x86_64',
            'Malloc_not_svelte': False,
            'Safestack': False,
        }
        config.update(additional_config)
        with soong_variables_path.open('w') as soong_variables:
            json.dump(config, soong_variables, indent=2, sort_keys=True)

    def build(self, targets: List[str]) -> None:
        """Builds the given targets.

        The out directory will be created if it does not already exist.
        Existing contents will not be removed, but affected outputs will be
        modified.

        Args:
            targets: A list of target names to build.
        """
        self.out_dir.mkdir(parents=True, exist_ok=True)
        self.soong_ui(['--make-mode'] + targets)
