#pragma once
#include <string>
#include "fmi2_headers/fmi2FunctionTypes.h"
#include "fmi2_headers/fmi2Functions.h"
#include <stdexcept>


/////////////////////////////////////////////////////////////////////////////////////////////////
std::string FMI2_Export fmi2Status_toString(fmi2Status status);


struct UnitDefinitionType{

    UnitDefinitionType(const std::string& _name = "1"): name(_name){}
    UnitDefinitionType(const std::string& _name, int _kg, int _m, int _s, int _A, int _K, int _mol, int _cd, int _rad):
        name(_name),
        kg(_kg),
        m(_m),
        s(_s),
        A(_A),
        K(_K),
        mol(_mol),
        cd(_cd),
        rad(_rad){}
    virtual ~UnitDefinitionType(){}

    std::string name;
    int kg = 0;
    int m = 0;
    int s = 0;
    int A = 0;
    int K = 0;
    int mol = 0;
    int cd = 0;
    int rad = 0;

    struct Hash {
        size_t operator()(const UnitDefinitionType& p) const {
            return std::hash<std::string>()(p.name)
            ^ std::hash<int>()(p.kg)
            ^ std::hash<int>()(p.m)
            ^ std::hash<int>()(p.s)
            ^ std::hash<int>()(p.A)
            ^ std::hash<int>()(p.K)
            ^ std::hash<int>()(p.mol)
            ^ std::hash<int>()(p.cd)
            ^ std::hash<int>()(p.rad);
        }
    };

    bool operator==(const UnitDefinitionType& other) const {
        return name == other.name;
    }

};


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

    std::string unitType = "1";


    FmuScalarVariableType type = FmuScalarVariableType::FMU_UNKNOWN;

    static std::string FmuScalarVariableType_toString(FmuScalarVariableType type){
        switch (type)
        {
        case FmuScalarVariable::FmuScalarVariableType::FMU_REAL:
            return "Real";
            break;
        case FmuScalarVariable::FmuScalarVariableType::FMU_INTEGER:
            return "Integer";
            break;
        case FmuScalarVariable::FmuScalarVariableType::FMU_BOOLEAN:
            return "Boolean";
            break;
        case FmuScalarVariable::FmuScalarVariableType::FMU_UNKNOWN:
            return "Unknown";
            break;
        case FmuScalarVariable::FmuScalarVariableType::FMU_STRING:
            return "String";
            break;
        default:
            throw std::runtime_error("FmuScalarVariableType_toString: received bad type.");
            
            break;  
        }
        return "";
    }

};

    
