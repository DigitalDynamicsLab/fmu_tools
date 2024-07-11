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
// Example FMU instantiation for co-simulation (FMI 2.0 standard)
// Illustrates the FMU importing capabilities in fmu_tools (FmuToolsImport.h)
// =============================================================================

#include <iostream>
#include <fstream>
#include <cstddef>

#include "fmi3/FmuToolsImport.h"

using namespace fmi3;

std::string unzipped_fmu_folder = FMU_UNPACK_DIRECTORY;
////std::string unzipped_fmu_folder = FMU_MAIN_DIRECTORY;

int main(int argc, char* argv[]) {
    FmuUnit my_fmu;
    my_fmu.SetVerbose(true);

    try {
        my_fmu.Load(FmuType::COSIMULATION, FMU_FILENAME, FMU_UNPACK_DIRECTORY);
        ////my_fmu.Load(fmi3Type::fmi3CoSimulation, FMU_FILENAME);                 // unpack in  /tmp
        ////my_fmu.LoadUnzipped(fmi3Type::fmi3CoSimulation, unzipped_fmu_folder);  // already unpacked
    } catch (std::exception& my_exception) {
        std::cout << "ERROR loading FMU: " << my_exception.what() << "\n";
    }

    my_fmu.Instantiate("FmuComponent");  // use default resources dir
    ////my_fmu.Instantiate("FmuComponent", my_fmu.GetUnzippedFolder() + "resources");  // specify resources dir

    std::vector<std::string> categoriesVector = {"logAll"};
    my_fmu.SetDebugLogging(fmi3True, categoriesVector);

    // alternatively, with native interface:
    // std::vector<const char*> categoriesArray;
    // for (const auto& category : categoriesVector) {
    //    categoriesArray.push_back(category.c_str());
    //}

    // my_fmu._fmi3SetDebugLogging(my_fmu.component, fmi3True, categoriesVector.size(), categoriesArray.data());

    // Initialize FMU
    my_fmu.EnterInitializationMode(fmi3False,  // no tolerance defined
                                   0.0,        // tolerance (dummy)
                                   0.0,        // start time
                                   fmi3False,  // do not use stop time
                                   1.0         // stop time (dummy)
    );
    {
        fmi3ValueReference valref = 3;
        fmi3Float64 m_in = 1.5;
        my_fmu.SetVariable(valref, m_in, FmuVariable::Type::Float64);

        fmi3Float64 m_out;
        my_fmu.GetVariable(valref, m_out, FmuVariable::Type::Float64);
        std::cout << "m_out: " << m_out << std::endl;
    }
    my_fmu.ExitInitializationMode();

    // Prepare output file
    std::ofstream ofile("results.out");

    // Co-simulation loop
    double time = 0;
    double time_end = 10;
    double dt = 0.01;

    while (time < time_end) {
        double x, theta;
        my_fmu.GetVariable("x", x, FmuVariable::Type::Float64);
        my_fmu.GetVariable("theta", theta, FmuVariable::Type::Float64);
        ofile << time << " " << x << " " << theta << std::endl;

        // Set next communication time
        time += dt;

        // Advance FMU state to new time
        my_fmu.DoStep(time, dt, fmi3True);
    }

    // Close output file
    ofile.close();

    // Getting FMU variables through valueRef
    fmi3Float64 val_real;
    fmi3ValueReference valrefs_float64[11] = {0, 1, 2, 3, 5, 6, 7, 8, 9, 10, 11};
    for (auto valref : valrefs_float64) {
        my_fmu.GetVariable(valref, val_real, FmuVariable::Type::Float64);
        std::cout << "REAL: valref: " << valref << " | val: " << val_real << std::endl;
    }

    // Getting FMU variables through custom fmu-tools functions, directly using the variable name
    auto status = my_fmu.GetVariable("m", val_real, FmuVariable::Type::Float64);

    fmi3Boolean val_bool;

    fmi3ValueReference valrefs_bool[1] = {4};
    for (auto valref : valrefs_bool) {
        my_fmu.GetVariable(valref, val_bool, FmuVariable::Type::Boolean);
        std::cout << "BOOLEAN: valref: " << valref << " | val: " << val_bool << std::endl;
    }

    return 0;
}
