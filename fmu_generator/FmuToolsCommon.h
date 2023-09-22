#pragma once
#include "fmi2_headers/fmi2FunctionTypes.h"
#include "fmi2_headers/fmi2Functions.h"
#include <stdexcept>
#include <string>
#include <typeindex>
#include <unordered_map>
#include <cassert>


#include "variant/variant_guard.hpp"

#ifndef FMITYPESPLATFORM_CUSTOM
    #include "TypesVariantsDefault.h"
#else
    #include "TypesVariantsCustom.h"
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


enum class FmuMachineStateType{
    anySettableState, // custom element, used to do checks
    instantiated,
    initializationMode,
    stepCompleted,  // only CoSimulation
    stepInProgress,  // only CoSimulation
    stepFailed,  // only CoSimulation
    stepCanceled,  // only CoSimulation
    terminated,
    error,
    fatal,
    eventMode, // only ModelExchange
    continuousTimeMode // only ModelExchange
};


/// Class holding a reference to a single FMU variable
/// Note that this is retrieved from the information from the XML

class FmuVariable {
public:


    using PtrType = FmuVariablePtrType;
    using StartType = FmuVariableStartType;

    enum class Type{
        FMU_REAL = 0, // numbering gives the order in which each type is printed in the modelDescription.xml
        FMU_INTEGER = 1,
        FMU_BOOLEAN = 2,
        FMU_STRING = 3,
        FMU_UNKNOWN = 4
    };

    enum class CausalityType{
        parameter,
        calculatedParameter,
        input,
        output,
        local,
        independent
    };

    enum class VariabilityType{
        constant,
        fixed,
        tunable,
        discrete,
        continuous
    };

    enum class InitialType{
        none,
        exact,
        approx,
        calculated
    };
    // TODO: DARIOM check if an additional InitialType::auto is needed


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
        ptr = other.ptr;
        start = other.start;
        allowed_start = other.allowed_start;
        required_start = other.required_start;
        has_start = other.has_start;

    }

    FmuVariable(
        const std::string& _name,
        FmuVariable::Type _type,
        CausalityType _causality = CausalityType::local,
        VariabilityType _variability = VariabilityType::continuous,
        InitialType _initial = InitialType::none):
        name(_name),
        type(_type),
        causality(_causality),
        variability(_variability),
        initial(_initial)
    {
        // set default value for "initial" if empty: reference FMIReference 2.0.2 - Section 2.2.7 Definition of Model Variables

        // (A)
        if((variability == VariabilityType::constant && (causality == CausalityType::output || causality == CausalityType::local)) ||
            (variability == VariabilityType::fixed || variability == VariabilityType::tunable) && causality == CausalityType::parameter){
            if (initial == InitialType::none)
                initial = InitialType::exact;
            else
                if (initial != InitialType::exact)
                    throw std::runtime_error("initial not set properly.");
        }
        // (B)
        else if((variability == VariabilityType::fixed || variability == VariabilityType::tunable) && (causality == CausalityType::calculatedParameter || causality == CausalityType::local)){
            if (initial == InitialType::none)
                initial = InitialType::calculated;
            else
                if (initial != InitialType::approx && initial != InitialType::calculated)
                    throw std::runtime_error("initial not set properly.");
        }
        // (C)
        else if((variability == VariabilityType::discrete || variability == VariabilityType::continuous) && (causality == CausalityType::output || causality == CausalityType::local)){
            if (initial == InitialType::none)
                initial = InitialType::calculated;
        }

        // From FMI Reference
        // (1) If causality = "independent", it is neither allowed to define a value for initial nor a value for start.
        // (2) If causality = "input", it is not allowed to define a value for initial and a value for start must be defined.
        // (3) [not relevant] If (C) and initial = "exact", then the variable is explicitly defined by its start value in Initialization Mode
        if (causality == CausalityType::independent && initial != InitialType::none)
            throw std::runtime_error("If causality = 'independent', it is neither allowed to define a value for initial nor a value for start.");

        if (causality == CausalityType::input && initial != InitialType::none)
            throw std::runtime_error("If causality = 'input', it is not allowed to define a value for initial and a value for start must be defined.");

        // From FMI Reference
        // If initial = 'exact' or 'approx', or causality = 'input',       a start value MUST be provided.
        // If initial = 'calculated',        or causality = 'independent', a start value CANNOT be provided. 
        if (initial == InitialType::calculated || causality == CausalityType::independent){
            allowed_start = false;
            required_start = false;
        }

        if (initial == InitialType::exact || initial == InitialType::approx || causality == CausalityType::input){
            allowed_start = true;
            required_start = true;
        }


        // Incompatible variability/causality settings
        // (a)
        if (variability == VariabilityType::constant && (causality == CausalityType::parameter || causality == CausalityType::calculatedParameter || causality == CausalityType::input))
            throw std::runtime_error("constants always have their value already set, thus their causality can be only 'output' or 'local'");
        // (b)
        if ((variability == VariabilityType::discrete || variability == VariabilityType::continuous) && (causality == CausalityType::parameter || causality == CausalityType::calculatedParameter))
            throw std::runtime_error("parameters and calculatedParameters cannot be discrete nor continuous, since the latters mean that they could change over time");
        // (c)
        if (causality == CausalityType::independent && variability != VariabilityType::continuous)
            throw std::runtime_error("For an 'independent' variable only variability = 'continuous' makes sense.");
        // (d) + (e)
        if (causality == CausalityType::input && (variability == VariabilityType::fixed || variability == VariabilityType::tunable))
            throw std::runtime_error("A fixed or tunable 'input'|'output' have exactly the same properties as a fixed or tunable parameter. For simplicity, only fixed and tunable parameters|calculatedParameters shall be defined.");


    }


    bool operator<(const FmuVariable& other) const {
        return this->type != other.type ? this->type < other.type : this->valueReference < other.valueReference;
    }

    bool operator==(const FmuVariable& other) const {
        // according to FMI Reference can exist two different variables with same type and same valueReference;
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
        ptr = other.ptr;
        start = other.start;
        allowed_start = other.allowed_start;
        required_start = other.required_start;
        has_start = other.has_start;

        return *this;
    }

    void SetPtr(const PtrType& ptr) {
        this->ptr = ptr;
    }

    template <class T>
    void GetPtr(T** ptr_address) const {
        *ptr_address = varns::get<T*>(this->ptr);
    }


    template <typename T>
    void GetValue(T* varptr) const {
        *varptr = varns::holds_alternative<std::function<T(void)>>(this->ptr) ? varns::get<std::function<T(void)>>(this->ptr)() : *varns::get<T*>(this->ptr);
    }


    template <typename T, typename = typename std::enable_if<!std::is_same<T, fmi2String>::value>::type>
    void SetStartVal(T startval){
        if (allowed_start)
            has_start = true;
        else
            return;
        this->start = startval;
    }

    void SetStartVal(fmi2String startval){
        if (allowed_start)
            has_start = true;
        else
            return;
        this->start = std::string(startval);
    }


    bool HasStartVal() const {
        return has_start;
    }

    template <typename T>
    void SetStartValIfRequired(T* startval){
        if (required_start)
            SetStartVal(*startval);
    }

    template <typename T>
    void SetStartValIfRequired(std::function<T(void)> startval){
        if (required_start)
            throw std::runtime_error("Developer error: there shouldn't be a required_start on a function.");
    }


    std::string GetStartVal_toString() const {
        if (const fmi2Real* start_ptr = varns::get_if<fmi2Real>(&this->start))
            return std::to_string(*start_ptr);
        if (const fmi2Integer* start_ptr = varns::get_if<fmi2Integer>(&this->start))
            return std::to_string(*start_ptr);
        if (const fmi2Boolean* start_ptr = varns::get_if<fmi2Boolean>(&this->start))
            return std::to_string(*start_ptr);
        if (const std::string* start_ptr = varns::get_if<std::string>(&this->start))
            return *start_ptr;
        return "";
    }

    bool IsSetAllowed(fmi2Type fmi_type, FmuMachineStateType fmu_machine_state) const {

        if (fmi_type == fmi2Type::fmi2CoSimulation){

            if (variability != VariabilityType::constant){
                if (initial == InitialType::approx)
                    return fmu_machine_state == FmuMachineStateType::instantiated  || fmu_machine_state == FmuMachineStateType::anySettableState;
                else if (initial == InitialType::exact)
                    return fmu_machine_state == FmuMachineStateType::instantiated || fmu_machine_state == FmuMachineStateType::initializationMode || fmu_machine_state == FmuMachineStateType::anySettableState;
            }

            if (causality == CausalityType::input || (causality == CausalityType::parameter && variability == VariabilityType::tunable))
                return fmu_machine_state == FmuMachineStateType::initializationMode || fmu_machine_state == FmuMachineStateType::stepCompleted || fmu_machine_state == FmuMachineStateType::anySettableState;

            return false;
        }
        else
        {
            // TODO for ModelExchange
        }

        return false;
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
    inline CausalityType GetCausality() const { return causality;}
    inline VariabilityType GetVariability() const { return variability;}
    inline InitialType GetInitial() const { return initial;}
    const inline std::string& GetDescription() const { return description;}
    void SetDescription(const std::string& _description) { description = _description;}
    const inline fmi2ValueReference GetValueReference() const { return valueReference;}
    void SetValueReference(fmi2ValueReference valref) { valueReference = valref;}
    const inline std::string& GetUnitName() const { return unitname;}
    void SetUnitName(const std::string& _unitname) { unitname = _unitname;}
    Type GetType() const { return type;}



protected:
    Type type = Type::FMU_UNKNOWN;
    std::string name;
    fmi2ValueReference valueReference = 0;
    std::string unitname = "1";
    CausalityType causality;
    VariabilityType variability;
    InitialType initial;
    std::string description = "";




    bool allowed_start = true;
    bool required_start = false;
    bool has_start = false;


    PtrType ptr;
    StartType start;

};

    
