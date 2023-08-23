
#include "fmi2_headers/fmi2Functions.h"
#include "FmuToolsCommon.hpp"
#include <cassert>
#include <vector>
#include <array>
#include <map>
#include <iostream>
#include <string>
#include <set>



class ChFmuComponent{
public:
    ChFmuComponent(fmi2String _instanceName, fmi2Type _fmuType, fmi2String _fmuGUID): instanceName(_instanceName), fmuGUID(_fmuGUID){}
    virtual ~ChFmuComponent(){}
protected:
    std::string instanceName;
    fmi2String fmuGUID;

    static const std::set<std::string> logCategories_available;

    std::set<std::string> logCategories;

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

    std::set<FmuScalarVariable> scalarVariables;

    fmi2CallbackFunctions callbackFunctions;

    void AddCallbackLoggerCategory(std::string cat){
        if (logCategories_available.find(cat) == logCategories_available.end())
            throw std::runtime_error(std::string("Log category \"") + cat + std::string("\" is not valid."));
        logCategories.insert(cat);
    }

    bool loggingOn = true;

    virtual fmi2Status DoStep(double stepSize = -1){ return fmi2Status::fmi2Error; };

    void ExportModelDescription(std::string path);

protected:



    fmi2ValueReference addFmuVariableReal(
        fmi2Real* var_ptr,
        std::string name,
        std::string description = "",
        std::string causality = "",
        std::string variability = "",
        std::string initial = ""){


        // check if same-name variable exists
        auto predicate_samename = [name](const FmuScalarVariable& var) { return var.name == name; };
        auto it = std::find_if(scalarVariables.begin(), scalarVariables.end(), predicate_samename);
        if (it!=scalarVariables.end())
            throw std::runtime_error("Cannot add two Fmu Variables with the same name.");

        // assign value reference
        fmi2ValueReference valref = fmi2Real_map.empty() ? 0 : fmi2Real_map.crbegin()->first+1;
        fmi2Real_map[valref] = var_ptr; //TODO: check if needed or the scalarVariables set is enough

        // create new variable
        FmuScalarVariable newvar;
        newvar.ptr.fmi2Real_ptr = var_ptr;
        newvar.name = name;
        newvar.description = description;
        newvar.causality = causality;
        newvar.variability = variability;
        newvar.initial = initial;

        scalarVariables.insert(newvar);

    }

    void sendToLog(std::string msg, fmi2Status status, std::string msg_cat){
        if (logCategories_available.find(msg_cat) == logCategories_available.end())
            throw std::runtime_error(std::string("Log category \"") + msg_cat + std::string("\" is not valid."));

        if (logCategories.find(msg_cat) != logCategories.end()){
            callbackFunctions.logger(callbackFunctions.componentEnvironment, instanceName.c_str(), status, msg_cat.c_str(), msg.c_str());
        }
    }

};

extern ChFmuComponent* fmi2Instantiate_getPointer(fmi2String instanceName, fmi2Type fmuType, fmi2String fmuGUID);
extern void createModelDescription(std::string path);


