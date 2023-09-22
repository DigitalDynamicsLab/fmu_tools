
#pragma once
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


#include "is_variant_invocable.h"
#include "variant/variant_guard.hpp"


extern const std::unordered_set<UnitDefinitionType, UnitDefinitionType::Hash> common_unitdefinitions;
#ifdef __cplusplus
extern "C" {
#endif

void FMI2_Export createModelDescription(const std::string& path, fmi2Type fmutype, fmi2String guid);

#ifdef __cplusplus
}
#endif

// Default UnitDefinitionTypes          |name|kg, m, s, A, K,mol,cd,rad
static const UnitDefinitionType UD_kg  ("kg",  1, 0, 0, 0, 0, 0, 0, 0 );
static const UnitDefinitionType UD_m   ("m",   0, 1, 0, 0, 0, 0, 0, 0 );
static const UnitDefinitionType UD_s   ("s",   0, 0, 1, 0, 0, 0, 0, 0 );
static const UnitDefinitionType UD_A   ("A",   0, 0, 0, 1, 0, 0, 0, 0 );
static const UnitDefinitionType UD_K   ("K",   0, 0, 0, 0, 1, 0, 0, 0 );
static const UnitDefinitionType UD_mol ("mol", 0, 0, 0, 0, 0, 1, 0, 0 );
static const UnitDefinitionType UD_cd  ("cd",  0, 0, 0, 0, 0, 0, 1, 0 );
static const UnitDefinitionType UD_rad ("rad", 0, 0, 0, 0, 0, 0, 0, 1 );

static const UnitDefinitionType UD_m_s    ("m/s",    0, 1, -1, 0, 0, 0, 0, 0 );
static const UnitDefinitionType UD_m_s2   ("m/s2",   0, 1, -2, 0, 0, 0, 0, 0 );
static const UnitDefinitionType UD_rad_s  ("rad/s",  0, 0, -1, 0, 0, 0, 0, 1 );
static const UnitDefinitionType UD_rad_s2 ("rad/s2", 0, 0, -2, 0, 0, 0, 0, 1 );



class FmuComponentBase{

public:
    FmuComponentBase(fmi2String _instanceName, fmi2Type _fmuType, fmi2String _fmuGUID):
        callbackFunctions({nullptr, nullptr, nullptr, nullptr, nullptr}),
        instanceName(_instanceName),
        fmuGUID(FMU_GUID),
        modelIdentifier(FMU_MODEL_IDENTIFIER),
        fmuMachineState(FmuMachineStateType::instantiated)
    {

        unitDefinitions["1"] = UnitDefinitionType("1"); // guarantee the existence of the default unit

        addFmuVariable(&time, "time", FmuVariable::Type::FMU_REAL, "s", "time");

        if(std::string(_fmuGUID).compare(fmuGUID) && ENABLE_GUID_CHECK)
            throw std::runtime_error("GUID used for instantiation not matching with source.");

    }

    virtual ~FmuComponentBase(){}

    void SetResourceLocation(fmi2String resloc){
        resourceLocation = std::string(resloc);
    }
    

    void SetDefaultExperiment(fmi2Boolean _toleranceDefined, fmi2Real _tolerance, fmi2Real _startTime, fmi2Boolean _stopTimeDefined, fmi2Real _stopTime){
        startTime = _startTime;
        stopTime = _stopTime;
        tolerance = _tolerance;
        toleranceDefined = _toleranceDefined;
        stopTimeDefined = _stopTimeDefined;
    }

    const std::set<FmuVariable>& GetScalarVariables() const { return scalarVariables; }

    void EnterInitializationMode(){
        fmuMachineState = FmuMachineStateType::initializationMode;
        _enterInitializationMode();
    };

    void ExitInitializationMode(){
        _exitInitializationMode();
        fmuMachineState = FmuMachineStateType::stepCompleted; // TODO: introduce additional state when after initialization and before step?
    };

    void SetCallbackFunctions(const fmi2CallbackFunctions* functions){ callbackFunctions = *functions; }

    void SetLogging(bool val){loggingOn = val; };

    void AddCallbackLoggerCategory(std::string cat){
        if (logCategories_available.find(cat) == logCategories_available.end())
            throw std::runtime_error(std::string("Log category \"") + cat + std::string("\" is not valid."));
        logCategories.insert(cat);
    }

    fmi2Status DoStep(fmi2Real currentCommunicationPoint, fmi2Real communicationStepSize, fmi2Boolean noSetFMUStatePriorToCurrentPoint) {
        fmi2Status doStep_status = _doStep(currentCommunicationPoint, communicationStepSize, noSetFMUStatePriorToCurrentPoint);

        switch (doStep_status)
        {
        case fmi2OK:
            fmuMachineState = FmuMachineStateType::stepCompleted;
            break;
        case fmi2Warning:
            fmuMachineState = FmuMachineStateType::stepCompleted;
            break;
        case fmi2Discard:
            fmuMachineState = FmuMachineStateType::stepFailed;
            break;
        case fmi2Error:
            fmuMachineState = FmuMachineStateType::error;
            break;
        case fmi2Fatal:
            fmuMachineState = FmuMachineStateType::fatal;
            break;
        case fmi2Pending:
            fmuMachineState = FmuMachineStateType::stepInProgress;
            break;
        default:
            throw std::runtime_error("Developer error: unexpected status from _doStep");
            break;
        }

        return doStep_status;
    }

    void ExportModelDescription(std::string path);

    double GetTime() const {return time;}

    template <class T>
    fmi2Status fmi2GetVariable(const fmi2ValueReference vr[], size_t nvr, T value[], FmuVariable::Type vartype){
        //TODO, when multiple variables are requested it might be better to iterate through scalarVariables just once
        // and check if they match any of the nvr requested variables 
        for (size_t s = 0; s<nvr; ++s){
            auto it = this->findByValrefType(vr[s], vartype);
            if (it != this->scalarVariables.end()){
                it->GetValue(&value[s]);
            }
            else
                return fmi2Status::fmi2Error; // requested a variable that does not exist
        }
        return fmi2Status::fmi2OK;
    }

    template <class T>
    fmi2Status fmi2SetVariable(const fmi2ValueReference vr[], size_t nvr, const T value[], FmuVariable::Type vartype){
    for (size_t s = 0; s<nvr; ++s){
        auto it = this->findByValrefType(vr[s], vartype);
        if (it != this->scalarVariables.end() && it->IsSetAllowed(this->fmuType, this->fmuMachineState)){
            T* val_ptr = nullptr;
            it->GetPtr(&val_ptr);
            *val_ptr = value[s];
        }
        else
            return fmi2Status::fmi2Error; // requested a variable that does not exist or that cannot be set
    }
    return fmi2Status::fmi2OK;
}

    // DEV: unfortunately it is not possible to retrieve the fmi2 type based on the var_ptr only; the reason is that:
    // e.g. both fmi2Integer and fmi2Boolean are actually alias of type int, thus impeding any possible splitting depending on type
    // if we accept to have both fmi2Integer and fmi2Boolean considered as the same type we can drop the 'scalartype' argument
    // but the risk is that a variable might end up being flagged as Integer while it's actually a Boolean and it is not nice
    // At least, in this way, we do not have any redundant code at least
    const FmuVariable& addFmuVariable(
            FmuVariable::PtrType var_ptr,
            std::string name,
            FmuVariable::Type scalartype = FmuVariable::Type::FMU_REAL,
            std::string unitname = "",
            std::string description = "",
            FmuVariable::CausalityType causality = FmuVariable::CausalityType::local,
            FmuVariable::VariabilityType variability = FmuVariable::VariabilityType::continuous,
            FmuVariable::InitialType initial = FmuVariable::InitialType::none)
    {

        // check if unit definition exists
        auto match_unit = unitDefinitions.find(unitname);
        if (match_unit == unitDefinitions.end()){
            auto predicate_samename = [unitname](const UnitDefinitionType& var) { return var.name == unitname; };
            auto match_commonunit = std::find_if(common_unitdefinitions.begin(), common_unitdefinitions.end(), predicate_samename);
            if (match_commonunit == common_unitdefinitions.end()){
                throw std::runtime_error("Variable unit is not registered within this FmuComponentBase. Call 'addUnitDefinition' first.");
            }
            else{
                addUnitDefinition(*match_commonunit);
            }
        }


        // create new variable
        // check if same-name variable exists
        auto predicate_samename = [name](const FmuVariable& var) { return var.GetName() == name; };
        auto it = std::find_if(scalarVariables.begin(), scalarVariables.end(), predicate_samename);
        if (it!=scalarVariables.end())
            throw std::runtime_error("Cannot add two Fmu variables with the same name.");




        FmuVariable newvar(name, scalartype, causality, variability, initial);
        newvar.SetUnitName(unitname);
        newvar.SetValueReference(++valueReferenceCounter[scalartype]);
        newvar.SetDescription(description);

        // check that the attributes of the variable would allow a no-set variable
        const FmuMachineStateType tempFmuState = FmuMachineStateType::anySettableState;
        if (is_variant_invocable(var_ptr) && newvar.IsSetAllowed(fmuType, tempFmuState)){
            sendToLog("Variable '" + name + "' was provided only through getter function, but its attributes would allow the user to call fmi2SetXXX on it. This is a bad practice.\n", fmi2Status::fmi2Warning, "logWarning");
        }

        newvar.SetPtr(var_ptr);


        varns::visit([&newvar](auto var_ptr_expanded) { newvar.SetStartValIfRequired(var_ptr_expanded);}, var_ptr);



        std::pair<std::set<FmuVariable>::iterator, bool> ret = scalarVariables.insert(newvar);
        if(!ret.second)
            throw std::runtime_error("Developer error: cannot insert new variable into FMU.");

        return *(ret.first);
    }


protected:

    virtual fmi2Status _doStep(fmi2Real currentCommunicationPoint, fmi2Real communicationStepSize, fmi2Boolean noSetFMUStatePriorToCurrentPoint) = 0;


    virtual void _enterInitializationMode() = 0;

    virtual void _exitInitializationMode() = 0;

    void initializeType(fmi2Type _fmuType){
        switch (_fmuType)
        {
        case fmi2Type::fmi2CoSimulation:
            if (!is_cosimulation_available())
                throw std::runtime_error("Requested CoSimulation FMU mode but it is not available.");
            fmuType = fmi2Type::fmi2CoSimulation;
            break;
        case fmi2Type::fmi2ModelExchange:
            if (!is_modelexchange_available())
                throw std::runtime_error("Requested ModelExchange FMU mode but it is not available.");
            fmuType = fmi2Type::fmi2ModelExchange;
            break;
        default:
            throw std::runtime_error("Requested unrecognized FMU type.");
            break;
        }
    }

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

    std::set<FmuVariable> scalarVariables;
    std::unordered_map<std::string, UnitDefinitionType> unitDefinitions;

    std::set<FmuVariable>::iterator findByValrefType(fmi2ValueReference vr, FmuVariable::Type vartype);


    fmi2CallbackFunctions callbackFunctions;
    bool loggingOn = true;
    FmuMachineStateType fmuMachineState;

    
    virtual bool is_cosimulation_available() const = 0;
    virtual bool is_modelexchange_available() const = 0;

    void addUnitDefinition(const UnitDefinitionType& newunitdefinition){
        unitDefinitions[newunitdefinition.name] = newunitdefinition;
    }

    void clearUnitDefinitions(){
        unitDefinitions.clear();
    }


    
    void sendToLog(std::string msg, fmi2Status status, std::string msg_cat){
        if (logCategories_available.find(msg_cat) == logCategories_available.end())
            throw std::runtime_error(std::string("Log category \"") + msg_cat + std::string("\" is not valid."));

        if (logCategories.find(msg_cat) != logCategories.end()){
            callbackFunctions.logger(callbackFunctions.componentEnvironment, instanceName.c_str(), status, msg_cat.c_str(), msg.c_str());
        }
    }

};


FmuComponentBase* fmi2Instantiate_getPointer(fmi2String instanceName, fmi2Type fmuType, fmi2String fmuGUID);




