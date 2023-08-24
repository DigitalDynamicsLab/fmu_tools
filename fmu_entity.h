
#pragma once
#include "fmi2_headers/fmi2Functions.h"
#include "FmuToolsCommon.h"
#include <cassert>
#include <vector>
#include <array>
#include <map>
#include <iostream>
#include <string>
#include <set>
#include <unordered_map>

//static const std::set<std::string> baseUnits;

void FMI2_Export createModelDescription(const std::string& path);

//                                      |name|kg, m, s, A, K,mol,cd,rad
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



class ChFmuComponent{
public:
    ChFmuComponent(fmi2String _instanceName, fmi2Type _fmuType, fmi2String _fmuGUID, bool cosim_available, bool modelexchange_available):
        callbackFunctions({nullptr, nullptr, nullptr, nullptr, nullptr}),
        instanceName(_instanceName),
        fmuGUID(_fmuGUID),
        fmi2Type_CoSimulation_available(cosim_available),
        fmi2Type_ModelExchange_available(modelexchange_available),
        modelIdentifier(FMU_MODEL_IDENTIFIER)
    {
        switch (_fmuType)
        {
        case fmi2Type::fmi2CoSimulation:
            if (!fmi2Type_CoSimulation_available)
                throw std::runtime_error("Requested CoSimulation fmu mode but it is not available.");
            fmuType = fmi2Type::fmi2CoSimulation;
            break;
        case fmi2Type::fmi2ModelExchange:
            if (!fmi2Type_ModelExchange_available)
                throw std::runtime_error("Requested ModelExchange fmu mode but it is not available.");
            fmuType = fmi2Type::fmi2ModelExchange;
            break;
        default:
            throw std::runtime_error("Requested unrecognized fmu type.");
            break;
        }

        UnitDefinitionType ud_kg("kg");   ud_kg.kg = 1;
        UnitDefinitionType ud_m("m");     ud_m.m = 1;
        UnitDefinitionType ud_s("s");     ud_s.s = 1;
        UnitDefinitionType ud_A("A");     ud_A.A = 1;
        UnitDefinitionType ud_K("K");     ud_K.K = 1;
        UnitDefinitionType ud_mol("mol"); ud_mol.mol = 1;
        UnitDefinitionType ud_cd("cd");   ud_cd.cd = 1;
        UnitDefinitionType ud_rad("rad"); ud_rad.rad = 1;

        unitDefinitions["1"] = UnitDefinitionType("1"); // guarantee the existence default unit


    }

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

    const bool fmi2Type_CoSimulation_available;
    const bool fmi2Type_ModelExchange_available;

    const std::string modelIdentifier;

    fmi2Type fmuType;


    //TODO: check if fmi2XXX_map are needed or if scalarVariables is enough
    // or if simply they can be generated afterwards
    std::map<fmi2ValueReference, fmi2Real*> fmi2Real_map;
    std::map<fmi2ValueReference, fmi2Integer*> fmi2Integer_map;
    std::map<fmi2ValueReference, fmi2Boolean*> fmi2Boolean_map;
    std::map<fmi2ValueReference, fmi2String*> fmi2String_map;

    std::set<FmuScalarVariable> scalarVariables;
    std::unordered_map<std::string, UnitDefinitionType> unitDefinitions;

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

    void AddUnitDefinition(const UnitDefinitionType& newunitdefinition){
        unitDefinitions[newunitdefinition.name] = newunitdefinition;
    }

    void ClearUnitDefinitions(){
        unitDefinitions.clear();
    }

    std::pair<std::set<FmuScalarVariable>::iterator, bool> addFmuVariableReal(
        fmi2Real* var_ptr,
        std::string name,
        std::string unitname = "",
        std::string description = "",
        std::string causality = "",
        std::string variability = "",
        std::string initial = "");

    void sendToLog(std::string msg, fmi2Status status, std::string msg_cat){
        if (logCategories_available.find(msg_cat) == logCategories_available.end())
            throw std::runtime_error(std::string("Log category \"") + msg_cat + std::string("\" is not valid."));

        if (logCategories.find(msg_cat) != logCategories.end()){
            callbackFunctions.logger(callbackFunctions.componentEnvironment, instanceName.c_str(), status, msg_cat.c_str(), msg.c_str());
        }
    }

};

ChFmuComponent* fmi2Instantiate_getPointer(fmi2String instanceName, fmi2Type fmuType, fmi2String fmuGUID);




