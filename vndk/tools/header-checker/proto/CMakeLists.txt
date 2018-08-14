#
# Copyright (C) 2018 The Android Open Source Project
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

#------------------------------------------------------------------------------
# header-checker-proto
#------------------------------------------------------------------------------

#list(APPEND Protobuf_IMPORT_DIRS ".")

protobuf_generate_cpp(
    HEADER_CHECKER_PROTOBUF_SRCS HEADER_CHECKER_PROTOBUF_HEADERS
    "abi_dump.proto"
    "abi_diff.proto"
)

add_library(header-checker-proto STATIC
    "${HEADER_CHECKER_PROTOBUF_SRCS}"
)

target_compile_options(header-checker-proto
    PUBLIC "-DGOOGLE_PROTOBUF_NO_RTTI"
)

target_include_directories(header-checker-proto
    PUBLIC "${CMAKE_BINARY_DIR}/proto"
    PUBLIC "${Protobuf_INCLUDE_DIRS}"
)

target_link_libraries(header-checker-proto
    PUBLIC "${Protobuf_LIBRARIES}"
)
