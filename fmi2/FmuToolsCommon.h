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
/// The model description XML lists variables grouped by type, in increasing order of the Type enum values.
class FmuVariable {
  public:
    enum class Type { Real = 0, Integer = 1, Boolean = 2, String = 3, Unknown = 4 };

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
        // Readibility replacements
        bool c_parameter = (causality == CausalityType::parameter);
        bool c_calculated = (causality == CausalityType::calculatedParameter);
        bool c_input = (causality == CausalityType::input);
        bool c_output = (causality == CausalityType::output);
        bool c_local = (causality == CausalityType::local);
        bool c_independent = (causality == CausalityType::independent);

        bool v_constant = (variability == VariabilityType::constant);
        bool v_fixed = (variability == VariabilityType::fixed);
        bool v_tunable = (variability == VariabilityType::tunable);
        bool v_discrete = (variability == VariabilityType::discrete);
        bool v_continuous = (variability == VariabilityType::continuous);

        bool i_none = (initial == InitialType::none);
        bool i_exact = (initial == InitialType::exact);
        bool i_approx = (initial == InitialType::approx);
        bool i_calculated = (initial == InitialType::calculated);

        // Set "initial" property if empty (see Table on page 51 of the FMI2.0.4 specification)
        // (A)
        if (((v_constant) && (c_output || c_local)) || ((v_fixed || v_tunable) && (c_parameter))) {
            if (i_none)
                initial = InitialType::exact;
            else if (!i_exact)
                throw std::runtime_error("initial not set properly.");
        }
        // (B)
        else if ((v_fixed || v_tunable) && (c_calculated || c_local)) {
            if (i_none)
                initial = InitialType::calculated;
            else if (!i_approx && !i_calculated)
                throw std::runtime_error("initial not set properly.");
        }
        // (C)
        else if ((v_discrete || v_continuous) && (c_output || c_local)) {
            if (i_none)
                initial = InitialType::calculated;
        }

        // From page 51 of FMI2.0.4 specification:
        // (1) If causality = "independent", it is neither allowed to define a value for initial nor a value for start.
        // (2) If causality = "input", it is not allowed to define a value for initial and a value for start must be
        // defined.
        // (3) [not relevant] If (C) and initial = "exact", then the variable is explicitly defined by its start value
        // in Initialization Mode
        if (c_independent && !i_none)
            throw std::runtime_error(
                "If causality = 'independent', it is neither allowed to define a value for initial nor a value for "
                "start.");

        if (c_input && !i_none)
            throw std::runtime_error(
                "If causality = 'input', it is not allowed to define a value for initial and a value for start must be "
                "defined.");

        // Incompatible variability/causality settings (see Table on page 51 of the FMI2.0.4 specification)
        // (a)
        if (v_constant && (c_parameter || c_calculated || c_input))
            throw std::runtime_error(
                "constants always have their value already set, thus their causality can be only 'output' or 'local'");
        // (b)
        if ((v_discrete || v_continuous) && (c_parameter || c_calculated))
            throw std::runtime_error(
                "parameters and calculatedParameters cannot be discrete nor continuous, as they do not change over "
                "time.");
        // (c)
        if (c_independent && !v_continuous)
            throw std::runtime_error("For an 'independent' variable only variability = 'continuous' makes sense.");
        // (d) + (e)
        if (c_input && (v_fixed || v_tunable))
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
            case Type::Real:
                return "Real";
                break;
            case Type::Integer:
                return "Integer";
                break;
            case Type::Boolean:
                return "Boolean";
                break;
            case Type::String:
                return "String";
                break;
            case Type::Unknown:
                return "Unknown";
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
