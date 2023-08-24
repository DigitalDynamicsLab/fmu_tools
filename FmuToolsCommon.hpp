#pragma once
#include <string>
#include "fmi2_headers/fmi2FunctionTypes.h"

/////////////////////////////////////////////////////////////////////////////////////////////////
std::string fmi2Status_toString(fmi2Status status){
    switch (status)
    {
    case fmi2Status::fmi2Discard:
        return "Discard";
        break;
    case fmi2Status::fmi2Error:
        return "Error";
        break;
    case fmi2Status::fmi2Fatal:
        return "Fatal";
        break;
    case fmi2Status::fmi2OK:
        return "OK";
        break;
    case fmi2Status::fmi2Pending:
        return "Pending";
        break;
    case fmi2Status::fmi2Warning:
        return "Warning";
        break;
    default:
        throw std::exception("Wrong fmi2Status");
        break;
    }
}

/// Class holding a reference to a single FMU variable
/// Note that this is retrieved from the information from the XML

class FmuScalarVariable {
public:
    FmuScalarVariable(){
        ptr.fmi2Real_ptr = nullptr; // just to provide a pre-initialization and avoid warnings
    }


    bool operator<(const FmuScalarVariable& other) const {
        return this->type != other.type ? this->type < other.type : this->valueReference < other.valueReference;
    }

    bool operator==(const FmuScalarVariable& other) const {
        // according to FMI Reference there can exist two different variables with same type and same valueReference;
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

    enum class FmuScalarVariableType {
        FMU_REAL = 0, // number gives the order in which each type is printed in the modelDescription.xml
        FMU_INTEGER = 1,
        FMU_BOOLEAN = 2,
        FMU_STRING = 3,
        FMU_UNKNOWN = 4
    };

    FmuScalarVariableType type = FmuScalarVariableType::FMU_UNKNOWN;




};