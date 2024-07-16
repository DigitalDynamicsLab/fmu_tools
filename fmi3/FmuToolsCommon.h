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
// Definition of the FMU variable base class and common utilities (FMI 3.0)
// =============================================================================

#ifndef FMUTOOLS_FMI3_COMMON_H
#define FMUTOOLS_FMI3_COMMON_H

#include <stdexcept>
#include <string>
#include <cstdarg>
#include <typeindex>
#include <unordered_map>
#include <cassert>

#include "FmuToolsDefinitions.h"

#include "fmi3/fmi3_headers/fmi3FunctionTypes.h"
#include "fmi3/fmi3_headers/fmi3Functions.h"

namespace fmi3 {

struct LoggingUtilities {
    static std::string fmi3Status_toString(fmi3Status status) {
        switch (status) {
            case fmi3Status::fmi3Discard:
                return "Discard";
                break;
            case fmi3Status::fmi3Error:
                return "Error";
                break;
            case fmi3Status::fmi3Fatal:
                return "Fatal";
                break;
            case fmi3Status::fmi3OK:
                return "OK";
                break;
            case fmi3Status::fmi3Warning:
                return "Warning";
                break;
            default:
                throw std::runtime_error("Wrong fmi3Status");
                break;
        }
    }

    static void logger_default(fmi3InstanceEnvironment c, fmi3Status status, fmi3String category, fmi3String message) {
        if (!category)
            category = "?";
        printf("[%s] %s: %s", fmi3Status_toString(status).c_str(), category, message);
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
    enum class Type {
        Float32 = 0,
        Float64 = 1,
        Int8 = 2,
        UInt8 = 3,
        Int16 = 4,
        UInt16 = 5,
        Int32 = 6,
        UInt32 = 7,
        Int64 = 8,
        UInt64 = 9,
        Boolean = 10,
        String = 11,
        Binary = 12,
        Unknown = 13
    };

    using DimensionsArrayType = std::vector<std::pair<std::uint64_t, bool>>;

    enum class CausalityType { structuralParameter, parameter, calculatedParameter, input, output, local, independent };

    enum class VariabilityType { constant, fixed, tunable, discrete, continuous };

    enum class InitialType { none, exact, approx, calculated };

    FmuVariable() : FmuVariable("", FmuVariable::Type::Float64) {}

    FmuVariable(const std::string& _name,
                FmuVariable::Type _type,
                const DimensionsArrayType& _dimensions = DimensionsArrayType(),
                CausalityType _causality = CausalityType::local,
                VariabilityType _variability = VariabilityType::continuous,
                InitialType _initial = InitialType::none)
        : name(_name),
          valueReference(0),
          unitname("1"),
          type(_type),
          m_dimensions(_dimensions),
          causality(_causality),
          variability(_variability),
          initial(_initial),
          description(""),
          has_start(false) {
        // Readibility replacements
        bool c_structural = (causality == CausalityType::structuralParameter);
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

        // Set "initial" property if empty (see Table 22 in FMI3.0 specification)
        // (A)
        if (                                                              //
            ((v_constant) && (c_output || c_local)) ||                    //
            ((v_fixed || v_tunable) && (c_structural || c_parameter)) ||  //
            ((v_discrete || v_continuous) && (c_input))                   //
        ) {
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

        // From FMI Reference
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

        // Incompatible variability/causality settings (see Tables 18 and 19 of the FMI3.0 specification)
        // (a)
        if (v_constant && (c_structural || c_parameter || c_calculated || c_input))
            throw std::runtime_error(
                "constants always have their value already set, thus their causality can be only 'output' or 'local'.");
        // (b)
        if ((v_discrete || v_continuous) && (c_structural || c_parameter || c_calculated))
            throw std::runtime_error(
                "strcturalParameters, parameters and calculatedParameters cannot be discrete nor continuous, as they "
                "do not change over time.");
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
        m_dimensions = other.m_dimensions;
    }

    // Copy assignment operator
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
        m_dimensions = other.m_dimensions;

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

    /// Check if setting this variable is allowed given the current FMU state.
    bool IsSetAllowed(FmuMachineState fmu_machine_state) const {
        //// RADU TODO: additional checks for configurationMode state?

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
    /// Return a string with the name of the specified FMU variable type.
    static std::string Type_toString(Type type) {
        switch (type) {
            case Type::Float32:
                return "Float32";
                break;
            case Type::Float64:
                return "Float64";
                break;
            case Type::Int8:
                return "Int8";
                break;
            case Type::Int16:
                return "Int16";
                break;
            case Type::Int32:
                return "Int32";
                break;
            case Type::Int64:
                return "Int64";
                break;
            case Type::UInt8:
                return "UInt8";
                break;
            case Type::UInt16:
                return "UInt16";
                break;
            case Type::UInt32:
                return "UInt32";
                break;
            case Type::UInt64:
                return "UInt64";
                break;
            case Type::Boolean:
                return "Boolean";
                break;
            case Type::String:
                return "String";
                break;
            case Type::Binary:
                return "Unknown";
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
    const inline fmi3ValueReference GetValueReference() const { return valueReference; }
    void SetValueReference(fmi3ValueReference valref) { valueReference = valref; }
    const inline std::string& GetUnitName() const { return unitname; }
    void SetUnitName(const std::string& _unitname) { unitname = _unitname; }
    Type GetType() const { return type; }

    DimensionsArrayType GetDimensions() const { return m_dimensions; }
    bool IsScalar() const { return m_dimensions.empty(); }

    /// Try to retrieve the size of the variable.
    /// Returns true if the size can be retrieved (i.e. the dimensions are fixed and not given by other variables),
    /// false otherwise.
    bool GetSize(size_t& size) const {
        if (m_dimensions.empty()) {
            size = 1;
            return true;
        }

        size = 1;
        for (const auto& dim : m_dimensions) {
            if (dim.second) {
                size *= dim.first;
            } else {
                return false;
            }
        }

        return size;
    }

  protected:
    Type type = Type::Unknown;          // variable type
    std::string name;                   // variable name
    fmi3ValueReference valueReference;  // reference among variables of same type
    std::string unitname;               // variable units
    CausalityType causality;            // variable causality
    VariabilityType variability;        // variable variability
    InitialType initial;                // type of initial value
    std::string description;            // description of this variable

    /// list of pairs (size, fixed) for each dimension;
    /// - if m_dimensions[i].second == true (i.e. labelled as 'fixed') then 'size' provides the actual size for that
    /// dimension
    /// - if m_dimensions[i].second == false (i.e. labelled as not 'fixed') then 'size' provides an fmi3ValueReference
    /// to another variable that will provides the size of this variable
    mutable DimensionsArrayType m_dimensions;

    bool has_start;  // start value provided
};

}  // namespace fmi3

#endif
