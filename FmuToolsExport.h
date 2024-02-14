
#ifndef FMUTOOLSEXPORT_H
#define FMUTOOLSEXPORT_H
#include "FmuToolsCommon.h"
#include "fmi2_headers/fmi2Functions.h"
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

#ifndef FMITYPESPLATFORM_CUSTOM
    #include "TypesVariantsDefault.h"
#else
    #include "TypesVariantsCustom.h"
#endif

// =============================================================================

#ifdef __cplusplus
extern "C" {
#endif

void FMI2_Export createModelDescription(const std::string& path, fmi2Type fmutype, fmi2String guid);

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

    template <typename T, typename = typename std::enable_if<!std::is_same<T, fmi2String>::value>::type>
    void SetValue(const T& val) const {
        if (is_pointer_variant(this->varbind))
            *varns::get<T*>(this->varbind) = val;
        else
            varns::get<FunGetSet<T>>(this->varbind).second(val);
    }

    void SetValue(const fmi2String& val) const;

    template <typename T, typename = typename std::enable_if<!std::is_same<T, fmi2String>::value>::type>
    void GetValue(T* varptr) const {
        *varptr = is_pointer_variant(this->varbind) ? *varns::get<T*>(this->varbind)
                                                    : varns::get<FunGetSet<T>>(this->varbind).first();
    }

    void GetValue(fmi2String* varptr) const;

    template <typename T, typename = typename std::enable_if<!std::is_same<T, fmi2String>::value>::type>
    void SetStartVal(T startval) {
        if (allowed_start)
            has_start = true;
        else
            return;

        this->start = startval;
    }

    void SetStartVal(fmi2String startval);

    void ExposeCurrentValueAsStart();

    std::string GetStartVal_toString() const;

  protected:
    bool allowed_start = true;
    bool required_start = false;

    VarbindType varbind;
    StartType start;

    // TODO: in C++17 should be possible to either use constexpr or use lambda with 'overload' keyword
    template <typename T>
    void setStartFromVar(FunGetSet<T> funPair);

    template <typename T>
    void setStartFromVar(T* var_ptr);
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

class FmuComponentBase {
  public:
    FmuComponentBase(fmi2String _instanceName, fmi2Type _fmuType, fmi2String _fmuGUID);

    virtual ~FmuComponentBase() {}

    void SetResourceLocation(fmi2String resloc) { resourceLocation = std::string(resloc); }

    void SetDefaultExperiment(fmi2Boolean _toleranceDefined,
                              fmi2Real _tolerance,
                              fmi2Real _startTime,
                              fmi2Boolean _stopTimeDefined,
                              fmi2Real _stopTime);

    const std::set<FmuVariableExport>& GetScalarVariables() const { return scalarVariables; }

    void EnterInitializationMode();

    void ExitInitializationMode();

    void SetCallbackFunctions(const fmi2CallbackFunctions* functions) { callbackFunctions = *functions; }

    void SetLogging(bool val) { loggingOn = val; };

    void AddCallbackLoggerCategory(std::string cat);

    fmi2Status DoStep(fmi2Real currentCommunicationPoint,
                      fmi2Real communicationStepSize,
                      fmi2Boolean noSetFMUStatePriorToCurrentPoint);

    void ExportModelDescription(std::string path);

    double GetTime() const { return time; }

    void executePreStepCallbacks();

    void executePostStepCallbacks();

    template <class T>
    fmi2Status fmi2GetVariable(const fmi2ValueReference vr[], size_t nvr, T value[], FmuVariable::Type vartype) {
        // TODO, when multiple variables are requested it might be better to iterate through scalarVariables just once
        //  and check if they match any of the nvr requested variables
        for (size_t s = 0; s < nvr; ++s) {
            auto it = this->findByValrefType(vr[s], vartype);
            if (it != this->scalarVariables.end()) {
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
            if (it != this->scalarVariables.end() && it->IsSetAllowed(this->fmuType, this->fmuMachineState)) {
                it->SetValue(value[s]);
            } else
                return fmi2Status::fmi2Error;  // requested a variable that does not exist or that cannot be set
        }
        return fmi2Status::fmi2OK;
    }

    // DEV: unfortunately it is not possible to retrieve the fmi2 type based on the var_ptr only; the reason is as
    // follows: e.g. both fmi2Integer and fmi2Boolean are actually alias of type int, thus impeding any possible
    // splitting depending on type if we accept to have both fmi2Integer and fmi2Boolean considered as the same type we
    // can drop the 'scalartype' argument but the risk is that a variable might end up being flagged as Integer while
    // it's actually a Boolean and it is not nice At least, in this way, we do not have any redundant code.
    const FmuVariableExport& AddFmuVariable(
        const FmuVariableExport::VarbindType& varbind,
        std::string name,
        FmuVariable::Type scalartype = FmuVariable::Type::Real,
        std::string unitname = "",
        std::string description = "",
        FmuVariable::CausalityType causality = FmuVariable::CausalityType::local,
        FmuVariable::VariabilityType variability = FmuVariable::VariabilityType::continuous,
        FmuVariable::InitialType initial = FmuVariable::InitialType::none);

    bool RebindVariable(FmuVariableExport::VarbindType varbind, std::string name);

  protected:
    virtual bool is_cosimulation_available() const = 0;
    virtual bool is_modelexchange_available() const = 0;

    virtual fmi2Status _doStep(fmi2Real currentCommunicationPoint,
                               fmi2Real communicationStepSize,
                               fmi2Boolean noSetFMUStatePriorToCurrentPoint) = 0;

    virtual void _preModelDescriptionExport() {}
    virtual void _postModelDescriptionExport() {}

    virtual void _enterInitializationMode() {}
    virtual void _exitInitializationMode() {}

    void initializeType(fmi2Type fmuType);

    void addUnitDefinition(const UnitDefinitionType& unit_definition);

    void clearUnitDefinitions() { unitDefinitions.clear(); }

    void sendToLog(std::string msg, fmi2Status status, std::string msg_cat);

    std::set<FmuVariableExport>::iterator findByValrefType(fmi2ValueReference vr, FmuVariable::Type vartype);
    std::set<FmuVariableExport>::iterator findByName(const std::string& name);

    std::string instanceName;
    std::string fmuGUID;
    std::string resourceLocation;

    static const std::set<std::string> logCategories_available;
    std::set<std::string> logCategories;

    // DefaultExperiment
    fmi2Real startTime = 0;
    fmi2Real stopTime = 1;
    fmi2Real tolerance = -1;
    fmi2Boolean toleranceDefined = 0;
    fmi2Boolean stopTimeDefined = 0;

    fmi2Real stepSize = 1e-3;
    fmi2Real time = 0;

    const std::string modelIdentifier;

    fmi2Type fmuType;

    std::map<FmuVariable::Type, unsigned int> valueReferenceCounter;

    std::set<FmuVariableExport> scalarVariables;
    std::unordered_map<std::string, UnitDefinitionType> unitDefinitions;

    std::list<std::function<void(void)>> preStepCallbacks;
    std::list<std::function<void(void)>> postStepCallbacks;

    fmi2CallbackFunctions callbackFunctions;
    bool loggingOn = true;
    FmuMachineStateType fmuMachineState;
};

// -----------------------------------------------------------------------------

FmuComponentBase* fmi2Instantiate_getPointer(fmi2String instanceName, fmi2Type fmuType, fmi2String fmuGUID);

#endif