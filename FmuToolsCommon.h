#pragma once
#include "fmi2_headers/fmi2FunctionTypes.h"
#include "fmi2_headers/fmi2Functions.h"
#include <stdexcept>
#include <string>
#include <typeindex>
#include <unordered_map>
#include <cassert>

#if _HAS_CXX17
#else
#include "variant/variant.hpp"
#endif


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

using ptr_type = mpark::variant<fmi2Real*, fmi2Integer*, fmi2String*>;
using start_type = mpark::variant<fmi2Real, fmi2Integer, std::string>;

/// Class holding a reference to a single FMU variable
/// Note that this is retrieved from the information from the XML

class FmuVariable {


public:
    enum class Type{
        FMU_REAL = 0, // numbering gives the order in which each type is printed in the modelDescription.xml
        FMU_INTEGER = 1,
        FMU_BOOLEAN = 2,
        FMU_STRING = 3,
        FMU_UNKNOWN = 4
    };

    FmuVariable() : FmuVariable("", FmuVariable::Type::FMU_REAL){}


    FmuVariable(const FmuVariable& other) {
        
        type = other.type;
        name = other.name;
        valueReference = other.valueReference;
        unitname = other.unitname;
        causality = other.causality;
        variability = other.variability;
        initial = other.initial;
        description = other.description;
        // TODO: check if it is not enough the two below
        ptr = other.ptr;
        start = other.start;

        //switch (this->type)
        //{
        //case FmuVariable::Type::FMU_REAL:
        //    this->ptr = other.ptr;
        //    this->start = other.start;
        //    break;
        //case FmuVariable::Type::FMU_INTEGER:
        //    this->ptr = other.ptr;
        //    this->start = other.start;
        //    break;
        //case FmuVariable::Type::FMU_BOOLEAN:
        //    this->ptr = other.ptr;
        //    this->start = other.start;
        //    break;
        //case FmuVariable::Type::FMU_STRING:
        //    this->fmi2String_start_buf = other_buf;
        //    this->ptr = other.ptr; //TODO: check
        //    this->start = this->fmi2String_start_buf.c_str();
        //    break;
        //case FmuVariable::Type::FMU_UNKNOWN:
        //    throw std::runtime_error("Cannot set FmuVariable::type to FMU_UNKNOWN.");
        //    break;
        //default:
        //    throw std::runtime_error("Developer error: passed a not registered FmuVariable::Type.");
        //    break;
        //}

        allowed_start = other.allowed_start;
        required_start = other.required_start;
        has_start = other.has_start;

    }

    FmuVariable(const std::string& _name, FmuVariable::Type _type): name(_name), type(_type)
    {
        //switch (this->type)
        //{
        //case FmuVariable::Type::FMU_REAL:
        //    this->ptr = nullptr;
        //    this->start = 0.0;
        //    break;
        //case FmuVariable::Type::FMU_INTEGER:
        //    this->ptr = nullptr;
        //    this->start = 0;
        //    break;
        //case FmuVariable::Type::FMU_BOOLEAN:
        //    this->ptr = nullptr;
        //    this->start = 0;
        //    break;
        //case FmuVariable::Type::FMU_STRING:
        //    this->ptr = nullptr;
        //    this->start = 0;
        //    break;
        //case FmuVariable::Type::FMU_UNKNOWN:
        //    throw std::runtime_error("Cannot set FmuVariable::type to FMU_UNKNOWN.");
        //    break;
        //default:
        //    throw std::runtime_error("Developer error: passed a not registered FmuVariable::Type.");
        //    break;
        //}

    }


    bool operator<(const FmuVariable& other) const {
        return this->type != other.type ? this->type < other.type : this->valueReference < other.valueReference;
    }

    bool operator==(const FmuVariable& other) const {
        // according to FMI Reference there can exist two different variables with same type and same valueReference;
        // they are called "alias" thus they should be allowed but not considered equal
        return this->name == other.name;
    }

    
    // Copy assignment operator
    FmuVariable& operator=(const FmuVariable& other) {
        if (this == &other) {
            return *this; // Self-assignment guard
        }

        type = other.type;
        name = other.name;
        valueReference = other.valueReference;
        unitname = other.unitname;
        causality = other.causality;
        variability = other.variability;
        initial = other.initial;
        description = other.description;
        // TODO: check if it is not enough the two below
        ptr = other.ptr;
        start = other.start;
        allowed_start = other.allowed_start;
        required_start = other.required_start;
        has_start = other.has_start;

        return *this;
    }

    void SetPtr(const ptr_type& ptr) {
        this->ptr = ptr;
    }

    template <class T>
    void GetPtr(T** ptr_address) const {
        *ptr_address = mpark::get<T*>(this->ptr);
    }

    template <typename T>
    void SetStartVal(T startval){
        if (allowed_start)
            has_start = true;
        else
            return;
        this->start = startval;
    }

    template <>
    void SetStartVal<fmi2String>(fmi2String startval){
        if (allowed_start)
            has_start = true;
        else
            return;
        this->start = std::string(startval);
    }


    std::string GetStartVal() const {
        if (const fmi2Real* start_ptr = mpark::get_if<fmi2Real>(&this->start))
            return std::to_string(*start_ptr);
        if (const fmi2Integer* start_ptr = mpark::get_if<fmi2Integer>(&this->start))
            return std::to_string(*start_ptr);
        if (const fmi2Boolean* start_ptr = mpark::get_if<fmi2Boolean>(&this->start))
            return std::to_string(*start_ptr);
        if (const std::string* start_ptr = mpark::get_if<std::string>(&this->start))
            return *start_ptr;
        return "";
    }

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

    const inline std::string& GetName() const { return name;}
    const inline std::string& GetCausality() const { return causality;}
    const inline std::string& GetVariability() const { return variability;}
    const inline std::string& GetInitial() const { return initial;}
    const inline std::string& GetDescription() const { return description;}
    void SetDescription(const std::string& _description) { description = _description;}
    const inline fmi2ValueReference GetValueReference() const { return valueReference;}
    void SetValueReference(fmi2ValueReference valref) { valueReference = valref;}
    const inline std::string& GetUnitName() const { return unitname;}
    void SetUnitName(const std::string& _unitname) { unitname = _unitname;}
    Type GetType() const { return type;}

    void SetCausalityVariabilityInitial(const std::string& causality, const std::string& variability, const std::string& initial){
        assert(
            (causality.empty() ||
            !causality.compare("parameter") ||
            !causality.compare("calculatedParameter") ||
            !causality.compare("input") ||
            !causality.compare("output") ||
            !causality.compare("local") ||
            !causality.compare("independent"))
            && "Requested bad formatted \"causality\"");

        assert(
            (variability.empty() ||
            !variability.compare("constant") ||
            !variability.compare("fixed") ||
            !variability.compare("tunable") ||
            !variability.compare("discrete") ||
            !variability.compare("continuous"))
            && "Requested bad formatted \"variability\"");

        // TODO: set initial to something based on table pag 51


        assert(
            (initial.empty() ||
            !initial.compare("exact") ||
            !initial.compare("approx") ||
            !initial.compare("calculated"))
            && "Requested bad formatted \"initial\"");


        if (!initial.compare("calculated") || !causality.compare("independent")){
            allowed_start = false;
            required_start = false;
        }

        if (!initial.compare("exact") || !initial.compare("approx") || !causality.compare("input")){
            allowed_start = true;
            required_start = true;
        }

    }



protected:
    Type type = Type::FMU_UNKNOWN;
    std::string name;
    fmi2ValueReference valueReference = 0;
    std::string unitname = "1";
    std::string causality = "";
    std::string variability = "";
    std::string initial = "";
    std::string description = "";


    bool allowed_start = true;
    bool required_start = false;
    bool has_start = false;



    // TODO: replace with std::variant as soon as C++17 becomes available

    //union ptr_type{
    //    fmi2Real* fmi2Real_ptr;
    //    fmi2Integer* fmi2Integer_ptr;
    //    fmi2Boolean* fmi2Boolean_ptr;
    //    fmi2String* fmi2String_ptr;
    //} ptr;

    //union start_type{
    //    fmi2Real fmi2Real_start;
    //    fmi2Integer fmi2Integer_start;
    //    fmi2Boolean fmi2Boolean_start;
    //    fmi2String fmi2String_start;
    //} start;

    ptr_type ptr;
    start_type start;

};

    
