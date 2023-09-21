// Copyright 2023 The XLS Authors
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

#ifndef XLS_COMMON_CHECK_SIMULATOR_H_
#define XLS_COMMON_CHECK_SIMULATOR_H_

#include <string_view>

#include "absl/status/status.h"

namespace xls {

// Verify that the environment is appropriate for the specified simulator;
// returns an OK status if so. (May also print warnings if appropriate.)
absl::Status CheckSimulator(std::string_view simulator);

bool DefaultSimulatorSupportsSystemVerilog();

}  // namespace xls

#endif  // XLS_COMMON_CHECK_SIMULATOR_H_
