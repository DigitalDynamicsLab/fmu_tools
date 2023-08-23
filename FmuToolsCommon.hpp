#pragma once
#include <string>
#include "fmi2_headers/fmi2FunctionTypes.h"

/////////////////////////////////////////////////////////////////////////////////////////////////


/// Class holding a reference to a single FMU variable
/// Note that this is retrieved from the information from the XML

class FmuScalarVariable {
public:
    FmuScalarVariable(){}


    bool operator<(const FmuScalarVariable& other) const {
        return this->type != other.type ? this->type < other.type : this->valueReference < other.valueReference;
    }

    bool operator==(const FmuScalarVariable& other) const {
        // there can exist two different variables with same type and same valueReference;
        // they are called "alias" thus they should be allowed but not considered equal
        return this->name == other.name;
    }


    std::string name;
    fmi2ValueReference valueReference = 0;
    std::string description = "";
    std::string causality = "";
    std::string variability = "";
    std::string initial = "";

    union ptr_type{
        fmi2Real* fmi2Real_ptr;
        fmi2Integer* fmi2Integer_ptr;
        fmi2Boolean* fmi2Boolean_ptr;
    } ptr;

    enum class e_fmu_variable_type {
        FMU_REAL = 0,
        FMU_INTEGER = 1,
        FMU_BOOLEAN = 2,
        FMU_STRING = 3,
        FMU_UNKNOWN = 4
    };

    e_fmu_variable_type type = e_fmu_variable_type::FMU_UNKNOWN;




};