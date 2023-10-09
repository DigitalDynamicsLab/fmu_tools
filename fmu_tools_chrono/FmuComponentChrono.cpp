
//#define FMI2_FUNCTION_PREFIX MyModel_
#include "FmuComponentChrono.h"
#include <cassert>
#include <vector>
#include <array>
#include <map>
#include <algorithm>
#include <chrono/core/ChVector.h>
#include <chrono/serialization/ChArchive.h>
#include "FmuToolsChrono.h"

FmuComponent::FmuComponent(fmi2String _instanceName, fmi2Type _fmuType, fmi2String _fmuGUID):
        FmuComponentBase(_instanceName, _fmuType, _fmuGUID)
{
    initializeType(_fmuType);

    SetChronoDataPath(CHRONO_DATA_DIR);

    /// FMU_ACTION: declare relevant variables
    AddFmuVariable(&x_tt,            "x_tt",            FmuVariable::Type::Real, "m/s2",   "cart acceleration");
    AddFmuVariable(&x_t,             "x_t",             FmuVariable::Type::Real, "m/s",    "cart velocity");
    AddFmuVariable(&x,               "x",               FmuVariable::Type::Real, "m",      "cart position");
    AddFmuVariable(&theta_tt,        "theta_tt",        FmuVariable::Type::Real, "rad/s2", "pendulum ang acceleration");
    AddFmuVariable(&theta_t,         "theta_t",         FmuVariable::Type::Real, "rad/s",  "pendulum ang velocity");
    AddFmuVariable(&theta,           "theta",           FmuVariable::Type::Real, "rad",    "pendulum angle");
    AddFmuVariable(&pendulum_length, "pendulum_length", FmuVariable::Type::Real, "m",      "pendulum length", FmuVariable::CausalityType::parameter, FmuVariable::VariabilityType::fixed);
    AddFmuVariable(&cart_mass,       "cart_mass",       FmuVariable::Type::Real, "kg",     "pendulum mass", FmuVariable::CausalityType::parameter, FmuVariable::VariabilityType::fixed);
    AddFmuVariable(&pendulum_mass,   "pendulum_mass",   FmuVariable::Type::Real, "kg",     "cart mass", FmuVariable::CausalityType::parameter, FmuVariable::VariabilityType::fixed);

    auto ground = chrono_types::make_shared<ChBody>();
    ground->SetBodyFixed(true);
    sys.Add(ground);

    // Cart is moving along X axis, Pendulum rotates along Z axis

    auto cart = chrono_types::make_shared<ChBodyEasyBox>(0.2, 0.1, 0.1, 750, true, false);
    cart->SetIdentifier(10);
    cart->SetMass(cart_mass);
    sys.Add(cart);

    auto pendulum = chrono_types::make_shared<ChBodyEasyBox>(0.025, pendulum_length, 0.01, 750, true, false);
    pendulum->SetMass(pendulum_mass);
    pendulum->SetInertiaXX(ChVector<>(0.01, 0.01, 0.01));
    sys.Add(pendulum);

    auto cart_prism = chrono_types::make_shared<ChLinkLockPrismatic>();
    cart_prism->Initialize(cart, ground, ChCoordsys<>(VNULL, Q_ROTATE_Z_TO_X));
    cart_prism->SetName("cart_prism");
    sys.Add(cart_prism);

    auto pendulum_rev = chrono_types::make_shared<ChLinkRevolute>();
    pendulum_rev->Initialize(pendulum, cart, true, ChFrame<>(VNULL, QUNIT), ChFrame<>(ChVector<>(0, +pendulum_length/2, 0), QUNIT));
    pendulum_rev->SetName("pendulum_rev");
    sys.Add(pendulum_rev);

    sys.DoFullAssembly();

    vis = chrono_types::make_shared<irrlicht::ChVisualSystemIrrlicht>();
    vis->AttachSystem(&sys);
    vis->SetWindowSize(800, 600);
    vis->SetWindowTitle("CartPendulumFMU");
    vis->Initialize();
    vis->AddLogo();
    vis->AddSkyBox();
    vis->AddCamera(ChVector<>(-0.5, -0.5, -1.0));
    vis->AddTypicalLights();

    updateVarsCallbacks.push_back([this](){ x_tt = this->sys.SearchBodyID(10)->GetPos_dtdt().x(); });
    updateVarsCallbacks.push_back([this](){ x_t = this->sys.SearchBodyID(10)->GetPos_dt().x(); });
    updateVarsCallbacks.push_back([this](){ x = this->sys.SearchBodyID(10)->GetPos().x(); });
};

void FmuComponent::_preModelDescriptionExport() {
    _exitInitializationMode();
    ChArchiveFmu archive_fmu(*this);
    archive_fmu << CHNVP(sys);
}

void FmuComponent::_postModelDescriptionExport() {

}


void FmuComponent::_exitInitializationMode() {

    sys.DoFullAssembly();
};

fmi2Status FmuComponent::_doStep(fmi2Real currentCommunicationPoint, fmi2Real communicationStepSize, fmi2Boolean noSetFMUStatePriorToCurrentPoint) {

    while (time < currentCommunicationPoint + communicationStepSize){
        fmi2Real _stepSize = std::min((currentCommunicationPoint + communicationStepSize - time), std::min(communicationStepSize, stepSize));

        if (vis)
        {        
            vis->BeginScene();
            vis->Render();
        }

        sys.DoStepDynamics(_stepSize);
        sendToLog("Step at time: " + std::to_string(time) + " with timestep: " + std::to_string(_stepSize) + "ms succeeded.\n", fmi2Status::fmi2OK, "logAll");
        updateVars();

        time = time + _stepSize;

        realtime_timer.Spin(_stepSize);

        
        if (vis)
        {        
            vis->EndScene();
        }




    }
    
    return fmi2Status::fmi2OK;
}



