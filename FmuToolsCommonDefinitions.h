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
// Definitions independent of FMI version
// =============================================================================

#ifndef FMUTOOLS_COMMON_DEFINITIONS_H
#define FMUTOOLS_COMMON_DEFINITIONS_H

/// Enumeration of supported FMU types (interfaces).
enum class FmuType {
    MODEL_EXCHANGE,      ///< FMU for model exchange
    COSIMULATION,        ///< FMU for co-simulation
    SCHEDULED_EXECUTION  ///< FMU for co-simulation (only FMI3)
};

#endif
