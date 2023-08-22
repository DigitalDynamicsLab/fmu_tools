// =============================================================================
// PROJECT CHRONO - http://projectchrono.org
//
// Copyright (c) 2014 projectchrono.org
// All rights reserved.
//
// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file at the top level of the distribution and at
// http://projectchrono.org/license-chrono.txt.
//
// =============================================================================
// A very simple example that can be used as template project for
// a Chrono::Engine simulator with 3D view.
// =============================================================================


#include "FmuToolsStandalone.hpp"
#include "fmi2_headers/fmi2Functions.h"

#include <iostream>
#define _USE_MATH_DEFINES
#include <math.h>

#include <ostream>

void logger(fmi2ComponentEnvironment c, fmi2String instanceName, fmi2Status status, fmi2String category, fmi2String message, ...)
{
    if (!instanceName) instanceName = "?";
    if (!category) category = "?";
    std::cout << "[" << instanceName << " | " << status << "] " << category << ": " << message;
}



// Remember to set the Working Directory to $(OutDir)
std::string unzipped_fmu_folder = ".";
int main(int argc, char* argv[]) {
    
    std::cout << "nonfmu_host\n";
    double time = 0;
    double t_end = 10;
    double stepsize = 1e-2;

    fmi2CallbackFunctions callbackFunctions = {
        logger,
        calloc,
        free,
        nullptr,
        nullptr
    };


    fmi2Component component = fmi2Instantiate("fmu_entity", fmi2Type::fmi2CoSimulation, "GUID", "", &callbackFunctions, fmi2False, fmi2False);
    fmi2Status fmi2SetupExperiment_status = fmi2SetupExperiment(component, fmi2False, 1e-6, time, fmi2False, t_end);
    fmi2EnterInitializationMode(component);

    // state
    const size_t nvr_state = 4;
    fmi2ValueReference state_vr[nvr_state] = {1, 2, 4, 5};
    fmi2Real state[nvr_state] = {0, 0, 0, M_PI/4};

    fmi2Status fmi2SetReal_state_status = fmi2SetReal(component, state_vr, nvr_state, state);


    // parameters
    const size_t nvr_param = 3;
    fmi2ValueReference param_vr[nvr_param] = {6, 7, 8};
    fmi2Real params[nvr_param] = {0.5, 1.0, 1.0};
    fmi2Status fmi2SetReal_params_status = fmi2SetReal(component, param_vr, nvr_param, params);

    fmi2String categories[4] = {"logStatusFatal","logStatusError","logStatusWarning","logAll"};
    fmi2Status fmi2SetDebugLogging_status = fmi2SetDebugLogging(component, fmi2True, 0, categories);

    const size_t nvr_bool = 1;
    fmi2ValueReference bool_vr[nvr_bool] = {0};
    fmi2Boolean params_bool[nvr_bool] = {fmi2True};
    fmi2Status fmi2SetBoolean_status = fmi2SetBoolean(component, bool_vr, nvr_bool, params_bool);

    fmi2ExitInitializationMode(component);



    fmi2Status fmi2DoStep_status;
    fmi2Status fmi2GetReal_status;
    fmi2Real state_temp[nvr_state];

    std::ofstream output_file("output.csv");

    while (time < t_end){
        fmi2DoStep_status = fmi2DoStep(component, time, stepsize, fmi2True);
        if (fmi2DoStep_status!=fmi2Status::fmi2OK){
            std::cout << "fmi2DoStep went wrong\n"; 
            return -1;
        }

        fmi2GetReal_status = fmi2GetReal(component, state_vr, nvr_state, state_temp);

        time += stepsize;
        //std::cout << time << ": [" <<
        //            state_temp[0] << ", " <<
        //            state_temp[1] << ", " <<
        //            state_temp[2] << ", " <<
        //            state_temp[3] << "]" << std::endl;

        output_file << time << ", " <<
                    state_temp[0] << ", " <<
                    state_temp[1] << ", " <<
                    state_temp[2] << ", " <<
                    state_temp[3] << std::endl;

    }
    output_file.close();


    fmi2FreeInstance(component);

    return 0;
}
