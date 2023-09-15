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


#include "FMU_tools.hpp"


// Use the namespace of Chrono

using namespace chrono;
using namespace chrono::irrlicht;

// Use the main namespaces of Irrlicht

using namespace irr;
using namespace irr::core;
using namespace irr::scene;
using namespace irr::video;
using namespace irr::io;
using namespace irr::gui;





int main(int argc, char* argv[]) {
    // Set path to Chrono data directory
    SetChronoDataPath(CHRONO_DATA_DIR);
    

    //
    // PARSE THE XML of fmu
    //

    FMU_unit my_fmu;

    try {
        //system("unzip");

        //my_fmu.Load("C:/Users/Alessandro/Desktop/test_fmu");
        //my_fmu.Load("C:/Users/Alessandro/Desktop/test_fmu2");
        //my_fmu.Load("C:/workspace/vehicle_modelica/fullvehicle3D_Pacejka_fmu/fmu_dynamic/fullvehicle3D_Pacejka");
        //my_fmu.Load("C:/workspace/vehicle_modelica/scripts/fullvehicle3D_Pacejka/fmu/fullvehicle3D_Pacejka");
        my_fmu.Load("C:/workspace/vehicle_modelica/scripts/fullvehicle3D_notires/fmu/fullvehicle3D_notires");

   

        my_fmu.LoadXML();
        my_fmu.LoadDLL();
        my_fmu.BuildVariablesTree();
        my_fmu.BuildVisualizersList(&my_fmu.tree_variables);

    }catch (exception& my_exception) {

        GetLog() << "ERROR loading FMU: " << my_exception.what() << "\n";
    }

    GetLog() << "FMU version:  " << my_fmu._fmi2GetVersion() << "\n";
    GetLog() << "FMU platform: " << my_fmu._fmi2GetTypesPlatform() << "\n";


    my_fmu.Instantiate("my_tool");

    double start_time = 0;
    double stop_time = 2;
    my_fmu._fmi2SetupExperiment(my_fmu.component, 
        fmi2False, // tolerance defined
        0.0,       // tolerance 
        start_time, 
        fmi2False,  // use stop time
        stop_time); 

    my_fmu._fmi2EnterInitializationMode(my_fmu.component);

     // play a bit with set/get:
     fmi2String m_str;
     unsigned int sref = 1;
     my_fmu._fmi2GetString(my_fmu.component, &sref, 1, &m_str);
     GetLog() << "FMU variable 1 has value: "   << m_str << "\n";

    my_fmu._fmi2ExitInitializationMode(my_fmu.component);

    
    // test a simulation loop:
    double time = 0;
    double dt = 0.001;
    /*
    for (int i= 0; i< 1000; ++i) {

        my_fmu._fmi2DoStep(my_fmu.component, time, dt, fmi2True); 

        time +=dt;
    }
    */

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

    GetLog() << "\n\n\n";

    // Create a Chrono physical system
    ChSystemNSC mphysicalSystem;

    // Create the Irrlicht visualization (open the Irrlicht device,
    // bind a simple user interface, etc. etc.)
    ChIrrApp application(&mphysicalSystem, L"A simple project template", core::dimension2d<u32>(800, 600),
                         false);  // screen dimensions

    // Easy shortcuts to add camera, lights, logo and sky in Irrlicht scene:
    application.AddTypicalLogo();
    application.AddTypicalSky();
    application.AddTypicalLights();
    application.AddTypicalCamera(core::vector3df(2, 2, -5),
                                 core::vector3df(0, 1, 0));  // to change the position of camera

    // application.AddLightWithShadow(vector3df(1,25,-5), vector3df(0,0,0), 35, 0.2,35, 55, 512, video::SColorf(1,1,1));

    //======================================================================

    // HERE YOU CAN POPULATE THE PHYSICAL SYSTEM WITH BODIES AND LINKS.
    //
    // An example: a pendulum.

 

    //======================================================================

    // Use this function for adding a ChIrrNodeAsset to all items
    // Otherwise use application.AssetBind(myitem); on a per-item basis.
    application.AssetBindAll();

    // Use this function for 'converting' assets into Irrlicht meshes
    application.AssetUpdateAll();

    // Adjust some settings:
    application.SetTimestep(dt);
    application.SetTryRealtime(true);

    //
    // THE SOFT-REAL-TIME CYCLE
    //

    auto shapes_root = application.GetSceneManager()->addEmptySceneNode();
    my_fmu.FMU_visualizers_create(shapes_root, application.GetSceneManager());

#if 1
    while (application.GetDevice()->run()) {
        application.BeginScene();

        application.DrawAll();

        my_fmu._fmi2DoStep(my_fmu.component, time, dt, fmi2True);


        time += dt;
        my_fmu.FMU_visualizers_update(shapes_root, application.GetSceneManager());

        application.EndScene();
    }
#else

    ChTimer<> timer;
    timer.start();
    int i = 0;
    for (i = 0; i < 10000; ++i) {

        my_fmu._fmi2DoStep(my_fmu.component, time, dt, fmi2True);
        time += dt;

    }
    timer.stop();
    std::cout << "Elapsed time: " << timer() << "s; Simulated time: " << i*dt << "s (" << i << " steps; dt: " << dt << ")" << std::endl;
#endif

    return 0;
}
