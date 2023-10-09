
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
        UnitDefinitionType UD_J ("J"); UD_J.kg = 1; UD_J.m = 2; UD_J.s = -2;
        addUnitDefinition(UD_J);

        /// FMU_ACTION: declare relevant variables
        AddFmuVariable(&q_t[0], "x_tt",     FmuVariable::Type::Real, "m/s2",   "cart acceleration");
        AddFmuVariable(&q[0],   "x_t",      FmuVariable::Type::Real, "m/s",    "cart velocity");
        AddFmuVariable(&q[1],   "x",        FmuVariable::Type::Real, "m",      "cart position");
        AddFmuVariable(&q_t[2], "theta_tt", FmuVariable::Type::Real, "rad/s2", "pendulum ang acceleration");
        AddFmuVariable(&q[2],   "theta_t",  FmuVariable::Type::Real, "rad/s",  "pendulum ang velocity");
        AddFmuVariable(&q[3],   "theta",    FmuVariable::Type::Real, "rad",    "pendulum angle");
        auto& fmu_len = AddFmuVariable(&len,  "len", FmuVariable::Type::Real, "m",  "pendulum length", FmuVariable::CausalityType::parameter, FmuVariable::VariabilityType::fixed);
        auto& fmu_m =   AddFmuVariable(&m,    "m",   FmuVariable::Type::Real, "kg", "pendulum mass",   FmuVariable::CausalityType::parameter, FmuVariable::VariabilityType::fixed);
        auto& fmu_M =   AddFmuVariable(&M,    "M",   FmuVariable::Type::Real, "kg", "cart mass",       FmuVariable::CausalityType::parameter, FmuVariable::VariabilityType::fixed);

        
        /// FMU_ACTION: you can also pass std::functions to get/set the value of the variable if queried
        //AddFmuVariable(std::make_pair(
        //    std::function<fmi2Real()>([this]() -> fmi2Real {
        //        return (0.5*(this->m*this->len*this->len/3)*(this->q_t[2]*this->q_t[2]));}),
        //    std::function<void(fmi2Real)>([this](fmi2Real) {})),
        //    "kineticEnergy",    FmuVariable::Type::Real, "J",    "kinetic energy");

        // same result is achieved through helper function 'MAKE_FUN_PAIR'
        AddFmuVariable(MAKE_GETSET_PAIR(fmi2Real,
            { return (0.5*(this->m*this->len*this->len/3)*(this->q_t[2]*this->q_t[2]));},
            {}),   "kineticEnergy",    FmuVariable::Type::Real, "J",    "kinetic energy");


        // FMU_ACTION: start value will be automatically grabbed from 'len' during AddFmuVariable;
        // use the following statement only if the start value is not already in 'len' when called AddFmuVariable
        //fmu_len.SetStartVal(len);


        auto& fmu_approximateOn = AddFmuVariable(&approximateOn, "approximateOn", FmuVariable::Type::Boolean, "1", "use approximated model", FmuVariable::CausalityType::parameter, FmuVariable::VariabilityType::fixed);

        // Additional commands
        q = {0, 0, 0, 3.14159265358979323846/4};
    }

    virtual void _enterInitializationMode() override {}

    virtual void _exitInitializationMode() override {}

    virtual ~FmuComponent(){}

    /// FMU_ACTION: override DoStep of the base class with the problem-specific implementation
    virtual fmi2Status _doStep(fmi2Real currentCommunicationPoint, fmi2Real communicationStepSize, fmi2Boolean noSetFMUStatePriorToCurrentPoint) override;


    // Problem-specific functions
    double get_x_tt(double th_t, double th);
    double get_th_tt(double th_t, double th);
    double get_x_tt_linear(double th_t, double th);
    double get_th_tt_linear(double th_t, double th);
    void get_q_t(const std::array<double, 4>& _q, std::array<double, 4>& q_t);


protected:

    /// FMU_ACTION: override the base methods so to retrieve the proper answer
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

    const bool cosim_available = true;
    const bool modelexchange_available = false;


};



