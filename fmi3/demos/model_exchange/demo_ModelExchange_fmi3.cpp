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
// Example FMU instantiation for model exchange (FMI 3.0 standard)
// Illustrates the FMU importing capabilities in fmu_tools (FmuToolsImport.h)
// =============================================================================

#include <iostream>
#include <fstream>
#include <cstddef>

#include "fmi3/FmuToolsImport.h"

using namespace fmu_tools::fmi3;

std::string unzipped_fmu_folder = FMU_UNPACK_DIRECTORY;

int main(int argc, char* argv[]) {
    // Load FMU
    FmuUnit my_fmu;
    my_fmu.SetVerbose(true);

    try {
        my_fmu.Load(FmuType::MODEL_EXCHANGE, FMU_FILENAME, FMU_UNPACK_DIRECTORY);
        ////my_fmu.Load(FmuType::MODEL_EXCHANGE, FMU_FILENAME);                 // unpack in  /tmp
        ////my_fmu.LoadUnzipped(FmuType::MODEL_EXCHANGE, unzipped_fmu_folder);  // already unpacked
    } catch (std::exception& my_exception) {
        std::cout << "ERROR loading FMU: " << my_exception.what() << std::endl;
        return 1;
    }

    try {
        my_fmu.Instantiate("FmuComponent");  // use default resources dir
        ////my_fmu.Instantiate("FmuComponent", my_fmu.GetUnzippedFolder() + "resources");  // specify resources dir
    } catch (std::exception& my_exception) {
        std::cout << "ERROR instantiating FMU: " << my_exception.what() << std::endl;
        return 1;
    }

    std::vector<std::string> categoriesVector = {"logAll"};
    my_fmu.SetDebugLogging(fmi3True, categoriesVector);

    // Get number of states from the ModelExchange FMU
    int num_states = my_fmu.GetNumStates();
    std::cout << "Num states: " << num_states << std::endl;

    // Initialize FMU
    my_fmu.EnterInitializationMode(fmi3False,  // no tolerance defined
                                   0.0,        // tolerance (dummy)
                                   0.0,        // start time
                                   fmi3False,  // do not use stop time
                                   1.0         // stop time (dummy)
    );
    my_fmu.ExitInitializationMode();

    // States and derivatives
    std::vector<fmi3Float64> states(num_states);
    std::vector<fmi3Float64> derivs(num_states);

    // Get initial conditions (states)
    my_fmu.GetContinuousStates(states.data(), num_states);

    // Prepare output file
    std::ofstream ofile("results.out");

    // Simulation loop
    double time = 0;
    double time_end = 10;
    double dt = 0.001;

    while (time < time_end) {
        double x, theta;
        my_fmu.GetVariable("x", x);
        my_fmu.GetVariable("theta", theta);
        ofile << time << " " << x << " " << theta << std::endl;

        // Compute derivatives (RHS)
        my_fmu.GetContinuousStateDerivatives(derivs.data(), num_states);

        // Advance time
        time += dt;
        my_fmu.SetTime(time);

        // Perform integration (forward Euler) and set state
        for (int i = 0; i < num_states; i++)
            states[i] += derivs[i] * dt;
        my_fmu.SetContinuousStates(states.data(), num_states);
    }

    // Close output file
    ofile.close();

    return 0;
}
