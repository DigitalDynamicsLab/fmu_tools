
//#define FMI2_FUNCTION_PREFIX MyModel_
#include "fmu_instance.h"
#include "fmi2_headers/fmi2Functions.h"
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



