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

#include <iostream>
#include <chrono>

//std::string unzipped_fmu_folder = "D:/workspace/DigitalDynamicsLab/fmu_generator_standalone_build";
std::string unzipped_fmu_folder = FMU_MAIN_DIRECTORY;
int main(int argc, char* argv[]) {
    

    //
    // PARSE THE XML of fmu
    //

    FmuUnit my_fmu;

    try {
        my_fmu.Load(unzipped_fmu_folder);

   

        my_fmu.LoadXML();
        my_fmu.LoadDLL();
        my_fmu.BuildVariablesTree();
        my_fmu.BuildVisualizersList(&my_fmu.tree_variables);

    }catch (std::exception& my_exception) {

        std::cout << "ERROR loading FMU: " << my_exception.what() << "\n";
    }

    std::cout << "FMU version:  " << my_fmu._fmi2GetVersion() << "\n";
    std::cout << "FMU platform: " << my_fmu._fmi2GetTypesPlatform() << "\n";

    my_fmu.Instantiate("fmu_instance");
    std::vector<std::string> categoriesVector = {"logAll"};

    std::vector<const char*> categoriesArray;
    for (const auto& category : categoriesVector) {
        categoriesArray.push_back(category.c_str());
    }

    my_fmu._fmi2SetDebugLogging(my_fmu.component, fmi2True, categoriesVector.size(), categoriesArray.data());

    double start_time = 0;
    double stop_time = 2;
    my_fmu._fmi2SetupExperiment(my_fmu.component, 
        fmi2False, // tolerance defined
        0.0,       // tolerance 
        start_time, 
        fmi2False,  // use stop time
        stop_time); 

    my_fmu._fmi2EnterInitializationMode(my_fmu.component);

     //// play a bit with set/get:
     //fmi2String m_str;
     //unsigned int sref = 1;
     //my_fmu._fmi2GetString(my_fmu.component, &sref, 1, &m_str);
     //std::cout << "FMU variable 1 has value: "   << m_str << "\n";

    my_fmu._fmi2ExitInitializationMode(my_fmu.component);

    
    // test a simulation loop:
    double time = 0;
    double dt = 0.001;
    
    std::chrono::high_resolution_clock::time_point startTime, endTime;

    std::chrono::duration<double> duration_average;
    for (int i = 0; i<1000; ++i) {

        my_fmu._fmi2DoStep(my_fmu.component, time, dt, fmi2True);

        fmi2ValueReference valref[1] = {1};
        fmi2Real val;
        startTime = std::chrono::high_resolution_clock::now();
        for (int j = 0; j<100000; ++j) {
            my_fmu._fmi2GetReal(my_fmu.component, valref, 1, &val); 
        }
        endTime = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = std::chrono::duration_cast<std::chrono::duration<double>>(endTime - startTime);
        duration_average += duration;
        std::cout << "Time taken: " << duration.count() << " seconds." << std::endl;



        time +=dt;
    }

    duration_average /= 1000;
    std::cout << "Time taken average: " << duration_average.count() << " seconds." << std::endl;

    

    // Just some dumps for checking:
    
    /*
    //my_fmu.DumpTree(&my_fmu.tree_variables,0);  // dump all tree
    my_fmu.DumpTree(&my_fmu.tree_variables.children["body1"],0);  // dump only one subtree
    */

    /*
    for (auto& i : my_fmu.visualizers) {
        std::cout   << "Visualizer: \n"
                    << "   r1  = " << i.pos_references[0] << "\n"
                    << "   r2  = " << i.pos_references[1] << "\n"
                    << "   r3  = " << i.pos_references[2] << "\n";
    }
    */

    /*
    for (auto i : my_fmu.flat_variables) {
        std::cout   << "Variable: \n"
                    << "   name  = " << i.second.name << "\n"
                    << "   ref.  = " << i.second.valueReference << "\n";
                   // << "   desc. = " << i.second.description << "\n"
                   // << "   init. = " << i.second.initial << "\n"
                   // << "   caus. = " << i.second.causality << "\n"
                   // << "   varb. = " << i.second.variability << "\n";
    }
    */


    //======================================================================

    std::cout << "\n\n\n";


    return 0;
}
