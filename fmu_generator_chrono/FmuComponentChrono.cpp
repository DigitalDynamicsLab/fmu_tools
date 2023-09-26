
//#define FMI2_FUNCTION_PREFIX MyModel_
#include "FmuComponentChrono.h"
#include <cassert>
#include <vector>
#include <array>
#include <map>
#include <algorithm>
#include <chrono/core/ChVector.h>
#include "FmuToolsChrono.h"

FmuComponent::FmuComponent(fmi2String _instanceName, fmi2Type _fmuType, fmi2String _fmuGUID):
        FmuComponentBase(_instanceName, _fmuType, _fmuGUID)
{

    SetChronoDataPath(CHRONO_DATA_DIR);



    /// FMU_ACTION: declare relevant variables
    addFmuVariable(&x_tt,            "x_tt",     FmuVariable::Type::FMU_REAL, "m/s2",   "cart acceleration");
    addFmuVariable(&x_t,             "x_t",      FmuVariable::Type::FMU_REAL, "m/s",    "cart velocity");
    addFmuVariable(&x,               "x",        FmuVariable::Type::FMU_REAL, "m",      "cart position");
    addFmuVariable(&theta_tt,        "theta_tt", FmuVariable::Type::FMU_REAL, "rad/s2", "pendulum ang acceleration");
    addFmuVariable(&theta_t,         "theta_t",  FmuVariable::Type::FMU_REAL, "rad/s",  "pendulum ang velocity");
    addFmuVariable(&theta,           "theta",    FmuVariable::Type::FMU_REAL, "rad",    "pendulum angle");
    addFmuVariable(&pendulum_length, "pendulum_length", FmuVariable::Type::FMU_REAL, "m",  "pendulum length", FmuVariable::CausalityType::parameter, FmuVariable::VariabilityType::fixed);
    addFmuVariable(&cart_mass,       "cart_mass",   FmuVariable::Type::FMU_REAL, "kg", "pendulum mass", FmuVariable::CausalityType::parameter, FmuVariable::VariabilityType::fixed);
    addFmuVariable(&pendulum_mass,   "pendulum_mass",   FmuVariable::Type::FMU_REAL, "kg", "cart mass", FmuVariable::CausalityType::parameter, FmuVariable::VariabilityType::fixed);

};

void FmuComponent::_enterInitializationMode() {
    sys.Clear();
};

void FmuComponent::_exitInitializationMode() {
    auto ground = chrono_types::make_shared<ChBody>();
    sys.Add(ground);

    auto cart = chrono_types::make_shared<ChBodyEasyBox>(0.2, 0.1, 0.1, 750, true, false);
    cart->SetIdentifier(10);
    cart->SetMass(cart_mass);
    sys.Add(cart);

    auto pendulum = chrono_types::make_shared<ChBodyEasyBox>(0.025, pendulum_length, 0.01, 750, true, false);
    pendulum->SetMass(pendulum_mass);
    pendulum->SetInertiaXX(ChVector<>(0.01, 0.01, 0.01));
    sys.Add(pendulum);

    auto cart_prism = chrono_types::make_shared<ChLinkLockPrismatic>();
    cart_prism->Initialize(cart, ground, ChCoordsys<>(VNULL, Q_from_AngY(90*CH_C_DEG_TO_RAD)));
    cart_prism->SetName("cart_prism");
    sys.Add(cart_prism);

    auto pendulum_rev = chrono_types::make_shared<ChLinkRevolute>();
    pendulum_rev->Initialize(pendulum, ground, ChFrame<>(VNULL, QUNIT));
    pendulum_rev->SetName("pendulum_rev");
    sys.Add(pendulum_rev);

    sys.DoFullAssembly();

    updateVarsCallbacks.push_back([this](){ x_tt = this->sys.SearchBodyID(10)->GetPos_dtdt().x(); });
    updateVarsCallbacks.push_back([this](){ x_t = this->sys.SearchBodyID(10)->GetPos_dt().x(); });
    updateVarsCallbacks.push_back([this](){ x = this->sys.SearchBodyID(10)->GetPos().x(); });

    //updateVarsCallbacks.push_back([this](){
    //    ChCoordsys<> rel_ref = std::dynamic_pointer_cast<ChLinkRevolute>(this->sys.SearchLink("cart_prism"))->GetLinkRelativeCoords();
    //    this->theta = std::atan2(rel_ref.rot.GetXaxis() ^ ChVector<>(0,1,0), rel_ref.rot.GetXaxis() ^ ChVector<>(1,0,0));
    //    });
};

fmi2Status FmuComponent::_doStep(fmi2Real currentCommunicationPoint, fmi2Real communicationStepSize, fmi2Boolean noSetFMUStatePriorToCurrentPoint) {

    while (time < currentCommunicationPoint + communicationStepSize){
        fmi2Real _stepSize = std::min((currentCommunicationPoint + communicationStepSize - time), std::min(communicationStepSize, stepSize));

        sys.DoStepDynamics(_stepSize);
        sendToLog("Step at time: " + std::to_string(time) + " with timestep: " + std::to_string(_stepSize) + "ms succeeded.\n", fmi2Status::fmi2OK, "logAll");
        updateVars();

        time = time + _stepSize;


    }
    
    return fmi2Status::fmi2OK;
}



