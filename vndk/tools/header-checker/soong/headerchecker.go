//
// Copyright (C) 2018 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

package headerchecker

import (
	"github.com/google/blueprint/proptools"

	"android/soong/android"
	"android/soong/cc"
)

func init() {
	android.RegisterModuleType("header_checker_defaults",
		headerCheckerDefaultsFactory)
}

func headerCheckerDefaults(ctx android.LoadHookContext) {
	type props struct {
		Enabled *bool
	}

	p := &props{}
	p.Enabled = proptools.BoolPtr(
		ctx.AConfig().IsEnvTrue("LLVM_BUILD_HOST_TOOLS"))
	ctx.AppendProperties(p)
}

func headerCheckerDefaultsFactory() android.Module {
	module := cc.DefaultsFactory()
	android.AddLoadHook(module, headerCheckerDefaults)
	return module
}
