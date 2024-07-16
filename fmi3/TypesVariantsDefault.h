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

/// List of pointers to all (unique) types used to define VariableTypes.
/// No duplicated entries are allowed.
/// In general it does not coincide with the list VariableTypes defined in fmi3PlatformTypes since usually more
/// VariableTypes refer to the same underlying concrete type.
/// Additionally, the list also includes pair of setter and getter functions in order to allow setting|getting values from objects not
/// having an actual address in memory.
/// The types listed here will then be match with any compatible VariableType during the call to FmuComponentBase::AddFmuVariable().
/// 
//// TODO: DARIOM double check if it setter function for big objects can pass by (const?) reference instead of value
using FmuVariableBindType =
    varns::variant<float*,
                   double*,
                   int8_t*,
                   uint8_t*,
                   int16_t*,
                   uint16_t*,
                   int32_t*,
                   uint32_t*,
                   int64_t*,
                   uint64_t*,
                   bool*,
                   char*,
                   std::string*,
                   std::pair<std::function<float()>, std::function<void(float)>>,
                   std::pair<std::function<double()>, std::function<void(double)>>,
                   std::pair<std::function<int8_t()>, std::function<void(int8_t)>>,
                   std::pair<std::function<uint8_t()>, std::function<void(uint8_t)>>,
                   std::pair<std::function<int16_t()>, std::function<void(int16_t)>>,
                   std::pair<std::function<uint16_t()>, std::function<void(uint16_t)>>,
                   std::pair<std::function<int32_t()>, std::function<void(int32_t)>>,
                   std::pair<std::function<uint32_t()>, std::function<void(uint32_t)>>,
                   std::pair<std::function<int64_t()>, std::function<void(int64_t)>>,
                   std::pair<std::function<uint64_t()>, std::function<void(uint64_t)>>,
                   std::pair<std::function<bool()>, std::function<void(bool)>>,
                   std::pair<std::function<char()>, std::function<void(char)>>,
                   std::pair<std::function<std::string()>, std::function<void(std::string)>>>;

///// List of (unique) types required to 
//using FmuVariableStartType = varns::variant<float,
//                                            double,
//                                            int8_t,
//                                            uint8_t,
//                                            int16_t,
//                                            uint16_t,
//                                            int32_t,
//                                            uint32_t,
//                                            int64_t,
//                                            uint64_t,
//                                            bool,
//                                            char,
//                                            std::string,
//                                            std::vector<float>,
//                                            std::vector<double>,
//                                            std::vector<int8_t>,
//                                            std::vector<uint8_t>,
//                                            std::vector<int16_t>,
//                                            std::vector<uint16_t>,
//                                            std::vector<int32_t>,
//                                            std::vector<uint32_t>,
//                                            std::vector<int64_t>,
//                                            std::vector<uint64_t>,
//                                            std::vector<bool>,
//                                            std::vector<char>
//>;

}  // namespace fmi3

#endif
