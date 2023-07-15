
//#define FMI2_FUNCTION_PREFIX MyModel_
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
    ChFmuComponent(fmi2String _instanceName, fmi2Type _fmuType, fmi2String _fmuGUID): instanceName(_instanceName), fmuType(_fmuType), fmuGUID(_fmuGUID){}
    virtual ~ChFmuComponent(){}
protected:
    fmi2String instanceName;
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

    size_t fmi2CallbackLoggerCategoryID;

    void SetCallbackLoggerCategory(fmi2String category){
        for (size_t cc = 0; cc<4; ++cc)
        {
            if (category==fmi2CallbackLoggerCategories[cc])
                fmi2CallbackLoggerCategoryID = cc;
            break;
        }
    }

    fmi2String fmi2CallbackLoggerCategories[4] = {"logStatusFatal","logStatusError","logStatusWarning","logAll"};
    bool loggingOn = true;

    virtual fmi2Status DoStep(double stepSize = -1){ return fmi2Status::fmi2Error; };

};

std::array<double, 4> operator*(double lhs, const std::array<double, 4>& rhs)
{
    std::array<double, 4> temp;
    temp[0] = rhs[0]*lhs;
    temp[1] = rhs[1]*lhs;
    temp[2] = rhs[2]*lhs;
    temp[3] = rhs[3]*lhs;
    return temp;
}

std::array<double, 4> operator+(const std::array<double, 4>& lhs, const std::array<double, 4>& rhs)
{
    std::array<double, 4> temp;
    temp[0] = rhs[0]+lhs[0];
    temp[1] = rhs[1]+lhs[1];
    temp[2] = rhs[2]+lhs[2];
    temp[3] = rhs[3]+lhs[3];
    return temp;
}

class ChFmuPendulum: public ChFmuComponent{
public:
    ChFmuPendulum(fmi2String _instanceName, fmi2Type _fmuType, fmi2String _fmuGUID): g(9.81), ChFmuComponent(_instanceName, _fmuType, _fmuGUID){
        fmi2Real_map[0] = &q_t[0];
        fmi2Real_map[1] = &q[0];
        fmi2Real_map[2] = &q[1];
        fmi2Real_map[3] = &q_t[2];
        fmi2Real_map[4] = &q[2];
        fmi2Real_map[5] = &q[3];
        fmi2Real_map[6] = &len;
        fmi2Real_map[7] = &m;
        fmi2Real_map[8] = &M;

        fmi2Boolean_map[0] = &approximateOn;

        q = {0, 0, 0, M_PI/4};
    }
    virtual ~ChFmuPendulum(){}

    virtual fmi2Status DoStep(double _stepSize = -1) override {
        if (_stepSize<0){
            if (fmi2CallbackLoggerCategoryID>=2){
                std::string str = "fmi2DoStep requeste a negative stepsize: " + std::to_string(_stepSize) + ".\n";
                callbackFunctions.logger(this, instanceName, fmi2Status::fmi2Warning, "Warning", str.c_str());
            }

            return fmi2Status::fmi2Warning;
        }

        _stepSize = std::min(_stepSize, stepSize);

        get_q_t(q, k1); // = q_t(q(step, :));
        get_q_t(q + (0.5*_stepSize)*k1, k2); // = q_t(q(step, :) + stepsize*k1/2);
        get_q_t(q + (0.5*_stepSize)*k2, k3); // = q_t(q(step, :) + stepsize*k2/2);
        get_q_t(q + _stepSize*k3, k4); // = q_t(q(step, :) + stepsize*k3);

        q_t = (1.0/6.0)*(k1 + 2.0*k2 + 2.0*k3 + k4);
        q = q + _stepSize*q_t;
        time = time + _stepSize;


        if (fmi2CallbackLoggerCategoryID>=3){
            std::string str = "Step at time: " + std::to_string(time) + " succeeded.\n";
            callbackFunctions.logger(this, instanceName, fmi2Status::fmi2OK, "Status", str.c_str());
        }

        return fmi2Status::fmi2OK;
    }

    double get_x_tt(double th_t, double th) {
        return (m*sin(th)*(len*th_t*th_t + g*cos(th)))/(-m*cos(th)*cos(th) + M + m);
    }

    double get_th_tt(double th_t, double th) {
        return -(sin(th)*(len*m*cos(th)*th_t*th_t + M*g + g*m))/(len*(-m*cos(th)*cos(th) + M + m));
    }

    double get_x_tt_linear(double th_t, double th) {
        return (m*th*(len*th_t*th_t + g))/M;
    }

    double get_th_tt_linear(double th_t, double th) {
        return -(th*(len*m*th_t*th_t + M*g + g*m))/(len*M);
    }

    void get_q_t(const std::array<double, 4>& _q, std::array<double, 4>& q_t){
        q_t[0] = approximateOn ? get_x_tt_linear(_q[2], _q[3]) : get_x_tt(_q[2], _q[3]);
        q_t[1] = _q[0];
        q_t[2] = approximateOn ? get_th_tt_linear(_q[2], _q[3]) : get_th_tt(_q[2], _q[3]);
        q_t[3] = _q[2];
    }


private:
    std::array<double, 4> q;
    std::array<double, 4> q_t;
    double len = 0.5;
    double m = 1.0;
    double M = 1.0;
    const double g;

    fmi2Boolean approximateOn = fmi2False;

    // TEMP
    std::array<double, 4> k1;
    std::array<double, 4> k2;
    std::array<double, 4> k3;
    std::array<double, 4> k4;
};


//////////////// FMU FUNCTIONS /////////////////

const char* fmi2GetTypesPlatform(void){
    return "default";
}

const char* fmi2GetVersion(void){
    return fmi2Version;
}

fmi2Status fmi2SetDebugLogging(fmi2Component c, fmi2Boolean loggingOn, size_t nCategories, const fmi2String categories[]){
    ChFmuPendulum* ch_ptr = reinterpret_cast<ChFmuPendulum*>(c);
    ch_ptr->loggingOn = loggingOn==fmi2True ? true : false;
    ch_ptr->SetCallbackLoggerCategory(categories[nCategories]);
    return fmi2Status::fmi2OK;
}

fmi2Component fmi2Instantiate(fmi2String instanceName, fmi2Type fmuType, fmi2String fmuGUID, fmi2String fmuResourceLocation, const fmi2CallbackFunctions* functions, fmi2Boolean visible, fmi2Boolean loggingOn){
    ChFmuPendulum* fmu_instance = new ChFmuPendulum(instanceName, fmuType, fmuGUID);
    fmu_instance->callbackFunctions = *functions;
    fmu_instance->loggingOn = loggingOn;
    
    return fmu_instance;
}

void fmi2FreeInstance(fmi2Component c){
    delete reinterpret_cast<ChFmuPendulum*>(c);
}

fmi2Status fmi2SetupExperiment(fmi2Component c, fmi2Boolean toleranceDefined, fmi2Real tolerance, fmi2Real startTime, fmi2Boolean stopTimeDefined, fmi2Real stopTime){
    assert(toleranceDefined==fmi2False);
    assert(stopTimeDefined==fmi2False);
    reinterpret_cast<ChFmuPendulum*>(c)->startTime = startTime;
    reinterpret_cast<ChFmuPendulum*>(c)->stopTime = stopTime;
    return fmi2Status::fmi2OK;
}

fmi2Status fmi2EnterInitializationMode(fmi2Component c){
    reinterpret_cast<ChFmuPendulum*>(c)->initializationMode = true;
    return fmi2Status::fmi2OK;
}

fmi2Status fmi2ExitInitializationMode(fmi2Component c){ 
    reinterpret_cast<ChFmuPendulum*>(c)->initializationMode = false;
    return fmi2Status::fmi2OK;
}
fmi2Status fmi2Terminate(fmi2Component c){ return fmi2Status::fmi2OK; }
fmi2Status fmi2Reset(fmi2Component c){ return fmi2Status::fmi2OK; }

fmi2Status fmi2GetReal(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, fmi2Real value[]){
    auto& fmi2Real_map = reinterpret_cast<ChFmuPendulum*>(c)->fmi2Real_map;
    for (size_t s = 0; s<nvr; ++s){
        auto it = fmi2Real_map.find(vr[s]);
        if (it != fmi2Real_map.end())
            value[s] = *it->second;
        else return fmi2Status::fmi2Error;
    }
    return fmi2Status::fmi2OK;
}

fmi2Status fmi2GetInteger(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, fmi2Integer value[]){
    auto& fmi2Integer_map = reinterpret_cast<ChFmuPendulum*>(c)->fmi2Integer_map;
    for (size_t s = 0; s<nvr; ++s){
        auto it = fmi2Integer_map.find(vr[s]);
        if (it != fmi2Integer_map.end())
            value[s] = *it->second;
        else return fmi2Status::fmi2Error;
    }
    return fmi2Status::fmi2OK;
}

fmi2Status fmi2GetBoolean(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, fmi2Boolean value[]){
    auto& fmi2Boolean_map = reinterpret_cast<ChFmuPendulum*>(c)->fmi2Boolean_map;
    for (size_t s = 0; s<nvr; ++s){
        auto it = fmi2Boolean_map.find(vr[s]);
        if (it != fmi2Boolean_map.end())
            value[s] = *it->second;
        else return fmi2Status::fmi2Error;
    }
    return fmi2Status::fmi2OK;
}

fmi2Status fmi2GetString(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, fmi2String value[]){
    auto& fmi2String_map = reinterpret_cast<ChFmuPendulum*>(c)->fmi2String_map;
    for (size_t s = 0; s<nvr; ++s){
        auto it = fmi2String_map.find(vr[s]);
        if (it != fmi2String_map.end())
            value[s] = *it->second;
        else return fmi2Status::fmi2Error;
    }
    return fmi2Status::fmi2OK;
}


fmi2Status fmi2SetReal(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, const fmi2Real value[]){
    auto& fmi2Real_map = reinterpret_cast<ChFmuPendulum*>(c)->fmi2Real_map;
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
    auto& fmi2Boolean_map = reinterpret_cast<ChFmuPendulum*>(c)->fmi2Boolean_map;
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

//// Model Exchange
fmi2Status fmi2EnterEventMode(fmi2Component c){ return fmi2Status::fmi2OK; }
fmi2Status fmi2NewDiscreteStates(fmi2Component c, fmi2EventInfo* fmi2eventInfo){ return fmi2Status::fmi2OK; }
fmi2Status fmi2EnterContinuousTimeMode(fmi2Component c){ return fmi2Status::fmi2OK; }
fmi2Status fmi2CompletedIntegratorStep(fmi2Component c, fmi2Boolean noSetFMUStatePriorToCurrentPoint, fmi2Boolean* enterEventMode, fmi2Boolean* terminateSimulation){ return fmi2Status::fmi2OK; }
fmi2Status fmi2SetTime(fmi2Component c, fmi2Real time){ return fmi2Status::fmi2OK; }
fmi2Status fmi2SetContinuousStates(fmi2Component c, const fmi2Real x[], size_t nx){ return fmi2Status::fmi2OK; }
fmi2Status fmi2GetDerivatives(fmi2Component c, fmi2Real derivatives[], size_t nx){ return fmi2Status::fmi2OK; }
fmi2Status fmi2GetEventIndicators(fmi2Component c, fmi2Real eventIndicators[], size_t ni){ return fmi2Status::fmi2OK; }
fmi2Status fmi2GetContinuousStates(fmi2Component c, fmi2Real x[], size_t nx){ return fmi2Status::fmi2OK; }
fmi2Status fmi2GetNominalsOfContinuousStates(fmi2Component c, fmi2Real x_nominal[], size_t nx){ return fmi2Status::fmi2OK; }

// Co-Simulation
fmi2Status fmi2SetRealInputDerivatives(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, const fmi2Integer order[], const fmi2Real value[]){ return fmi2Status::fmi2OK; }
fmi2Status fmi2GetRealOutputDerivatives(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, const fmi2Integer order[], fmi2Real value[]){ return fmi2Status::fmi2OK; }
fmi2Status fmi2DoStep(fmi2Component c, fmi2Real currentCommunicationPoint, fmi2Real communicationStepSize, fmi2Boolean noSetFMUStatePriorToCurrentPoint){ 
    fmi2Status fmi2DoStep_status = fmi2Status::fmi2OK;
    while (true){
        double candidateStepSize = currentCommunicationPoint + communicationStepSize - reinterpret_cast<ChFmuPendulum*>(c)->time;
        if (candidateStepSize < -1e-10)
            return fmi2Status::fmi2Warning;
        else
        {
            if (candidateStepSize < 1e-10)
                break;

            fmi2DoStep_status = reinterpret_cast<ChFmuPendulum*>(c)->DoStep(candidateStepSize);
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
