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
        //ptr = other.ptr;
        //start = other.start;

        switch (this->type)
        {
        case FmuVariable::Type::FMU_REAL:
            this->ptr.fmi2Real_ptr = other.ptr.fmi2Real_ptr;
            this->start.fmi2Real_start = other.start.fmi2Real_start;
            break;
        case FmuVariable::Type::FMU_INTEGER:
            this->ptr.fmi2Integer_ptr = other.ptr.fmi2Integer_ptr;
            this->start.fmi2Integer_start = other.start.fmi2Integer_start;
            break;
        case FmuVariable::Type::FMU_BOOLEAN:
            this->ptr.fmi2Boolean_ptr = other.ptr.fmi2Boolean_ptr;
            this->start.fmi2Boolean_start = other.start.fmi2Boolean_start;
            break;
        case FmuVariable::Type::FMU_STRING:
            this->fmi2String_start_buf = other.fmi2String_start_buf;
            this->ptr.fmi2String_ptr = other.ptr.fmi2String_ptr; //TODO: check
            this->start.fmi2String_start = this->fmi2String_start_buf.c_str();
            break;
        case FmuVariable::Type::FMU_UNKNOWN:
            throw std::runtime_error("Cannot set FmuVariable::type to FMU_UNKNOWN.");
            break;
        default:
            throw std::runtime_error("Developer error: passed a not registered FmuVariable::Type.");
            break;
        }

        allowed_start = other.allowed_start;
        required_start = other.required_start;
        has_start = other.has_start;

    }

    FmuVariable(const std::string& _name, FmuVariable::Type _type): name(_name), type(_type)
    {
        switch (this->type)
        {
        case FmuVariable::Type::FMU_REAL:
            this->ptr.fmi2Real_ptr = nullptr;
            this->start.fmi2Real_start = 0.0;
            break;
        case FmuVariable::Type::FMU_INTEGER:
            this->ptr.fmi2Integer_ptr = nullptr;
            this->start.fmi2Integer_start = 0;
            break;
        case FmuVariable::Type::FMU_BOOLEAN:
            this->ptr.fmi2Boolean_ptr = nullptr;
            this->start.fmi2Boolean_start = 0;
            break;
        case FmuVariable::Type::FMU_STRING:
            this->ptr.fmi2String_ptr = nullptr;
            this->start.fmi2String_start = 0;
            break;
        case FmuVariable::Type::FMU_UNKNOWN:
            throw std::runtime_error("Cannot set FmuVariable::type to FMU_UNKNOWN.");
            break;
        default:
            throw std::runtime_error("Developer error: passed a not registered FmuVariable::Type.");
            break;
        }

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
        //ptr = other.ptr;
        //start = other.start;

        switch (this->type)
        {
        case FmuVariable::Type::FMU_REAL:
            this->ptr.fmi2Real_ptr = other.ptr.fmi2Real_ptr;
            this->start.fmi2Real_start = other.start.fmi2Real_start;
            break;
        case FmuVariable::Type::FMU_INTEGER:
            this->ptr.fmi2Integer_ptr = other.ptr.fmi2Integer_ptr;
            this->start.fmi2Integer_start = other.start.fmi2Integer_start;
            break;
        case FmuVariable::Type::FMU_BOOLEAN:
            this->ptr.fmi2Boolean_ptr = other.ptr.fmi2Boolean_ptr;
            this->start.fmi2Boolean_start = other.start.fmi2Boolean_start;
            break;
        case FmuVariable::Type::FMU_STRING:
            this->fmi2String_start_buf = other.fmi2String_start_buf;
            this->ptr.fmi2String_ptr = other.ptr.fmi2String_ptr; //TODO: check
            this->start.fmi2String_start = this->fmi2String_start_buf.c_str();
            break;
        case FmuVariable::Type::FMU_UNKNOWN:
            throw std::runtime_error("Cannot set FmuVariable::type to FMU_UNKNOWN.");
            break;
        default:
            throw std::runtime_error("Developer error: passed a not registered FmuVariable::Type.");
            break;
        }

        allowed_start = other.allowed_start;
        required_start = other.required_start;
        has_start = other.has_start;

        return *this;
    }



    template <typename T>
    void SetPtr(T* ptr) {
        // DEV: the reinterpret cast is actually doing nothing since the T* should already be of the same type
        switch (this->type)
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
            throw std::runtime_error("Need to set FmuVariable::type before calling this command.");
            break;
        default:
            break;
        }
    }

    template <typename T>
    void GetPtr(T** ptr) const {
        // DEV: the reinterpret cast is actually doing nothing since the T* should already be of the same type
        switch (this->type)
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
            throw std::runtime_error("Need to set FmuVariable::type before calling this command.");
            break;
        default:
            break;
        }
    }

    template <typename T>
    void SetStartVal(T startval) {

        if (allowed_start)
            has_start = true;
        else
            return;

        // DEV: the reinterpret cast is actually doing nothing since the T* should already be of the same type
        switch (this->type)
        {
        case FmuVariable::Type::FMU_REAL:
            assert(typeid(this->start.fmi2Real_start) == typeid(startval));
            this->start.fmi2Real_start = static_cast<fmi2Real>(startval);
            break;
        case FmuVariable::Type::FMU_INTEGER:
            assert(typeid(this->start.fmi2Integer_start) == typeid(startval));
            this->start.fmi2Integer_start = static_cast<fmi2Integer>(startval);
            break;
        case FmuVariable::Type::FMU_BOOLEAN:
            assert(typeid(this->start.fmi2Boolean_start) == typeid(startval));
            this->start.fmi2Boolean_start = static_cast<fmi2Boolean>(startval);
            break;
        case FmuVariable::Type::FMU_STRING:
            assert(typeid(this->start.fmi2String_start) == typeid(startval));
            this->fmi2String_start_buf = std::string(*reinterpret_cast<fmi2String*>(&startval));
            this->start.fmi2String_start = fmi2String_start_buf.c_str();
            break;
        case FmuVariable::Type::FMU_UNKNOWN:
            throw std::runtime_error("Need to set FmuVariable::type before calling this command.");
            break;
        default:
            break;
        }
    }

    std::string GetStartVal() const {
        std::string ret;
        switch (this->type)
        {
        case FmuVariable::Type::FMU_REAL:
            ret = std::to_string(this->start.fmi2Real_start);
            return std::to_string(this->start.fmi2Real_start);
            break;
        case FmuVariable::Type::FMU_INTEGER:
            return std::to_string(this->start.fmi2Integer_start);
            break;
        case FmuVariable::Type::FMU_BOOLEAN:
            return std::to_string(this->start.fmi2Boolean_start);
            break;
        case FmuVariable::Type::FMU_STRING:
            return this->fmi2String_start_buf;
            break;
        case FmuVariable::Type::FMU_UNKNOWN:
            throw std::runtime_error("Need to set FmuVariable::type before calling this command.");
            break;
        default:
            throw std::runtime_error("Need to set FmuVariable::type before calling this command.");
            break;
        }
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

    union ptr_type{
        fmi2Real* fmi2Real_ptr;
        fmi2Integer* fmi2Integer_ptr;
        fmi2Boolean* fmi2Boolean_ptr;
        fmi2String* fmi2String_ptr;
    } ptr;

    union start_type{
        fmi2Real fmi2Real_start;
        fmi2Integer fmi2Integer_start;
        fmi2Boolean fmi2Boolean_start;
        fmi2String fmi2String_start;
    } start;

    std::string fmi2String_start_buf;

};

    
