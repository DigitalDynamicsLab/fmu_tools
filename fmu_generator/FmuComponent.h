
#pragma once
//#define FMI2_FUNCTION_PREFIX MyModel_
#include "FmuToolsExport.h"
#include <vector>
#include <array>



class FmuComponent: public FmuComponentBase{
public:
    FmuComponent(fmi2String _instanceName, fmi2Type _fmuType, fmi2String _fmuGUID):
        FmuComponentBase(_instanceName, _fmuType, _fmuGUID)
    {

        initializeType(_fmuType);

        /// FMU_ACTION: define new units if needed
        //UnitDefinitionType UD_rad_s4 ("rad/s4"); UD_rad_s4.s = -4; UD_rad_s4.rad = 1;
        //addUnitDefinition(UD_rad_s4);

        /// FMU_ACTION: declare relevant variables
        addFmuVariable(&q_t[0], "x_tt",     FmuVariable::Type::FMU_REAL, "m/s2",   "cart acceleration");
        addFmuVariable(&q[0],   "x_t",      FmuVariable::Type::FMU_REAL, "m/s",    "cart velocity");
        addFmuVariable(&q[1],   "x",        FmuVariable::Type::FMU_REAL, "m",      "cart position");
        addFmuVariable(&q_t[2], "theta_tt", FmuVariable::Type::FMU_REAL, "rad/s2", "pendulum ang acceleration");
        addFmuVariable(&q[2],   "theta_t",  FmuVariable::Type::FMU_REAL, "rad/s",  "pendulum ang velocity");
        addFmuVariable(&q[3],   "theta",    FmuVariable::Type::FMU_REAL, "rad",    "pendulum angle");
        auto fmu_len = addFmuVariable(&len,  "len", FmuVariable::Type::FMU_REAL, "m",  "pendulum length", FmuVariable::CausalityType::parameter, FmuVariable::VariabilityType::fixed);
        auto fmu_m =   addFmuVariable(&m,    "m",   FmuVariable::Type::FMU_REAL, "kg", "pendulum mass",   FmuVariable::CausalityType::parameter, FmuVariable::VariabilityType::fixed);
        auto fmu_M =   addFmuVariable(&M,    "M",   FmuVariable::Type::FMU_REAL, "kg", "cart mass",       FmuVariable::CausalityType::parameter, FmuVariable::VariabilityType::fixed);

        // FMU_ACTION: start value will be automatically grabbed from 'len' during addFmuVariable;
        // use the following statement only if the start value is not already in 'len' when called addFmuVariable
        //fmu_len.SetStartVal(len);


        auto fmu_approximateOn = addFmuVariable(&approximateOn, "approximateOn", FmuVariable::Type::FMU_BOOLEAN, "1", "use approximated model", FmuVariable::CausalityType::parameter, FmuVariable::VariabilityType::fixed);

        // Additional commands
        q = {0, 0, 0, 3.14159265358979323846/4};
    }

    virtual void EnterInitializationMode() override {}

    virtual void ExitInitializationMode() override {}

    virtual ~FmuComponent(){}

    /// FMU_ACTION: override DoStep of the base class with the problem-specific implementation
    virtual fmi2Status DoStep(fmi2Real currentCommunicationPoint, fmi2Real communicationStepSize, fmi2Boolean noSetFMUStatePriorToCurrentPoint) override;


    // Problem-specific functions
    double get_x_tt(double th_t, double th);
    double get_th_tt(double th_t, double th);
    double get_x_tt_linear(double th_t, double th);
    double get_th_tt_linear(double th_t, double th);
    void get_q_t(const std::array<double, 4>& _q, std::array<double, 4>& q_t);


protected:

    virtual bool is_cosimulation_available() const override {return true;}
    virtual bool is_modelexchange_available() const override {return false;}

    // Problem-specific data members   
    std::array<double, 4> q;
    std::array<double, 4> q_t;
    double len = 0.5;
    double m = 1.0;
    double M = 1.0;
    const double g = 9.81;
    fmi2Boolean approximateOn = fmi2False;
    std::array<double, 4> k1 = {0,0,0,0};
    std::array<double, 4> k2 = {0,0,0,0};
    std::array<double, 4> k3 = {0,0,0,0};
    std::array<double, 4> k4 = {0,0,0,0};

    /// FMU_ACTION: set flags accordingly to mode availability
    const bool cosim_available = true;
    const bool modelexchange_available = false;


};


// FMU_ACTION:: implement the following functions
FmuComponentBase* fmi2Instantiate_getPointer(fmi2String instanceName, fmi2Type fmuType, fmi2String fmuGUID){
    return new FmuComponent(instanceName, fmuType, fmuGUID);
}


