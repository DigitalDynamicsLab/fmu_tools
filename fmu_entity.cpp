
#include "fmu_entity.h"
#include <cassert>
#include <vector>
#include <array>
#include <map>
#include <iostream>
#include <fstream>
#include <string>
#include <set>
#include <typeindex>



#define _USE_MATH_DEFINES
#include <math.h>

////                                      |name|kg, m, s, A, K,mol,cd,rad
//static const UnitDefinitionType UD_kg  ("kg",  1, 0, 0, 0, 0, 0, 0, 0 );
//static const UnitDefinitionType UD_m   ("m",   0, 1, 0, 0, 0, 0, 0, 0 );
//static const UnitDefinitionType UD_s   ("s",   0, 0, 1, 0, 0, 0, 0, 0 );
//static const UnitDefinitionType UD_A   ("A",   0, 0, 0, 1, 0, 0, 0, 0 );
//static const UnitDefinitionType UD_K   ("K",   0, 0, 0, 0, 1, 0, 0, 0 );
//static const UnitDefinitionType UD_mol ("mol", 0, 0, 0, 0, 0, 1, 0, 0 );
//static const UnitDefinitionType UD_cd  ("cd",  0, 0, 0, 0, 0, 0, 1, 0 );
//static const UnitDefinitionType UD_rad ("rad", 0, 0, 0, 0, 0, 0, 0, 1 );
//
//static const UnitDefinitionType UD_m_s    ("m/s",    0, 1, -1, 0, 0, 0, 0, 0 );
//static const UnitDefinitionType UD_m_s2   ("m/s2",   0, 1, -2, 0, 0, 0, 0, 0 );
//static const UnitDefinitionType UD_rad_s  ("rad/s",  0, 0, -1, 0, 0, 0, 0, 1 );
//static const UnitDefinitionType UD_rad_s2 ("rad/s2", 0, 0, -2, 0, 0, 0, 0, 1 );

const std::unordered_set<UnitDefinitionType, UnitDefinitionType::Hash> common_unitdefinitions = {UD_kg, UD_m, UD_s, UD_A, UD_K, UD_mol, UD_cd, UD_rad, UD_m_s, UD_m_s2, UD_rad_s, UD_rad_s2};

const std::set<std::string> ChFmuComponent::logCategories_available = {
    "logEvents",
    "logSingularLinearSystems",
    "logNonlinearSystems",
    "logDynamicStateSelection",
    "logStatusWarning",
    "logStatusDiscard",
    "logStatusError",
    "logStatusFatal",
    "logStatusPending",
    "logAll"
};



void createModelDescription(const std::string& path){
    ChFmuComponent* fmu = fmi2Instantiate_getPointer("", fmi2Type::fmi2CoSimulation, "");
    fmu->ExportModelDescription(path);
    delete fmu;
}
            
//////////////// FMU FUNCTIONS /////////////////

fmi2Component fmi2Instantiate(fmi2String instanceName, fmi2Type fmuType, fmi2String fmuGUID, fmi2String fmuResourceLocation, const fmi2CallbackFunctions* functions, fmi2Boolean visible, fmi2Boolean loggingOn){
    ChFmuComponent* fmu = fmi2Instantiate_getPointer(instanceName, fmuType, fmuGUID);
    fmu->callbackFunctions = *functions;
    fmu->loggingOn = loggingOn;
    
    return reinterpret_cast<void*>(fmu);
}

const char* fmi2GetTypesPlatform(void){
    return fmi2TypesPlatform;
}

const char* fmi2GetVersion(void){
    return fmi2Version;
}

fmi2Status fmi2SetDebugLogging(fmi2Component c, fmi2Boolean loggingOn, size_t nCategories, const fmi2String categories[]){
    ChFmuComponent* fmu_ptr = reinterpret_cast<ChFmuComponent*>(c);
    fmu_ptr->loggingOn = loggingOn==fmi2True ? true : false;
    for (auto cs = 0; cs < nCategories; ++cs){
        fmu_ptr->AddCallbackLoggerCategory(categories[cs]);
    }

    return fmi2Status::fmi2OK;
}


void fmi2FreeInstance(fmi2Component c){
    delete reinterpret_cast<ChFmuComponent*>(c);
}

fmi2Status fmi2SetupExperiment(fmi2Component c, fmi2Boolean toleranceDefined, fmi2Real tolerance, fmi2Real startTime, fmi2Boolean stopTimeDefined, fmi2Real stopTime){
    assert(toleranceDefined==fmi2False);
    assert(stopTimeDefined==fmi2False);
    reinterpret_cast<ChFmuComponent*>(c)->startTime = startTime;
    reinterpret_cast<ChFmuComponent*>(c)->stopTime = stopTime;
    return fmi2Status::fmi2OK;
}

fmi2Status fmi2EnterInitializationMode(fmi2Component c){
    reinterpret_cast<ChFmuComponent*>(c)->initializationMode = true;
    return fmi2Status::fmi2OK;
}

fmi2Status fmi2ExitInitializationMode(fmi2Component c){ 
    reinterpret_cast<ChFmuComponent*>(c)->initializationMode = false;
    return fmi2Status::fmi2OK;
}
fmi2Status fmi2Terminate(fmi2Component c){ return fmi2Status::fmi2OK; }
fmi2Status fmi2Reset(fmi2Component c){ return fmi2Status::fmi2OK; }

std::set<FmuVariable>::iterator ChFmuComponent::findByValrefType(fmi2ValueReference vr, FmuVariable::Type vartype){
    auto predicate_samevalreftype = [vr, vartype](const FmuVariable& var) {
        return var.GetValueReference() == vr;
        return var.GetType() == vartype;
    };
    return std::find_if(scalarVariables.begin(), scalarVariables.end(), predicate_samevalreftype);
}

template <class T>
fmi2Status fmi2GetVariable(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, T value[], FmuVariable::Type vartype){
    auto& scalarVariables = reinterpret_cast<ChFmuComponent*>(c)->scalarVariables;
    for (size_t s = 0; s<nvr; ++s){
        auto it = reinterpret_cast<ChFmuComponent*>(c)->findByValrefType(vr[s], vartype);
        if (it != scalarVariables.end()){
            T* val_ptr;
            it->GetPtr(&val_ptr);
            value[s] = *val_ptr;
        }
        else
            return fmi2Status::fmi2Error; // requested a variable that does not exist
    }
    return fmi2Status::fmi2OK;
}


fmi2Status fmi2GetReal(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, fmi2Real value[]){
    return fmi2GetVariable(c, vr, nvr, value, FmuVariable::Type::FMU_REAL);
}

fmi2Status fmi2GetInteger(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, fmi2Integer value[]){
    return fmi2GetVariable(c, vr, nvr, value, FmuVariable::Type::FMU_INTEGER);
}

fmi2Status fmi2GetBoolean(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, fmi2Boolean value[]){
    return fmi2GetVariable(c, vr, nvr, value, FmuVariable::Type::FMU_BOOLEAN);
}

fmi2Status fmi2GetString(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, fmi2String value[]){
    return fmi2GetVariable(c, vr, nvr, value, FmuVariable::Type::FMU_STRING);
}

template <class T>
fmi2Status fmi2SetVariable(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, const T value[], FmuVariable::Type vartype){
    auto& scalarVariables = reinterpret_cast<ChFmuComponent*>(c)->scalarVariables;
    for (size_t s = 0; s<nvr; ++s){
        auto it = reinterpret_cast<ChFmuComponent*>(c)->findByValrefType(vr[s], vartype);
        if (it != scalarVariables.end()){
            T* val_ptr;
            it->GetPtr(&val_ptr);
            *val_ptr = value[s];
        }
        else
            return fmi2Status::fmi2Error; // requested a variable that does not exist
    }
    return fmi2Status::fmi2OK;
}

fmi2Status fmi2SetReal(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, const fmi2Real value[]){
    return fmi2SetVariable(c, vr, nvr, value, FmuVariable::Type::FMU_REAL);
}

fmi2Status fmi2SetInteger(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, const fmi2Integer value[]){
    return fmi2SetVariable(c, vr, nvr, value, FmuVariable::Type::FMU_INTEGER);
}

fmi2Status fmi2SetBoolean(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, const fmi2Boolean value[]){
    return fmi2SetVariable(c, vr, nvr, value, FmuVariable::Type::FMU_BOOLEAN);
}

fmi2Status fmi2SetString(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, const fmi2String value[]){
    return fmi2SetVariable(c, vr, nvr, value, FmuVariable::Type::FMU_STRING);
}

fmi2Status fmi2GetFMUstate(fmi2Component c, fmi2FMUstate* FMUstate){ return fmi2Status::fmi2OK; }
fmi2Status fmi2SetFMUstate(fmi2Component c, fmi2FMUstate FMUstate){ return fmi2Status::fmi2OK; }
fmi2Status fmi2FreeFMUstate(fmi2Component c, fmi2FMUstate* FMUstate){ return fmi2Status::fmi2OK; }
fmi2Status fmi2SerializedFMUstateSize(fmi2Component c, fmi2FMUstate FMUstate, size_t* size){ return fmi2Status::fmi2OK; }
fmi2Status fmi2SerializeFMUstate(fmi2Component c, fmi2FMUstate FMUstate, fmi2Byte serializedState[], size_t size){ return fmi2Status::fmi2OK; }
fmi2Status fmi2DeSerializeFMUstate(fmi2Component c, const fmi2Byte serializedState[], size_t size, fmi2FMUstate* FMUstate){ return fmi2Status::fmi2OK; }
fmi2Status fmi2GetDirectionalDerivative(fmi2Component c, const fmi2ValueReference vUnknown_ref[], size_t nUnknown, const fmi2ValueReference vKnown_ref[], size_t nKnown, const fmi2Real dvKnown[], fmi2Real dvUnknown[]){ return fmi2Status::fmi2OK; }

////// Model Exchange
//fmi2Status fmi2EnterEventMode(fmi2Component c){ return fmi2Status::fmi2OK; }
//fmi2Status fmi2NewDiscreteStates(fmi2Component c, fmi2EventInfo* fmi2eventInfo){ return fmi2Status::fmi2OK; }
//fmi2Status fmi2EnterContinuousTimeMode(fmi2Component c){ return fmi2Status::fmi2OK; }
//fmi2Status fmi2CompletedIntegratorStep(fmi2Component c, fmi2Boolean noSetFMUStatePriorToCurrentPoint, fmi2Boolean* enterEventMode, fmi2Boolean* terminateSimulation){ return fmi2Status::fmi2OK; }
//fmi2Status fmi2SetTime(fmi2Component c, fmi2Real time){ return fmi2Status::fmi2OK; }
//fmi2Status fmi2SetContinuousStates(fmi2Component c, const fmi2Real x[], size_t nx){ return fmi2Status::fmi2OK; }
//fmi2Status fmi2GetDerivatives(fmi2Component c, fmi2Real derivatives[], size_t nx){ return fmi2Status::fmi2OK; }
//fmi2Status fmi2GetEventIndicators(fmi2Component c, fmi2Real eventIndicators[], size_t ni){ return fmi2Status::fmi2OK; }
//fmi2Status fmi2GetContinuousStates(fmi2Component c, fmi2Real x[], size_t nx){ return fmi2Status::fmi2OK; }
//fmi2Status fmi2GetNominalsOfContinuousStates(fmi2Component c, fmi2Real x_nominal[], size_t nx){ return fmi2Status::fmi2OK; }

// Co-Simulation
fmi2Status fmi2SetRealInputDerivatives(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, const fmi2Integer order[], const fmi2Real value[]){ return fmi2Status::fmi2OK; }
fmi2Status fmi2GetRealOutputDerivatives(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, const fmi2Integer order[], fmi2Real value[]){ return fmi2Status::fmi2OK; }

fmi2Status fmi2DoStep(fmi2Component c, fmi2Real currentCommunicationPoint, fmi2Real communicationStepSize, fmi2Boolean noSetFMUStatePriorToCurrentPoint){ 
    fmi2Status fmi2DoStep_status = fmi2Status::fmi2OK;
    while (true){
        double candidateStepSize = currentCommunicationPoint + communicationStepSize - reinterpret_cast<ChFmuComponent*>(c)->time;
        if (candidateStepSize < -1e-10)
            return fmi2Status::fmi2Warning;
        else
        {
            if (candidateStepSize < 1e-10)
                break;

            fmi2DoStep_status = reinterpret_cast<ChFmuComponent*>(c)->DoStep(candidateStepSize);
        }
    }
    
    return fmi2DoStep_status;
}

fmi2Status fmi2CancelStep(fmi2Component c){ return fmi2Status::fmi2OK; }
fmi2Status fmi2GetStatus(fmi2Component c, const fmi2StatusKind s, fmi2Status* value){ return fmi2Status::fmi2OK; }
fmi2Status fmi2GetRealStatus(fmi2Component c, const fmi2StatusKind s, fmi2Real* value){ return fmi2Status::fmi2OK; }
fmi2Status fmi2GetIntegerStatus(fmi2Component c, const fmi2StatusKind s, fmi2Integer* value){ return fmi2Status::fmi2OK; }
fmi2Status fmi2GetBooleanStatus(fmi2Component c, const fmi2StatusKind s, fmi2Boolean* value){ return fmi2Status::fmi2OK; }
fmi2Status fmi2GetStringStatus(fmi2Component c, const fmi2StatusKind s, fmi2String* value){ return fmi2Status::fmi2OK; }
