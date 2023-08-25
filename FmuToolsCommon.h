#pragma once
#include "fmi2_headers/fmi2FunctionTypes.h"
#include "fmi2_headers/fmi2Functions.h"
#include <stdexcept>
#include <string>
#include <typeindex>
#include <unordered_map>
#include <cassert>


/////////////////////////////////////////////////////////////////////////////////////////////////


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
            return std::hash<std::string>()(p.name);
        }
    };

    bool operator==(const UnitDefinitionType& other) const {
        return name == other.name;
    }

};


/// Class holding a reference to a single FMU variable
/// Note that this is retrieved from the information from the XML

class FmuVariable {
public:
    FmuVariable(){
        ptr.fmi2Real_ptr = nullptr; // just to provide a pre-initialization and avoid warnings
    }


    bool operator<(const FmuVariable& other) const {
        return this->type != other.type ? this->type < other.type : this->valueReference < other.valueReference;
    }

    bool operator==(const FmuVariable& other) const {
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

    enum class Type{
        FMU_REAL = 0, // numbering gives the order in which each type is printed in the modelDescription.xml
        FMU_INTEGER = 1,
        FMU_BOOLEAN = 2,
        FMU_STRING = 3,
        FMU_UNKNOWN = 4
    } type = Type::FMU_UNKNOWN;
;

    union ptr_type{
        fmi2Real* fmi2Real_ptr;
        fmi2Integer* fmi2Integer_ptr;
        fmi2Boolean* fmi2Boolean_ptr;
        fmi2String* fmi2String_ptr;
    } ptr;

    template <typename T>
    void SetPtr(FmuVariable::Type scalartype, T* ptr) {
        // DEV: the reinterpret cast is actually doing nothing since the T* should already be of the same type
        switch (scalartype)
        {
        case FmuVariable::Type::FMU_REAL:
            assert(typeid(this->ptr.fmi2Real_ptr) == typeid(ptr));
            this->ptr.fmi2Real_ptr = reinterpret_cast<fmi2Real*>(ptr);
            break;
        case FmuVariable::Type::FMU_INTEGER:
            assert(typeid(this->ptr.fmi2Integer_ptr) == typeid(ptr));
            this->ptr.fmi2Integer_ptr = reinterpret_cast<fmi2Integer*>(ptr);
            break;
        case FmuVariable::Type::FMU_BOOLEAN:
            assert(typeid(this->ptr.fmi2Boolean_ptr) == typeid(ptr));
            this->ptr.fmi2Boolean_ptr = reinterpret_cast<fmi2Boolean*>(ptr);
            break;
        case FmuVariable::Type::FMU_STRING:
            assert(typeid(this->ptr.fmi2String_ptr) == typeid(ptr));
            this->ptr.fmi2String_ptr = reinterpret_cast<fmi2String*>(ptr);
            break;
        case FmuVariable::Type::FMU_UNKNOWN:
            throw std::runtime_error("FMU_UNKNOWN not implemented yet.");
            break;
        default:
            break;
        }
    }

    template <typename T>
    void GetPtr(FmuVariable::Type scalartype, T** ptr) const {
        // DEV: the reinterpret cast is actually doing nothing since the T* should already be of the same type
        switch (scalartype)
        {
        case FmuVariable::Type::FMU_REAL:
            assert(typeid(this->ptr.fmi2Real_ptr) == typeid(*ptr));
            *ptr = reinterpret_cast<T*>(this->ptr.fmi2Real_ptr);
            break;
        case FmuVariable::Type::FMU_INTEGER:
            assert(typeid(this->ptr.fmi2Integer_ptr) == typeid(*ptr));
            *ptr = reinterpret_cast<T*>(this->ptr.fmi2Integer_ptr);
            break;
        case FmuVariable::Type::FMU_BOOLEAN:
            assert(typeid(this->ptr.fmi2Boolean_ptr) == typeid(*ptr));
            *ptr = reinterpret_cast<T*>(this->ptr.fmi2Boolean_ptr);
            break;
        case FmuVariable::Type::FMU_STRING:
            assert(typeid(this->ptr.fmi2String_ptr) == typeid(*ptr));
            *ptr = reinterpret_cast<T*>(this->ptr.fmi2String_ptr);
            break;
        case FmuVariable::Type::FMU_UNKNOWN:
            throw std::runtime_error("FMU_UNKNOWN not implemented yet.");
            break;
        default:
            break;
        }
    }


    std::string unitname = "1";



    static std::string Type_toString(Type type){
        switch (type)
        {
        case FmuVariable::Type::FMU_REAL:
            return "Real";
            break;
        case FmuVariable::Type::FMU_INTEGER:
            return "Integer";
            break;
        case FmuVariable::Type::FMU_BOOLEAN:
            return "Boolean";
            break;
        case FmuVariable::Type::FMU_UNKNOWN:
            return "Unknown";
            break;
        case FmuVariable::Type::FMU_STRING:
            return "String";
            break;
        default:
            throw std::runtime_error("Type_toString: received bad type.");
            
            break;  
        }
        return "";
    }

};

    
