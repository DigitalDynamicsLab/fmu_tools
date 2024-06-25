// =============================================================================
// fmu_tools
//
// Copyright (c) 2024 Project Chrono (projectchrono.org)
// Copyright (c) 2024 Digital Dynamics Lab, University of Parma, Italy
// Copyright (c) 2024 Simulation Based Engineering Lab, University of Wisconsin-Madison, USA
// All rights reserved.
//
// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file at the top level of the distribution.
//
// =============================================================================
// Definitions for the types platform (FMI 3.0)
// =============================================================================

#ifndef FMUTOLS_FMI3_TYPESVARIANTS_H
#define FMUTOLS_FMI3_TYPESVARIANTS_H

#include <string>

#include "variant/variant_guard.hpp"
#include "fmi3/fmi3_headers/fmi3PlatformTypes.h"

namespace fmi3 {

#define FMITYPESPLATFORM_DEFAULT

// Note: variants do not allow repeated types.
// fmi3Byte == fmi3UInt8
// fmi3Clock == fmi3Boolean
using FmuVariableBindType = varns::variant<fmi3Float32*,
                                           fmi3Float64*,
                                           fmi3Int8*,
                                           fmi3UInt8*,
                                           fmi3Int16*,
                                           fmi3UInt16*,
                                           fmi3Int32*,
                                           fmi3UInt32*,
                                           fmi3Int64*,
                                           fmi3UInt64*,
                                           fmi3Boolean*,
                                           fmi3Char*,
                                           fmi3Binary*,
                                           std::string*,
                                           std::pair<std::function<fmi3Float64()>, std::function<void(fmi3Float64)>>,
                                           std::pair<std::function<fmi3Int32()>, std::function<void(fmi3Int32)>>,
                                           std::pair<std::function<fmi3UInt32()>, std::function<void(fmi3UInt32)>>,
                                           std::pair<std::function<std::string()>, std::function<void(std::string)>>>;

using FmuVariableStartType = varns::variant<fmi3Float32,
                                            fmi3Float64,
                                            fmi3Int8,
                                            fmi3UInt8,
                                            fmi3Int16,
                                            fmi3UInt16,
                                            fmi3Int32,
                                            fmi3UInt32,
                                            fmi3Int64,
                                            fmi3UInt64,
                                            fmi3Boolean,
                                            fmi3Char,
                                            fmi3Binary,
                                            std::string>;

}  // namespace fmi3

#endif
