
#pragma once
//#define FMI2_FUNCTION_PREFIX MyModel_
#include "fmu_entity.h"
#include <cassert>
#include <vector>
#include <array>
#include <map>
#include <iostream>
#include <string>

#define _USE_MATH_DEFINES
#include <math.h>




class FmuInstance: public ChFmuComponent{
public:
    FmuInstance(fmi2String _instanceName, fmi2Type _fmuType, fmi2String _fmuGUID):
        ChFmuComponent(_instanceName, _fmuType, _fmuGUID, cosim_available, modelexchange_available)
    {

        /// FMU_ACTION: define new units if needed
        //UnitDefinitionType UD_rad_s4 ("rad/s4"); UD_rad_s4.s = -4; UD_rad_s4.rad = 1;
        //AddUnitDefinition(UD_rad_s4);

        /// FMU_ACTION: declare relevant variables
        addFmuVariable(&q_t[0], "x_tt",     FmuScalarVariable::Type::FMU_REAL, "m/s2",   "cart acceleration");
        addFmuVariable(&q[0],   "x_t",      FmuScalarVariable::Type::FMU_REAL, "m/s",    "cart velocity");
        addFmuVariable(&q[1],   "x",        FmuScalarVariable::Type::FMU_REAL, "m",      "cart position");
        addFmuVariable(&q_t[2], "theta_tt", FmuScalarVariable::Type::FMU_REAL, "rad/s2", "pendulum ang acceleration");
        addFmuVariable(&q[2],   "theta_t",  FmuScalarVariable::Type::FMU_REAL, "rad/s",  "pendulum ang velocity");
        addFmuVariable(&q[3],   "theta",    FmuScalarVariable::Type::FMU_REAL, "rad",    "pendulum angle");
        addFmuVariable(&len,    "len",      FmuScalarVariable::Type::FMU_REAL, "m",      "pendulum length", "parameter", "fixed");
        addFmuVariable(&m,      "m",        FmuScalarVariable::Type::FMU_REAL, "kg",     "pendulum mass",   "parameter", "fixed");
        addFmuVariable(&M,      "M",        FmuScalarVariable::Type::FMU_REAL, "kg",     "cart mass",       "parameter", "fixed");

        addFmuVariable(&approximateOn, "approximateOn", FmuScalarVariable::Type::FMU_BOOLEAN, "1", "use approximated model", "parameter", "fixed");

        // Additional commands
        q = {0, 0, 0, M_PI/4};
    }

    virtual ~FmuInstance(){}

    /// FMU_ACTION: override DoStep of the base class with the problem-specific implementation
    virtual fmi2Status DoStep(double _stepSize = -1) override;


    // Problem-specific functions
    double get_x_tt(double th_t, double th);
    double get_th_tt(double th_t, double th);
    double get_x_tt_linear(double th_t, double th);
    double get_th_tt_linear(double th_t, double th);
    void get_q_t(const std::array<double, 4>& _q, std::array<double, 4>& q_t);


private:

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

