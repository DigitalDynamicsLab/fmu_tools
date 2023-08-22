
#include "fmi2_headers/fmi2Functions.h"
#include <cassert>
#include <vector>
#include <array>
#include <map>
#include <iostream>
#include <string>

#define _USE_MATH_DEFINES
#include <math.h>


class ChFmuComponent{
public:
    ChFmuComponent(fmi2String _instanceName, fmi2Type _fmuType, fmi2String _fmuGUID): instanceName(_instanceName), fmuType(_fmuType), fmuGUID(_fmuGUID){    }
    virtual ~ChFmuComponent(){}
protected:
    std::string instanceName;
    fmi2Type fmuType;
    fmi2String fmuGUID;
public:
    bool initializationMode = false;
    double startTime = 0;
    double stopTime = 1;
    double stepSize = 1e-3;
    double time = 0;

    std::map<fmi2ValueReference, fmi2Real*> fmi2Real_map;
    std::map<fmi2ValueReference, fmi2Integer*> fmi2Integer_map;
    std::map<fmi2ValueReference, fmi2Boolean*> fmi2Boolean_map;
    std::map<fmi2ValueReference, fmi2String*> fmi2String_map;

    fmi2CallbackFunctions callbackFunctions;

    const static size_t fmi2CallbackLoggerCategorySize = 4;

    size_t fmi2CallbackLoggerCategoryID;

    void SetCallbackLoggerCategory(fmi2String category){
        for (size_t cc = 0; cc<fmi2CallbackLoggerCategorySize; ++cc)
        {
            if (category==fmi2CallbackLoggerCategories[cc])
                fmi2CallbackLoggerCategoryID = cc;
            break;
        }
    }

    fmi2String fmi2CallbackLoggerCategories[fmi2CallbackLoggerCategorySize] = {"logStatusFatal","logStatusError","logStatusWarning","logAll"};
    bool loggingOn = true;

    virtual fmi2Status DoStep(double stepSize = -1){ return fmi2Status::fmi2Error; };

};


//////////////// FMU FUNCTIONS /////////////////

const char* fmi2GetTypesPlatform(void){
    return "default";
}

const char* fmi2GetVersion(void){
    return fmi2Version;
}

fmi2Status fmi2SetDebugLogging(fmi2Component c, fmi2Boolean loggingOn, size_t nCategories, const fmi2String categories[]){
    ChFmuComponent* ch_ptr = reinterpret_cast<ChFmuComponent*>(c);
    ch_ptr->loggingOn = loggingOn==fmi2True ? true : false;
    ch_ptr->SetCallbackLoggerCategory(categories[nCategories]);

    return fmi2Status::fmi2OK;
}

fmi2Component fmi2Instantiate(fmi2String instanceName, fmi2Type fmuType, fmi2String fmuGUID, fmi2String fmuResourceLocation, const fmi2CallbackFunctions* functions, fmi2Boolean visible, fmi2Boolean loggingOn){
    ChFmuComponent* fmu_instance = new ChFmuComponent(instanceName, fmuType, fmuGUID);
    fmu_instance->callbackFunctions = *functions;
    fmu_instance->loggingOn = loggingOn;
    
    return reinterpret_cast<void*>(fmu_instance);
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

fmi2Status fmi2GetReal(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, fmi2Real value[]){
    auto& fmi2Real_map = reinterpret_cast<ChFmuComponent*>(c)->fmi2Real_map;
    for (size_t s = 0; s<nvr; ++s){
        auto it = fmi2Real_map.find(vr[s]);
        if (it != fmi2Real_map.end())
            value[s] = *it->second;
        else return fmi2Status::fmi2Error;
    }
    return fmi2Status::fmi2OK;
}

fmi2Status fmi2GetInteger(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, fmi2Integer value[]){
    auto& fmi2Integer_map = reinterpret_cast<ChFmuComponent*>(c)->fmi2Integer_map;
    for (size_t s = 0; s<nvr; ++s){
        auto it = fmi2Integer_map.find(vr[s]);
        if (it != fmi2Integer_map.end())
            value[s] = *it->second;
        else return fmi2Status::fmi2Error;
    }
    return fmi2Status::fmi2OK;
}

fmi2Status fmi2GetBoolean(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, fmi2Boolean value[]){
    auto& fmi2Boolean_map = reinterpret_cast<ChFmuComponent*>(c)->fmi2Boolean_map;
    for (size_t s = 0; s<nvr; ++s){
        auto it = fmi2Boolean_map.find(vr[s]);
        if (it != fmi2Boolean_map.end())
            value[s] = *it->second;
        else return fmi2Status::fmi2Error;
    }
    return fmi2Status::fmi2OK;
}

fmi2Status fmi2GetString(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, fmi2String value[]){
    auto& fmi2String_map = reinterpret_cast<ChFmuComponent*>(c)->fmi2String_map;
    for (size_t s = 0; s<nvr; ++s){
        auto it = fmi2String_map.find(vr[s]);
        if (it != fmi2String_map.end())
            value[s] = *it->second;
        else return fmi2Status::fmi2Error;
    }
    return fmi2Status::fmi2OK;
}


fmi2Status fmi2SetReal(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, const fmi2Real value[]){
    auto& fmi2Real_map = reinterpret_cast<ChFmuComponent*>(c)->fmi2Real_map;
    for (size_t s = 0; s<nvr; ++s){
        auto it = fmi2Real_map.find(vr[s]);
        if (it != fmi2Real_map.end())
            *it->second = value[s];
        else return fmi2Status::fmi2Error;
    }
    return fmi2Status::fmi2OK;
}

fmi2Status fmi2SetInteger(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, const fmi2Integer value[]){ return fmi2Status::fmi2OK; }
fmi2Status fmi2SetBoolean(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, const fmi2Boolean value[]){
    auto& fmi2Boolean_map = reinterpret_cast<ChFmuComponent*>(c)->fmi2Boolean_map;
    for (size_t s = 0; s<nvr; ++s){
        auto it = fmi2Boolean_map.find(vr[s]);
        if (it != fmi2Boolean_map.end())
            *it->second = value[s];
        else return fmi2Status::fmi2Error;
    }
    return fmi2Status::fmi2OK;
}
fmi2Status fmi2SetString(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, const fmi2String value[]){ return fmi2Status::fmi2OK; }
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
