// =============================================================================
// fmu_tools
//
// Copyright (c) 2024 Project Chrono (projectchrono.org)
// Copyright (c) 2024 Digital Dynamics Lab, University of Parma, Italy
// Copyright (c) 2024 Simulation Based Engineering Lab, University of Wisconsin-Madison, USA
// All rights reserved.
//
// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file at the top level of the distribution.
//
// =============================================================================
// Definition of the FMU variable base class and common utilities (FMI 2.0)
// =============================================================================

#ifndef FMUTOOLS_COMMON_H
#define FMUTOOLS_COMMON_H

#include <stdexcept>
#include <string>
#include <cstdarg>
#include <typeindex>
#include <unordered_map>
#include <cassert>

#include "FmuToolsDefinitions.h"

#include "fmi2/fmi2_headers/fmi2FunctionTypes.h"
#include "fmi2/fmi2_headers/fmi2Functions.h"

namespace fmi2 {

struct LoggingUtilities {
    static std::string fmi2Status_toString(fmi2Status status) {
        switch (status) {
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
                throw std::runtime_error("Wrong fmi2Status");
                break;
        }
    }

    static void logger_default(fmi2ComponentEnvironment c,
                               fmi2String instanceName,
                               fmi2Status status,
                               fmi2String category,
                               fmi2String message,
                               ...) {
        char msg[2024];
        va_list argp;
        va_start(argp, message);
        vsprintf(msg, message, argp);
        if (!instanceName)
            instanceName = "?";
        if (!category)
            category = "?";
        printf("[%s|%s] %s: %s", instanceName, fmi2Status_toString(status).c_str(), category, msg);
    }
};

// =============================================================================

/// Implementation of an FMU variable.
/// Objects of this type are created during:
/// - FMU export (and encoded in the model description XML)
/// - FMU import (retrieved from the model description XML)
class FmuVariable {
  public:
    enum class Type {
        Real = 0,  // numbering gives the order in which each type is printed in the modelDescription.xml
        Integer = 1,
        Boolean = 2,
        String = 3,
        Unknown = 4
    };

    enum class CausalityType { parameter, calculatedParameter, input, output, local, independent };

    enum class VariabilityType { constant, fixed, tunable, discrete, continuous };

    enum class InitialType { none, exact, approx, calculated };

    FmuVariable() : FmuVariable("", FmuVariable::Type::Real) {}

    FmuVariable(const std::string& _name,
                FmuVariable::Type _type,
                CausalityType _causality = CausalityType::local,
                VariabilityType _variability = VariabilityType::continuous,
                InitialType _initial = InitialType::none)
        : name(_name),
          valueReference(0),
          unitname("1"),
          type(_type),
          causality(_causality),
          variability(_variability),
          initial(_initial),
          description(""),
          has_start(false) {
        // set default value for "initial" if empty: reference FMIReference 2.0.2 - Section 2.2.7 Definition of Model
        // Variables

        // (A)
        if ((variability == VariabilityType::constant &&
             (causality == CausalityType::output || causality == CausalityType::local)) ||
            (variability == VariabilityType::fixed || variability == VariabilityType::tunable) &&
                causality == CausalityType::parameter) {
            if (initial == InitialType::none)
                initial = InitialType::exact;
            else if (initial != InitialType::exact)
                throw std::runtime_error("initial not set properly.");
        }
        // (B)
        else if ((variability == VariabilityType::fixed || variability == VariabilityType::tunable) &&
                 (causality == CausalityType::calculatedParameter || causality == CausalityType::local)) {
            if (initial == InitialType::none)
                initial = InitialType::calculated;
            else if (initial != InitialType::approx && initial != InitialType::calculated)
                throw std::runtime_error("initial not set properly.");
        }
        // (C)
        else if ((variability == VariabilityType::discrete || variability == VariabilityType::continuous) &&
                 (causality == CausalityType::output || causality == CausalityType::local)) {
            if (initial == InitialType::none)
                initial = InitialType::calculated;
        }

        // From FMI Reference
        // (1) If causality = "independent", it is neither allowed to define a value for initial nor a value for start.
        // (2) If causality = "input", it is not allowed to define a value for initial and a value for start must be
        // defined. (3) [not relevant] If (C) and initial = "exact", then the variable is explicitly defined by its
        // start value in Initialization Mode
        if (causality == CausalityType::independent && initial != InitialType::none)
            throw std::runtime_error(
                "If causality = 'independent', it is neither allowed to define a value for initial nor a value for "
                "start.");

        if (causality == CausalityType::input && initial != InitialType::none)
            throw std::runtime_error(
                "If causality = 'input', it is not allowed to define a value for initial and a value for start must be "
                "defined.");

        // Incompatible variability/causality settings
        // (a)
        if (variability == VariabilityType::constant &&
            (causality == CausalityType::parameter || causality == CausalityType::calculatedParameter ||
             causality == CausalityType::input))
            throw std::runtime_error(
                "constants always have their value already set, thus their causality can be only 'output' or 'local'");
        // (b)
        if ((variability == VariabilityType::discrete || variability == VariabilityType::continuous) &&
            (causality == CausalityType::parameter || causality == CausalityType::calculatedParameter))
            throw std::runtime_error(
                "parameters and calculatedParameters cannot be discrete nor continuous, since the latters mean that "
                "they could change over time");
        // (c)
        if (causality == CausalityType::independent && variability != VariabilityType::continuous)
            throw std::runtime_error("For an 'independent' variable only variability = 'continuous' makes sense.");
        // (d) + (e)
        if (causality == CausalityType::input &&
            (variability == VariabilityType::fixed || variability == VariabilityType::tunable))
            throw std::runtime_error(
                "A fixed or tunable 'input'|'output' have exactly the same properties as a fixed or tunable parameter. "
                "For simplicity, only fixed and tunable parameters|calculatedParameters shall be defined.");
    }

    FmuVariable(const FmuVariable& other) {
        type = other.type;
        name = other.name;
        valueReference = other.valueReference;
        unitname = other.unitname;
        causality = other.causality;
        variability = other.variability;
        initial = other.initial;
        description = other.description;
        has_start = other.has_start;
    }

    // Copy assignment operator
    FmuVariable& operator=(const FmuVariable& other) {
        if (this == &other) {
            return *this;  // Self-assignment guard
        }

        type = other.type;
        name = other.name;
        valueReference = other.valueReference;
        unitname = other.unitname;
        causality = other.causality;
        variability = other.variability;
        initial = other.initial;
        description = other.description;
        has_start = other.has_start;

        return *this;
    }

    virtual ~FmuVariable() {}

    bool operator<(const FmuVariable& other) const {
        return this->type != other.type ? this->type < other.type : this->valueReference < other.valueReference;
    }

    bool operator==(const FmuVariable& other) const {
        // according to FMI Reference can exist two different variables with same type and same valueReference;
        // they are called "alias" thus they should be allowed but not considered equal
        return this->name == other.name;
    }

    /// Return true if a start value is specified for this variable.
    bool HasStartVal() const { return has_start; }

    /// Check if setting this variable is allowed, given the FMU type and current FMU state.
    bool IsSetAllowed(fmi2Type fmi_type, FmuMachineState fmu_machine_state) const {
        if (variability != VariabilityType::constant) {
            if (initial == InitialType::approx)
                return fmu_machine_state == FmuMachineState::instantiated ||
                       fmu_machine_state == FmuMachineState::anySettableState;
            else if (initial == InitialType::exact)
                return fmu_machine_state == FmuMachineState::instantiated ||
                       fmu_machine_state == FmuMachineState::initializationMode ||
                       fmu_machine_state == FmuMachineState::anySettableState;
        }

        if (causality == CausalityType::input ||
            (causality == CausalityType::parameter && variability == VariabilityType::tunable))
            return fmu_machine_state == FmuMachineState::initializationMode ||
                   fmu_machine_state == FmuMachineState::stepCompleted ||
                   fmu_machine_state == FmuMachineState::anySettableState;

        return false;
    }

    /// Return a string with the name of the specified FMU variable type.
    static std::string Type_toString(Type type) {
        switch (type) {
            case FmuVariable::Type::Real:
                return "Real";
                break;
            case FmuVariable::Type::Integer:
                return "Integer";
                break;
            case FmuVariable::Type::Boolean:
                return "Boolean";
                break;
            case FmuVariable::Type::Unknown:
                return "Unknown";
                break;
            case FmuVariable::Type::String:
                return "String";
                break;
            default:
                throw std::runtime_error("Type_toString: received bad type.");

                break;
        }
        return "";
    }

    const inline std::string& GetName() const { return name; }
    inline CausalityType GetCausality() const { return causality; }
    inline VariabilityType GetVariability() const { return variability; }
    inline InitialType GetInitial() const { return initial; }
    const inline std::string& GetDescription() const { return description; }
    void SetDescription(const std::string& _description) { description = _description; }
    const inline fmi2ValueReference GetValueReference() const { return valueReference; }
    void SetValueReference(fmi2ValueReference valref) { valueReference = valref; }
    const inline std::string& GetUnitName() const { return unitname; }
    void SetUnitName(const std::string& _unitname) { unitname = _unitname; }
    Type GetType() const { return type; }

  protected:
    Type type = Type::Unknown;          // variable type
    std::string name;                   // variable name
    fmi2ValueReference valueReference;  // reference among variables of same type
    std::string unitname;               // variable units
    CausalityType causality;            // variable causality
    VariabilityType variability;        // variable variability
    InitialType initial;                // type of initial value
    std::string description;            // description of this variable

    bool has_start;  // start value provided
};

}  // namespace fmi2

#endif
