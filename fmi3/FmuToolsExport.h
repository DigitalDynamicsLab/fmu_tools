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
// Classes for exporting FMUs (FMI 3.0)
// =============================================================================

#ifndef FMUTOOLS_FMI3_EXPORT_H
#define FMUTOOLS_FMI3_EXPORT_H

#include <algorithm>
#include <cassert>
#include <vector>
#include <array>
#include <map>
#include <iostream>
#include <string>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <functional>
#include <list>

#include "FmuToolsDefinitions.h"
#include "FmuToolsUnitDefinitions.h"
#include "fmi3/FmuToolsCommon.h"
#include "fmi3/fmi3_headers/fmi3Functions.h"

//// RADU TODO - do we want/need custom type variants?
#ifndef FMITYPESPLATFORM_CUSTOM
    #include "fmi3/TypesVariantsDefault.h"
#else
    #include "fmi3/TypesVariantsCustom.h"
#endif

// =============================================================================

namespace fmi3 {

#ifdef __cplusplus
extern "C" {
#endif

void FMI3_Export createModelDescription(const std::string& path, FmuType fmu_type);

#ifdef __cplusplus
}
#endif

// =============================================================================

#define MAKE_GETSET_PAIR(returnType, codeGet, codeSet)                          \
    std::make_pair(std::function<returnType()>([this]() -> returnType codeGet), \
                   std::function<void(returnType)>([this](returnType val) codeSet))

template <class T>
using FunGetSet = std::pair<std::function<T(void)>, std::function<void(T)>>;

bool is_pointer_variant(const FmuVariableBindType& myVariant);

// =============================================================================

/// Implementation of an FMU variable for export (generation of model description XML).
class FmuVariableExport : public FmuVariable {
  public:
    using VarbindType = FmuVariableBindType;
    using StartType = FmuVariableStartType;

    FmuVariableExport(const VarbindType& varbind,
                      const std::string& _name,
                      FmuVariable::Type _type,
                      CausalityType _causality = CausalityType::local,
                      VariabilityType _variability = VariabilityType::continuous,
                      InitialType _initial = InitialType::none);

    FmuVariableExport(const FmuVariableExport& other);

    /// Copy assignment operator.
    FmuVariableExport& operator=(const FmuVariableExport& other);

    void Bind(VarbindType newvarbind) { varbind = newvarbind; }

    /// Set the value of this FMU variable (for all cases, except variables of type fmi3String).
    template <typename T, typename = typename std::enable_if<!std::is_same<T, fmi3String>::value>::type>
    void SetValue(const T& val) const {
        if (is_pointer_variant(this->varbind))
            *varns::get<T*>(this->varbind) = val;
        else
            varns::get<FunGetSet<T>>(this->varbind).second(val);
    }

    /// Set the value of this FMU variable of type fmi3String.
    void SetValue(const fmi3String& val) const;

    /// Get the value of this FMU variable (for all cases, except variables of type fmi3String).
    template <typename T, typename = typename std::enable_if<!std::is_same<T, fmi3String>::value>::type>
    void GetValue(T* varptr) const {
        *varptr = is_pointer_variant(this->varbind) ? *varns::get<T*>(this->varbind)
                                                    : varns::get<FunGetSet<T>>(this->varbind).first();
    }

    /// Get the value of this FMU variable of type fmi3String.
    void GetValue(fmi3String* varptr) const;

    /// Set the start value for this FMU variable (for all cases, except variables of type fmi3String).
    template <typename T, typename = typename std::enable_if<!std::is_same<T, fmi3String>::value>::type>
    void SetStartVal(T startval) {
        if (!allowed_start)
            return;
        has_start = true;
        this->start = startval;
    }

    /// Set the start value for this FMU variable of type fmi3String.
    void SetStartVal(fmi3String startval);

    void ExposeCurrentValueAsStart();

  protected:
    bool allowed_start = true;
    bool required_start = false;

    VarbindType varbind;  // value of this variable
    StartType start;      // start value for this variable

    // TODO: in C++17 should be possible to either use constexpr or use lambda with 'overload' keyword
    template <typename T>
    void setStartFromVar(FunGetSet<T> funPair);

    template <typename T>
    void setStartFromVar(T* var_ptr);

    std::string GetStartVal_toString() const;

    friend class FmuComponentBase;
};

// -----------------------------------------------------------------------------

template <typename T>
void FmuVariableExport::setStartFromVar(FunGetSet<T> funPair) {
    if (allowed_start)
        has_start = true;
    else
        return;

    this->start = funPair.first();
}

template <typename T>
void FmuVariableExport::setStartFromVar(T* var_ptr) {
    if (allowed_start)
        has_start = true;
    else
        return;

    this->start = *var_ptr;
}

// =============================================================================

/// Base class for an FMU component (used for export).
/// (1) This class provides support for:
/// - defining FMU variables (cauality, variability, start value, etc)
/// - defining FMU model structure (outputs, derivatives, variable dependencies, etc)
/// (2) This class defines the virtual methods that an FMU must implement
class FmuComponentBase {
  public:
    FmuComponentBase(FmuType fmiInterfaceType,
                     fmi3String instanceName,
                     fmi3String instantiationToken,
                     fmi3String resourcePath,
                     fmi3Boolean visible,
                     fmi3Boolean loggingOn,
                     fmi3InstanceEnvironment instanceEnvironment,
                     fmi3LogMessageCallback logMessage,
                     const std::unordered_map<std::string, bool>& logCategories_init,
                     const std::unordered_set<std::string>& logCategories_debug_init);

    virtual ~FmuComponentBase() {}

    const std::set<FmuVariableExport>& GetVariables() const { return m_variables; }

    /// Enable/disable the logging for a specific log category.
    virtual void SetDebugLogging(std::string cat, bool val);

    /// Create the modelDescription.xml file in the given location \a path.
    void ExportModelDescription(std::string path);

    double GetTime() const { return m_time; }

    template <class T>
    fmi3Status fmi3GetVariable(const fmi3ValueReference vrs[],
                               size_t nvr,
                               T values[],
                               size_t nval,
                               FmuVariable::Type vartype) {
        //// RADU TODO
        return fmi3Status::fmi3OK;
    }

    template <class T>
    fmi3Status fmi3SetVariable(const fmi3ValueReference vrs[],
                               size_t nvr,
                               const T values[],
                               size_t nvals,
                               FmuVariable::Type vartype) {
        //// RADU TODO
        return fmi3Status::fmi3OK;
    }

    /// Adds a variable to the list of variables of the FMU.
    /// The start value is automatically grabbed from the variable itself.
    const FmuVariableExport& AddFmuVariable(
        const FmuVariableExport::VarbindType& varbind,
        std::string name,
        FmuVariable::Type scalartype = FmuVariable::Type::Float64,
        std::string unitname = "",
        std::string description = "",
        FmuVariable::CausalityType causality = FmuVariable::CausalityType::local,
        FmuVariable::VariabilityType variability = FmuVariable::VariabilityType::continuous,
        FmuVariable::InitialType initial = FmuVariable::InitialType::none);

    /// Declare a state derivative variables, specifying the corresponding state and dependencies on other variables.
    /// Calls to this function must be made *after* all FMU variables were defined.
    void DeclareStateDerivative(const std::string& derivative_name,
                                const std::string& state_name,
                                const std::vector<std::string>& dependency_names);

    /// Declare variable dependencies.
    /// Calls to this function must be made *after* all FMU variables were defined.
    void DeclareVariableDependencies(const std::string& variable_name,
                                     const std::vector<std::string>& dependency_names);

    bool RebindVariable(FmuVariableExport::VarbindType varbind, std::string name);

    /// Add a function to be executed before doStep (co-simulation FMU) or before getDerivatives (model exchange FMU).
    /// Such functions can be used to implement FMU-specific pre-processing of input variables.
    void AddPreStepFunction(std::function<void(void)> function) { m_preStepCallbacks.push_back(function); }

    /// Add a function to be executed after doStep (co-simulation FMU) or after getDerivatives (model exchange FMU).
    /// Such functions can be used to implement FMU-specific post-processing to prepare output variables.
    void AddPostStepFunction(std::function<void(void)> function) { m_postStepCallbacks.push_back(function); }

  protected:
    // This section declares the virtual methods that a concrete FMU must implement.
    // - some of these functions have a default implementation
    // - some are required for all FMUs (pure virtual)
    // - others are required only for specific FMU types (e.g., doStepIMPL must be implemented for CS, but not for ME)

    virtual bool is_cosimulation_available() const = 0;
    virtual bool is_modelexchange_available() const = 0;

    virtual void preModelDescriptionExport() {}
    virtual void postModelDescriptionExport() {}

    virtual fmi3Status updateDiscreteStatesIMPL(fmi3Boolean* discreteStatesNeedUpdate,
                                                fmi3Boolean* terminateSimulation,
                                                fmi3Boolean* nominalsOfContinuousStatesChanged,
                                                fmi3Boolean* valuesOfContinuousStatesChanged,
                                                fmi3Boolean* nextEventTimeDefined,
                                                fmi3Float64* nextEventTime) {
        discreteStatesNeedUpdate = fmi3False;
        terminateSimulation = fmi3False;
        nominalsOfContinuousStatesChanged = fmi3False;
        valuesOfContinuousStatesChanged = fmi3False;
        nextEventTimeDefined = fmi3False;
        nextEventTime = 0;

        return fmi3Status::fmi3OK;
    }

    virtual fmi3Status doStepIMPL(fmi3Float64 currentCommunicationPoint,
                                  fmi3Float64 communicationStepSize,
                                  fmi3Boolean noSetFMUStatePriorToCurrentPoint,
                                  fmi3Boolean* eventHandlingNeeded,
                                  fmi3Boolean* terminateSimulation,
                                  fmi3Boolean* earlyReturn,
                                  fmi3Float64* lastSuccessfulTime) {
        if (is_cosimulation_available())
            throw std::runtime_error("An FMU for co-simulation must implement doStepIMPL");

        return fmi3OK;
    }

    virtual fmi3Status completedIntegratorStepIMPL(fmi3Boolean noSetFMUStatePriorToCurrentPoint,
                                                   fmi3Boolean* enterEventMode,
                                                   fmi3Boolean* terminateSimulation) {
        *enterEventMode = fmi3False;
        *terminateSimulation = fmi3False;

        return fmi3Status::fmi3OK;
    }

    virtual fmi3Status setTimeIMPL(fmi3Float64 time) { return fmi3Status::fmi3OK; }

    virtual fmi3Status getContinuousStatesIMPL(fmi3Float64 continuousStates[], size_t nContinuousStates) {
        if (is_modelexchange_available())
            throw std::runtime_error("An FMU for model exchange must implement getContinuousStatesIMPL");

        return fmi3Status::fmi3OK;
    }

    virtual fmi3Status setContinuousStatesIMPL(const fmi3Float64 continuousStates[], size_t nContinuousStates) {
        if (is_modelexchange_available())
            throw std::runtime_error("An FMU for model exchange must implement setContinuousStatesIMPL");

        return fmi3Status::fmi3OK;
    }

    virtual fmi3Status getDerivativesIMPL(fmi3Float64 derivatives[], size_t nContinuousStates) {
        if (is_modelexchange_available())
            throw std::runtime_error("An FMU for model exchange must implement getDerivativesIMPL");

        return fmi3Status::fmi3OK;
    }

    virtual fmi3Status enterInitializationModeIMPL() { return fmi3Status::fmi3OK; }
    virtual fmi3Status exitInitializationModeIMPL() { return fmi3Status::fmi3OK; }

  public:
    void SetIntermediateUpdateCallback(fmi3IntermediateUpdateCallback intermediateUpdate) {
        m_intermediateUpdate = intermediateUpdate;
    }

    fmi3Status EnterInitializationMode(fmi3Boolean toleranceDefined,
                                       fmi3Float64 tolerance,
                                       fmi3Float64 startTime,
                                       fmi3Boolean stopTimeDefined,
                                       fmi3Float64 stopTime);

    fmi3Status ExitInitializationMode();

    // Common FMI function.
    // These functions are used to implement the actual common functions imposed by the FMI3 standard.
    // In turn, they call overrides of virtual methods provided by a concrete FMU.
    fmi3Status UpdateDiscreteStates(fmi3Boolean* discreteStatesNeedUpdate,
                                    fmi3Boolean* terminateSimulation,
                                    fmi3Boolean* nominalsOfContinuousStatesChanged,
                                    fmi3Boolean* valuesOfContinuousStatesChanged,
                                    fmi3Boolean* nextEventTimeDefined,
                                    fmi3Float64* nextEventTime);

    // Co-Simulation FMI functions.
    // These functions are used to implement the actual co-simulation functions imposed by the FMI3 standard.
    // In turn, they call overrides of virtual methods provided by a concrete FMU.

    fmi3Status DoStep(fmi3Float64 currentCommunicationPoint,
                      fmi3Float64 communicationStepSize,
                      fmi3Boolean noSetFMUStatePriorToCurrentPoint,
                      fmi3Boolean* eventHandlingNeeded,
                      fmi3Boolean* terminateSimulation,
                      fmi3Boolean* earlyReturn,
                      fmi3Float64* lastSuccessfulTime);

    // Model Exchange FMI functions.
    // These functions are used to implement the actual model exchange functions imposed by the FMI3 standard.
    // In turn, they call overrides of virtual methods provided by a concrete FMU.

    fmi3Status CompletedIntegratorStep(fmi3Boolean noSetFMUStatePriorToCurrentPoint,
                                       fmi3Boolean* enterEventMode,
                                       fmi3Boolean* terminateSimulation);
    fmi3Status SetTime(const fmi3Float64 time);
    fmi3Status GetContinuousStates(fmi3Float64 continuousStates[], size_t nContinuousStates);
    fmi3Status SetContinuousStates(const fmi3Float64 continuousStates[], size_t nContinuousStates);
    fmi3Status GetDerivatives(fmi3Float64 derivatives[], size_t nContinuousStates);

  protected:
    /// Add a declaration of a state derivative.
    virtual void addDerivative(const std::string& derivative_name,
                               const std::string& state_name,
                               const std::vector<std::string>& dependency_names);

    /// Check if the variable with specified name is a state derivative.
    /// If true, return the name of the corresponding state variable. Otherwise, return an empty string.
    std::string isDerivative(const std::string& name);

    /// Add a declaration of dependency of "variable_name" on the variables in the "dependency_names" list.
    virtual void addDependencies(const std::string& variable_name, const std::vector<std::string>& dependency_names);

    void initializeType(FmuType fmuType);

    void addUnitDefinition(const UnitDefinition& unit_definition);

    void clearUnitDefinitions() { m_unitDefinitions.clear(); }

    /// Send message to the logger function.
    /// The message will be sent if at least one of the following applies:
    /// - a msg_cat was enabled by `SetDebugLogging(msg_cat, true)`
    /// - the FMU was instantiated with `loggingOn=fmi3True` and a msg_cat has been labelled as a debugging category;
    /// FMUs generated by this library provides a Description which reports if the category is debug.
    void sendToLog(std::string msg, fmi3Status status, std::string msg_cat);

    std::set<FmuVariableExport>::iterator findByValref(fmi3ValueReference vr);
    std::set<FmuVariableExport>::iterator findByName(const std::string& name);

    void executePreStepCallbacks();
    void executePostStepCallbacks();

    std::string m_instanceName;
    std::string m_instantiationToken;
    std::string m_resources_location;

    bool m_visible;

    const std::unordered_set<std::string> m_logCategories_debug;  ///< list of log categories considered to be of debug
    bool m_debug_logging_enabled = true;                          ///< enable logging?

    // DefaultExperiment
    fmi3Float64 m_startTime = 0;
    fmi3Float64 m_stopTime = 1;
    fmi3Float64 m_tolerance = -1;
    fmi3Boolean m_toleranceDefined = fmi3False;
    fmi3Boolean m_stopTimeDefined = fmi3False;

    fmi3Float64 m_stepSize = 1e-3;
    fmi3Float64 m_time = 0;

    const std::string m_modelIdentifier;

    FmuType m_fmuType;

    unsigned int m_valrefCounter = 0;

    std::set<FmuVariableExport> m_variables;
    std::unordered_map<std::string, UnitDefinition> m_unitDefinitions;
    std::unordered_map<std::string, std::pair<std::string, std::vector<std::string>>> m_derivatives;
    std::unordered_map<std::string, std::vector<std::string>> m_variableDependencies;

    std::list<std::function<void(void)>> m_preStepCallbacks;
    std::list<std::function<void(void)>> m_postStepCallbacks;

    fmi3InstanceEnvironment m_instanceEnvironment;
    fmi3LogMessageCallback m_logMessage;
    fmi3IntermediateUpdateCallback m_intermediateUpdate;
    // The following callbacks are used only for ScheduledExecution
    ////fmi3LockPreemptionCallback m_lockPreemption;
    ////fmi3UnlockPreemptionCallback m_unlockPreemption;

    FmuMachineState m_fmuMachineState;

    std::unordered_map<std::string, bool> m_logCategories_enabled;
};

// -----------------------------------------------------------------------------

/// Function to create an instance of a particular FMU for the specified FMI interface.
/// This function must be implemented by a concrete FMU and should return a new object of the concrete FMU type.
/// If instantiation failed (e.g., because the FMU does not support the requested interface), the FMU must call
/// logMessage with detailed information about the reason.
FmuComponentBase* fmi3InstantiateIMPL(FmuType fmiInterfaceType,
                                      fmi3String instanceName,
                                      fmi3String instantiationToken,
                                      fmi3String resourcePath,
                                      fmi3Boolean visible,
                                      fmi3Boolean loggingOn,
                                      fmi3InstanceEnvironment instanceEnvironment,
                                      fmi3LogMessageCallback logMessage);

}  // namespace fmi3

#endif
