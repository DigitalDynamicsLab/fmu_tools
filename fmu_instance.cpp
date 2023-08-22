
//#define FMI2_FUNCTION_PREFIX MyModel_
#include "fmu_entity.hpp"
#include "fmi2_headers/fmi2Functions.h"
#include <cassert>
#include <vector>
#include <array>
#include <map>
#include <iostream>
#include <string>

#define _USE_MATH_DEFINES
#include <math.h>



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

class FmuInstance: public ChFmuComponent{
public:
    FmuInstance(fmi2String _instanceName, fmi2Type _fmuType, fmi2String _fmuGUID): g(9.81), ChFmuComponent(_instanceName, _fmuType, _fmuGUID){
        fmi2Real_map[0] = &q_t[0];
        fmi2Real_map[1] = &q[0];
        fmi2Real_map[2] = &q[1];
        fmi2Real_map[3] = &q_t[2];
        fmi2Real_map[4] = &q[2];
        fmi2Real_map[5] = &q[3];
        fmi2Real_map[6] = &len;
        fmi2Real_map[7] = &m;
        fmi2Real_map[8] = &M;

        fmi2Boolean_map[0] = &approximateOn;

        q = {0, 0, 0, M_PI/4};
    }
    virtual ~FmuInstance(){}

    virtual fmi2Status DoStep(double _stepSize = -1) override {
        if (_stepSize<0){
            if (fmi2CallbackLoggerCategoryID>=2){
                std::string str = "fmi2DoStep requeste a negative stepsize: " + std::to_string(_stepSize) + ".\n";
                callbackFunctions.logger(callbackFunctions.componentEnvironment, instanceName.c_str(), fmi2Status::fmi2Warning, "Warning", str.c_str());
            }

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


        if (fmi2CallbackLoggerCategoryID>=3){
            std::string str = "Step at time: " + std::to_string(time) + " succeeded.\n";
            callbackFunctions.logger(callbackFunctions.componentEnvironment, instanceName.c_str(), fmi2Status::fmi2OK, "Status", str.c_str());
        }

        return fmi2Status::fmi2OK;
    }

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
    std::array<double, 4> q;
    std::array<double, 4> q_t;
    double len = 0.5;
    double m = 1.0;
    double M = 1.0;
    const double g;

    fmi2Boolean approximateOn = fmi2False;

    // TEMP
    std::array<double, 4> k1;
    std::array<double, 4> k2;
    std::array<double, 4> k3;
    std::array<double, 4> k4;
};

