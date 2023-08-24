
#pragma once
//#define FMI2_FUNCTION_PREFIX MyModel_
#include "fmu_entity.h"
#include "fmi2_headers/fmi2Functions.h"
#include <cassert>
#include <vector>
#include <array>
#include <map>
#include <iostream>
#include <string>

#define _USE_MATH_DEFINES
#include <math.h>



std::array<double, 4> operator*(double lhs, const std::array<double, 4>& rhs);


std::array<double, 4> operator+(const std::array<double, 4>& lhs, const std::array<double, 4>& rhs);


class FmuInstance: public ChFmuComponent{
public:
    FmuInstance(fmi2String _instanceName, fmi2Type _fmuType, fmi2String _fmuGUID):
        ChFmuComponent(_instanceName, _fmuType, _fmuGUID, cosim_available, modelexchange_available)
    {

        /// FMU_ACTION: define new units if needed
        //UnitDefinitionType UD_rad_s4 ("rad/s4"); UD_rad_s4.s = -4; UD_rad_s4.rad = 1;
        //AddUnitDefinition(UD_rad_s4);

        /// FMU_ACTION: declare relevant variables
        addFmuVariableReal(&q_t[0], "x_tt",     "m/s2",   "cart acceleration");
        addFmuVariableReal(&q[0],   "x_t",      "m/s",    "cart velocity");
        addFmuVariableReal(&q[1],   "x",        "m",      "cart position");
        addFmuVariableReal(&q_t[2], "theta_tt", "rad/s2", "pendulum ang acceleration");
        addFmuVariableReal(&q[2],   "theta_t",  "rad/s",  "pendulum ang velocity");
        addFmuVariableReal(&q[3],   "theta",    "rad",    "pendulum angle");
        addFmuVariableReal(&len,    "len",      "m",      "pendulum length", "parameter", "fixed");
        addFmuVariableReal(&m,      "m",        "kg",     "pendulum mass",   "parameter", "fixed");
        addFmuVariableReal(&M,      "M",        "kg",     "cart mass",       "parameter", "fixed");

        fmi2Boolean_map[0] = &approximateOn; // TODO: same approach as for Real

        // Additional commands
        q = {0, 0, 0, M_PI/4};
    }

    virtual ~FmuInstance(){}

    /// FMU_ACTION: override DoStep of the base class with the problem-specific implementation
    virtual fmi2Status DoStep(double _stepSize = -1) override {
        if (_stepSize<0){
            sendToLog("Step at time: " + std::to_string(time) + " succeeded.\n", fmi2Status::fmi2Warning, "logStatusWarning");
            return fmi2Status::fmi2Warning;
        }

        _stepSize = std::min(_stepSize, stepSize);

        get_q_t(q, k1); // = q_t(q(step, :));
        get_q_t(q + (0.5*_stepSize)*k1, k2); // = q_t(q(step, :) + stepsize*k1/2);
        get_q_t(q + (0.5*_stepSize)*k2, k3); // = q_t(q(step, :) + stepsize*k2/2);
        get_q_t(q + _stepSize*k3, k4); // = q_t(q(step, :) + stepsize*k3);

        q_t = (1.0/6.0)*(k1 + 2.0*k2 + 2.0*k3 + k4);
        q = q + _stepSize*q_t;
        time = time + _stepSize;

        sendToLog("Step at time: " + std::to_string(time) + " succeeded.\n", fmi2Status::fmi2OK, "logAll");

        return fmi2Status::fmi2OK;
    }


    // Problem-specific functions

    double get_x_tt(double th_t, double th) {
        return (m*sin(th)*(len*th_t*th_t + g*cos(th)))/(-m*cos(th)*cos(th) + M + m);
    }

    double get_th_tt(double th_t, double th) {
        return -(sin(th)*(len*m*cos(th)*th_t*th_t + M*g + g*m))/(len*(-m*cos(th)*cos(th) + M + m));
    }

    double get_x_tt_linear(double th_t, double th) {
        return (m*th*(len*th_t*th_t + g))/M;
    }

    double get_th_tt_linear(double th_t, double th) {
        return -(th*(len*m*th_t*th_t + M*g + g*m))/(len*M);
    }

    void get_q_t(const std::array<double, 4>& _q, std::array<double, 4>& q_t){
        q_t[0] = approximateOn ? get_x_tt_linear(_q[2], _q[3]) : get_x_tt(_q[2], _q[3]);
        q_t[1] = _q[0];
        q_t[2] = approximateOn ? get_th_tt_linear(_q[2], _q[3]) : get_th_tt(_q[2], _q[3]);
        q_t[3] = _q[2];
    }


private:

    /// FMU_ACTION: declare all variables used by the fmu
    
    std::array<double, 4> q;
    std::array<double, 4> q_t;
    double len = 0.5;
    double m = 1.0;
    double M = 1.0;
    const double g = 9.81;

    fmi2Boolean approximateOn = fmi2False;

    /// FMU_ACTION: set flags accordingly to available model
    const bool cosim_available = true;
    const bool modelexchange_available = false;

    // TEMP
    std::array<double, 4> k1 = {0,0,0,0};
    std::array<double, 4> k2 = {0,0,0,0};
    std::array<double, 4> k3 = {0,0,0,0};
    std::array<double, 4> k4 = {0,0,0,0};
};

