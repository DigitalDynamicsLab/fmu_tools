
//#define FMI2_FUNCTION_PREFIX MyModel_
#include "fmu_instance.h"
#include <cassert>
#include <vector>
#include <array>
#include <map>
#include <iostream>
#include <string>

#define _USE_MATH_DEFINES
#include <math.h>


// FMU_ACTION:: implement the following functions
ChFmuComponent* fmi2Instantiate_getPointer(fmi2String instanceName, fmi2Type fmuType, fmi2String fmuGUID){
    return new FmuInstance(instanceName, fmuType, fmuGUID);
}


std::array<double, 4> operator*(double lhs, const std::array<double, 4>& rhs)
{
    std::array<double, 4> temp;
    temp[0] = rhs[0]*lhs;
    temp[1] = rhs[1]*lhs;
    temp[2] = rhs[2]*lhs;
    temp[3] = rhs[3]*lhs;
    return temp;
}

std::array<double, 4> operator+(const std::array<double, 4>& lhs, const std::array<double, 4>& rhs)
{
    std::array<double, 4> temp;
    temp[0] = rhs[0]+lhs[0];
    temp[1] = rhs[1]+lhs[1];
    temp[2] = rhs[2]+lhs[2];
    temp[3] = rhs[3]+lhs[3];
    return temp;
}

double FmuInstance::get_x_tt(double th_t, double th) {
    return (m*sin(th)*(len*th_t*th_t + g*cos(th)))/(-m*cos(th)*cos(th) + M + m);
}

double FmuInstance::get_th_tt(double th_t, double th) {
    return -(sin(th)*(len*m*cos(th)*th_t*th_t + M*g + g*m))/(len*(-m*cos(th)*cos(th) + M + m));
}

double FmuInstance::get_x_tt_linear(double th_t, double th) {
    return (m*th*(len*th_t*th_t + g))/M;
}

double FmuInstance::get_th_tt_linear(double th_t, double th) {
    return -(th*(len*m*th_t*th_t + M*g + g*m))/(len*M);
}

void FmuInstance::get_q_t(const std::array<double, 4>& _q, std::array<double, 4>& q_t){
    q_t[0] = approximateOn ? get_x_tt_linear(_q[2], _q[3]) : get_x_tt(_q[2], _q[3]);
    q_t[1] = _q[0];
    q_t[2] = approximateOn ? get_th_tt_linear(_q[2], _q[3]) : get_th_tt(_q[2], _q[3]);
    q_t[3] = _q[2];
}

fmi2Status FmuInstance::DoStep(double _stepSize) {
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



