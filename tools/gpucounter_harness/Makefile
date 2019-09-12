# Copyright (C) 2019 The Android Open Source Project
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


UNAME := $(shell uname)
ifeq ($(UNAME), Darwin)
NDK_HOST := darwin-x86_64
else
NDK_HOST := linux-x86_64
endif

CXX := $(NDK_HOME)/toolchains/llvm/prebuilt/$(NDK_HOST)/bin/clang++
LNK := $(CXX)

TEST_CFG = duration_ms: 10000; buffers { size_kb: 1024 }; data_sources { config { name: "gpu.counters" gpu_counter_config { counter_ids: 0 counter_ids: 1 } } }

CFLAGS += -std=c++11
CFLAGS += -fno-omit-frame-pointer
CFLAGS += -g
CFLAGS += -Wa,--noexecstack
CFLAGS += -fPIC
CFLAGS += -fno-exceptions
CFLAGS += -fno-rtti
CFLAGS += -fvisibility=hidden
CFLAGS += -Wno-everything
CFLAGS += -I$(LIBPROTOBUF_DIR)/src
CFLAGS += -I.
CFLAGS += -DPERFETTO_IMPLEMENTATION
CFLAGS += -DGOOGLE_PROTOBUF_NO_RTTI
CFLAGS += -DGOOGLE_PROTOBUF_NO_STATIC_INITIALIZER
CFLAGS += -DHAVE_PTHREAD=1

ANDROID_CFLAGS += --target=aarch64-linux-android
ANDROID_CFLAGS += --sysroot=$(NDK_HOME)/sysroot/usr/include
ANDROID_CFLAGS += -I$(NDK_HOME)/sources/android/support/include
ANDROID_CFLAGS += -I$(NDK_HOME)/sources/cxx-stl/llvm-libc++/include
ANDROID_CFLAGS += -I$(NDK_HOME)/sources/cxx-stl/llvm-libc++abi/include
ANDROID_CFLAGS += -isystem$(NDK_HOME)/sysroot/usr/include
ANDROID_CFLAGS += -isystem$(NDK_HOME)/sysroot/usr/include/aarch64-linux-android
ANDROID_CFLAGS += -DANDROID
ANDROID_CFLAGS += -D__ANDROID_API__=21
CFLAGS += $(ANDROID_CFLAGS)

ANDROID_LDFLAGS += -gcc-toolchain $(NDK_HOME)/toolchains/aarch64-linux-android-4.9/prebuilt/$(NDK_HOST)
ANDROID_LDFLAGS += --sysroot=$(NDK_HOME)/platforms/android-21/arch-arm64
ANDROID_LDFLAGS += --target=aarch64-linux-android
ANDROID_LDFLAGS += -Wl,--exclude-libs,libunwind.a
ANDROID_LDFLAGS += -Wl,--exclude-libs,libgcc.a
ANDROID_LDFLAGS += -Wl,--exclude-libs,libc++_static.a
ANDROID_LDFLAGS += -fuse-ld=gold
ANDROID_LDFLAGS += -Wl,--no-undefined
ANDROID_LDFLAGS += -Wl,-z,noexecstack
ANDROID_LDFLAGS += -Wl,-z,now
ANDROID_LDFLAGS += -Wl,--fatal-warnings
ANDROID_LDFLAGS += -pie
ANDROID_LDFLAGS += -L$(NDK_HOME)/sources/cxx-stl/llvm-libc++/libs/arm64-v8a
ANDROID_LDFLAGS += -lgcc -lc++_static -lc++abi -llog
LDFLAGS += $(ANDROID_LDFLAGS)

all: out/gpuCountersExecutable

clean:
	rm -rf out

outdir:
	@mkdir -p out

# Build object files
out/%.o: %.cc outdir
	@echo CXX $@
	@$(CXX) -o $@ -c $(CFLAGS) $<

# Link executable
out/gpuCountersExecutable: out/gpuCountersExecutable.o
	@echo LNK $@
	@$(LNK) $< $(LDFLAGS) -o $@

test: all
	adb root
	echo '$(TEST_CFG)' | adb shell perfetto --txt -c - -o /data/misc/perfetto-traces/trace.perfetto --background

.PHONY: clean all
