#!/usr/bin/env python
#
# Copyright 2017 - The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
"""Provide the information list for command 'dump'."""

from gsi_util.dumpers.prop_dumper import PropDumper
from gsi_util.dumpers.xml_dumper import XmlDumper

SYSTEM_MATRIX_DUMPER = (XmlDumper, '/system/compatibility_matrix.xml')
SYSTEM_BUILD_PROP_DUMPER = (PropDumper, '/system/build.prop')
SYSTEM_MANIFEST_DUMPER = (PropDumper, '/system/manifest.xml')

VENDOR_DEFAULT_PROP_DUMPER = (PropDumper, '/vendor/default.prop')
VENDOR_BUILD_PROP_DUMPER = (PropDumper, '/vendor/build.prop')

INFO_LIST = [
    ('system_build_id', SYSTEM_BUILD_PROP_DUMPER, 'ro.build.display.id'),
    ('system_sdk_ver', SYSTEM_BUILD_PROP_DUMPER, 'ro.build.version.sdk'),
    ('system_security_patch_level', SYSTEM_BUILD_PROP_DUMPER, 'ro.build.version.security_patch'),
    ('system_kernel_sepolicy_ver', SYSTEM_MATRIX_DUMPER, './sepolicy/kernel-sepolicy-version'),
    ('system_support_sepolicy_ver', SYSTEM_MATRIX_DUMPER, './sepolicy/sepolicy-version'),
    ('system_avb_ver', SYSTEM_MATRIX_DUMPER, './avb/vbmeta-version'),
    ('vendor_fingerprint', VENDOR_BUILD_PROP_DUMPER, 'ro.vendor.build.fingerprint'),
    ('vendor_zygote', VENDOR_DEFAULT_PROP_DUMPER, 'ro.zygote')] # pyformat: disable
