#ifndef FMUTOOLS_EXPORT_H
#define FMUTOOLS_EXPORT_H

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
#include "fmi2/FmuToolsCommon.h"
#include "fmi2/fmi2_headers/fmi2Functions.h"

#ifndef FMITYPESPLATFORM_CUSTOM
    #include "fmi2/TypesVariantsDefault.h"
#else
    #include "fmi2/TypesVariantsCustom.h"
#endif

// =============================================================================

#ifdef __cplusplus
extern "C" {
#endif

void FMI2_Export createModelDescription(const std::string& path, FmuType fmu_type);

#ifdef __cplusplus
}
#endif

// =============================================================================

struct UnitDefinitionType {
    UnitDefinitionType(const std::string& _name = "1") : name(_name) {}

    UnitDefinitionType(const std::string& _name, int _kg, int _m, int _s, int _A, int _K, int _mol, int _cd, int _rad)
        : name(_name), kg(_kg), m(_m), s(_s), A(_A), K(_K), mol(_mol), cd(_cd), rad(_rad) {}

    virtual ~UnitDefinitionType() {}

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
        size_t operator()(const UnitDefinitionType& p) const { return std::hash<std::string>()(p.name); }
    };

    bool operator==(const UnitDefinitionType& other) const { return name == other.name; }
};

extern const std::unordered_set<UnitDefinitionType, UnitDefinitionType::Hash> common_unitdefinitions;

// Default UnitDefinitionTypes          |name|kg, m, s, A, K,mol,cd,rad
static const UnitDefinitionType UD_kg("kg", 1, 0, 0, 0, 0, 0, 0, 0);
static const UnitDefinitionType UD_m("m", 0, 1, 0, 0, 0, 0, 0, 0);
static const UnitDefinitionType UD_s("s", 0, 0, 1, 0, 0, 0, 0, 0);
static const UnitDefinitionType UD_A("A", 0, 0, 0, 1, 0, 0, 0, 0);
static const UnitDefinitionType UD_K("K", 0, 0, 0, 0, 1, 0, 0, 0);
static const UnitDefinitionType UD_mol("mol", 0, 0, 0, 0, 0, 1, 0, 0);
static const UnitDefinitionType UD_cd("cd", 0, 0, 0, 0, 0, 0, 1, 0);
static const UnitDefinitionType UD_rad("rad", 0, 0, 0, 0, 0, 0, 0, 1);

static const UnitDefinitionType UD_m_s("m/s", 0, 1, -1, 0, 0, 0, 0, 0);
static const UnitDefinitionType UD_m_s2("m/s2", 0, 1, -2, 0, 0, 0, 0, 0);
static const UnitDefinitionType UD_rad_s("rad/s", 0, 0, -1, 0, 0, 0, 0, 1);
static const UnitDefinitionType UD_rad_s2("rad/s2", 0, 0, -2, 0, 0, 0, 0, 1);

static const UnitDefinitionType UD_N("N", 1, 1, -2, 0, 0, 0, 0, 0);
static const UnitDefinitionType UD_Nm("Nm", 1, 2, -2, 0, 0, 0, 0, 0);
static const UnitDefinitionType UD_N_m2("N/m2", 1, -1, -2, 0, 0, 0, 0, 0);

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

    /// Set the value of this FMU variable (for all cases, except variables of type fmi2String).
    template <typename T, typename = typename std::enable_if<!std::is_same<T, fmi2String>::value>::type>
    void SetValue(const T& val) const {
        if (is_pointer_variant(this->varbind))
            *varns::get<T*>(this->varbind) = val;
        else
            varns::get<FunGetSet<T>>(this->varbind).second(val);
    }

    /// Set the value of this FMU variable of type fmi2String.
    void SetValue(const fmi2String& val) const;

    /// Get the value of this FMU variable (for all cases, except variables of type fmi2String).
    template <typename T, typename = typename std::enable_if<!std::is_same<T, fmi2String>::value>::type>
    void GetValue(T* varptr) const {
        *varptr = is_pointer_variant(this->varbind) ? *varns::get<T*>(this->varbind)
                                                    : varns::get<FunGetSet<T>>(this->varbind).first();
    }

    /// Get the value of this FMU variable of type fmi2String.
    void GetValue(fmi2String* varptr) const;

    /// Set the start value for this FMU variable (for all cases, except variables of type fmi2String).
    template <typename T, typename = typename std::enable_if<!std::is_same<T, fmi2String>::value>::type>
    void SetStartVal(T startval) {
        if (!allowed_start)
            return;
        has_start = true;
        this->start = startval;
    }

    /// Set the start value for this FMU variable of type fmi2String.
    void SetStartVal(fmi2String startval);

    void ExposeCurrentValueAsStart();

  protected:
    bool allowed_start = true;
    bool required_start = false;

    VarbindType varbind;     // value of this variable
    StartType start;         // start value for this variable

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
    FmuComponentBase(fmi2String instanceName,
                     fmi2Type fmuType,
                     fmi2String fmuGUID,
                     fmi2String fmuResourceLocation,
                     const fmi2CallbackFunctions* functions,
                     fmi2Boolean visible,
                     fmi2Boolean loggingOn,
                     const std::unordered_map<std::string, bool>& logCategories_init,
                     const std::unordered_set<std::string>& logCategories_debug_init);

    virtual ~FmuComponentBase() {}

    const std::set<FmuVariableExport>& GetScalarVariables() const { return m_scalarVariables; }

    /// Enable/disable the logging for a specific log category.
    virtual void SetDebugLogging(std::string cat, bool val);

    /// Create the modelDescription.xml file in the given location \a path.
    void ExportModelDescription(std::string path);

    double GetTime() const { return m_time; }

    template <class T>
    fmi2Status fmi2GetVariable(const fmi2ValueReference vr[], size_t nvr, T value[], FmuVariable::Type vartype) {
        // TODO, when multiple variables are requested it might be better to iterate through scalarVariables just once
        //  and check if they match any of the nvr requested variables
        for (size_t s = 0; s < nvr; ++s) {
            auto it = this->findByValrefType(vr[s], vartype);
            if (it != this->m_scalarVariables.end()) {
                it->GetValue(&value[s]);
            } else
                return fmi2Status::fmi2Error;  // requested a variable that does not exist
        }
        return fmi2Status::fmi2OK;
    }

    template <class T>
    fmi2Status fmi2SetVariable(const fmi2ValueReference vr[], size_t nvr, const T value[], FmuVariable::Type vartype) {
        for (size_t s = 0; s < nvr; ++s) {
            std::set<FmuVariableExport>::iterator it = this->findByValrefType(vr[s], vartype);
            if (it != this->m_scalarVariables.end() && it->IsSetAllowed(this->m_fmuType, this->m_fmuMachineState)) {
                it->SetValue(value[s]);
            } else
                return fmi2Status::fmi2Error;  // requested a variable that does not exist or that cannot be set
        }
        return fmi2Status::fmi2OK;
    }

    /// Adds a variable to the list of variables of the FMU.
    /// The start value is automatically grabbed from the variable itself.
    const FmuVariableExport& AddFmuVariable(
        const FmuVariableExport::VarbindType& varbind,
        std::string name,
        FmuVariable::Type scalartype = FmuVariable::Type::Real,
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
    virtual bool is_cosimulation_available() const = 0;
    virtual bool is_modelexchange_available() const = 0;

    virtual fmi2Status _doStep(fmi2Real currentCommunicationPoint,
                               fmi2Real communicationStepSize,
                               fmi2Boolean noSetFMUStatePriorToCurrentPoint) {
        if (is_cosimulation_available())
            throw std::runtime_error("An FMU for co-simulation must implement _doStep");

        return fmi2OK;
    }

    virtual fmi2Status _setTime(fmi2Real time) { return fmi2Status::fmi2OK; }

    virtual fmi2Status _getContinuousStates(fmi2Real x[], size_t nx) {
        if (is_modelexchange_available())
            throw std::runtime_error("An FMU for model exchange must implement _getContinuousStates");

        return fmi2OK;
    }

    virtual fmi2Status _setContinuousStates(const fmi2Real x[], size_t nx) {
        if (is_modelexchange_available())
            throw std::runtime_error("An FMU for model exchange must implement _setContinuousStates");

        return fmi2OK;
    }

    virtual fmi2Status _getDerivatives(fmi2Real derivatives[], size_t nx) {
        if (is_modelexchange_available())
            throw std::runtime_error("An FMU for model exchange must implement _getDerivatives");

        return fmi2OK;
    }

    virtual void _preModelDescriptionExport() {}
    virtual void _postModelDescriptionExport() {}

    virtual void _enterInitializationMode() {}
    virtual void _exitInitializationMode() {}

  public:
    void SetDefaultExperiment(fmi2Boolean toleranceDefined,
                              fmi2Real tolerance,
                              fmi2Real startTime,
                              fmi2Boolean stopTimeDefined,
                              fmi2Real stopTime);

    void EnterInitializationMode();

    void ExitInitializationMode();

    // Co-Simulation FMI functions.
    // These functions are used to implement the actual co-simulation functions imposed by the FMI2 standard.
    // In turn, they call overrides of virtual methods provided by a concrete FMU.

    fmi2Status DoStep(fmi2Real currentCommunicationPoint,
                      fmi2Real communicationStepSize,
                      fmi2Boolean noSetFMUStatePriorToCurrentPoint);

    // Model Exchange FMI functions.
    // These functions are used to implement the actual model exchange functions imposed by the FMI2 standard.
    // In turn, they call overrides of virtual methods provided by a concrete FMU.

    fmi2Status SetTime(const fmi2Real time);
    fmi2Status GetContinuousStates(fmi2Real x[], size_t nx);
    fmi2Status SetContinuousStates(const fmi2Real x[], size_t nx);
    fmi2Status GetDerivatives(fmi2Real derivatives[], size_t nx);

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

    void initializeType(fmi2Type fmuType);

    void addUnitDefinition(const UnitDefinitionType& unit_definition);

    void clearUnitDefinitions() { m_unitDefinitions.clear(); }

    /// Send message to the logger function.
    /// The message will be sent if at least one of the following applies:
    /// - \a msg_cat has been enabled by `SetDebugLogging(msg_cat, true)`
    /// - the FMU has been instantiated with `loggingOn` set as `fmi2True` and \a msg_cat has been labelled as a
    /// debugging category;
    /// FMUs generated by this library provides a Description in which it is reported if the category
    /// is of debug.
    void sendToLog(std::string msg, fmi2Status status, std::string msg_cat);

    std::set<FmuVariableExport>::iterator findByValrefType(fmi2ValueReference vr, FmuVariable::Type vartype);
    std::set<FmuVariableExport>::iterator findByName(const std::string& name);

    void executePreStepCallbacks();
    void executePostStepCallbacks();

    std::string m_instanceName;
    std::string m_fmuGUID;
    std::string m_resources_location;

    bool m_visible;

    const std::unordered_set<std::string> m_logCategories_debug;  ///< list of log categories considered to be of debug
    bool m_debug_logging_enabled = true;                          ///< enable logging?

    // DefaultExperiment
    fmi2Real m_startTime = 0;
    fmi2Real m_stopTime = 1;
    fmi2Real m_tolerance = -1;
    fmi2Boolean m_toleranceDefined = 0;
    fmi2Boolean m_stopTimeDefined = 0;

    fmi2Real m_stepSize = 1e-3;
    fmi2Real m_time = 0;

    const std::string m_modelIdentifier;

    fmi2Type m_fmuType;

    std::map<FmuVariable::Type, unsigned int> m_valueReferenceCounter;

    std::set<FmuVariableExport> m_scalarVariables;
    std::unordered_map<std::string, UnitDefinitionType> m_unitDefinitions;
    std::unordered_map<std::string, std::pair<std::string, std::vector<std::string>>> m_derivatives;
    std::unordered_map<std::string, std::vector<std::string>> m_variableDependencies;

    std::list<std::function<void(void)>> m_preStepCallbacks;
    std::list<std::function<void(void)>> m_postStepCallbacks;

    fmi2CallbackFunctions m_callbackFunctions;
    FmuMachineStateType m_fmuMachineState;

    std::unordered_map<std::string, bool> m_logCategories_enabled;
};

// -----------------------------------------------------------------------------

FmuComponentBase* fmi2Instantiate_getPointer(fmi2String instanceName,
                                             fmi2Type fmuType,
                                             fmi2String fmuGUID,
                                             fmi2String fmuResourceLocation,
                                             const fmi2CallbackFunctions* functions,
                                             fmi2Boolean visible,
                                             fmi2Boolean loggingOn);

#endif
