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
// Example FMU for co-simulation (FMI 3.0 standard)
// Illustrates the FMU exporting capabilities in fmu_tools (FmuToolsExport.h)
// =============================================================================

#pragma once

#include <vector>
#include <array>
#include <fstream>

// #define FMI3_FUNCTION_PREFIX MyModel_
#include "fmi3/FmuToolsExport.h"

class myFmuComponent : public fmi3::FmuComponentBase {
  public:
    myFmuComponent(FmuType fmiInterfaceType,
                   fmi3String instanceName,
                   fmi3String instantiationToken,
                   fmi3String resourcePath,
                   fmi3Boolean visible,
                   fmi3Boolean loggingOn,
                   fmi3InstanceEnvironment instanceEnvironment,
                   fmi3LogMessageCallback logMessage);

    ~myFmuComponent() {}

    virtual fmi3Status enterInitializationModeIMPL() override;

    virtual fmi3Status exitInitializationModeIMPL() override;

    virtual fmi3Status doStepIMPL(fmi3Float64 currentCommunicationPoint,
                                  fmi3Float64 communicationStepSize,
                                  fmi3Boolean noSetFMUStatePriorToCurrentPoint,
                                  fmi3Boolean* eventHandlingNeeded,
                                  fmi3Boolean* terminateSimulation,
                                  fmi3Boolean* earlyReturn,
                                  fmi3Float64* lastSuccessfulTime) override;

  private:
    virtual bool is_cosimulation_available() const override { return true; }
    virtual bool is_modelexchange_available() const override { return false; }

    // Problem-specific functions
    typedef std::array<double, 4> vec4;

    double calcX_dd(double theta, double theta_d);
    double calcTheta_dd(double theta, double theta_d);
    vec4 calcRHS(double t, const vec4& q);

    void calcAccelerations();

    // Problem parameters
    double len = 0.5;
    double m = 1.0;
    double M = 1.0;
    const double g = 9.81;

    fmi3Boolean approximateOn = fmi3False;
    std::string filename;

    // Problem states
    vec4 q;
    double x_dd;
    double theta_dd;
};
