
#include "fmu_entity.h"
#include <cassert>
#include <vector>
#include <array>
#include <map>
#include <iostream>
#include <fstream>
#include <string>
#include <set>

#include "rapidxml.hpp"
#include "rapidxml_print.hpp"

#define _USE_MATH_DEFINES
#include <math.h>

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


void ChFmuComponent::ExportModelDescription(std::string path){
    // Create the XML document
    rapidxml::xml_document<>* doc_ptr = new rapidxml::xml_document<>();


    // Add the XML declaration
    rapidxml::xml_node<>* declaration = doc_ptr->allocate_node(rapidxml::node_declaration);
    declaration->append_attribute(doc_ptr->allocate_attribute("version", "1.0"));
    declaration->append_attribute(doc_ptr->allocate_attribute("encoding", "UTF-8"));
    doc_ptr->append_node(declaration);

    // Create the root node
    rapidxml::xml_node<>* rootNode = doc_ptr->allocate_node(rapidxml::node_element, "fmiModelDescription");
    rootNode->append_attribute(doc_ptr->allocate_attribute("xmlns:xsi", "http://www.w3.org/2001/XMLSchema-instance"));
    rootNode->append_attribute(doc_ptr->allocate_attribute("fmiVersion", "2.0"));
    rootNode->append_attribute(doc_ptr->allocate_attribute("modelName", "fmu_instance"));
    rootNode->append_attribute(doc_ptr->allocate_attribute("guid", "{16ce9076-4f15-4484-9e18-fefd58f15f51}"));
    rootNode->append_attribute(doc_ptr->allocate_attribute("generationTool", "fmu_generator_standalone"));
    rootNode->append_attribute(doc_ptr->allocate_attribute("variableNamingConvention", "structured"));
    rootNode->append_attribute(doc_ptr->allocate_attribute("numberOfEventIndicators", "0"));
    doc_ptr->append_node(rootNode);

    // Add CoSimulation node
    rapidxml::xml_node<>* coSimNode = doc_ptr->allocate_node(rapidxml::node_element, "CoSimulation");
    coSimNode->append_attribute(doc_ptr->allocate_attribute("modelIdentifier", "fmu_instance"));
    coSimNode->append_attribute(doc_ptr->allocate_attribute("canHandleVariableCommunicationStepSize", "true"));
    coSimNode->append_attribute(doc_ptr->allocate_attribute("canInterpolateInputs", "true"));
    coSimNode->append_attribute(doc_ptr->allocate_attribute("maxOutputDerivativeOrder", "1"));
    coSimNode->append_attribute(doc_ptr->allocate_attribute("canGetAndSetFMUstate", "false"));
    coSimNode->append_attribute(doc_ptr->allocate_attribute("canSerializeFMUstate", "false"));
    coSimNode->append_attribute(doc_ptr->allocate_attribute("providesDirectionalDerivative", "false"));
    rootNode->append_node(coSimNode);

    // Add UnitDefinitions node
    rapidxml::xml_node<>* unitDefsNode = doc_ptr->allocate_node(rapidxml::node_element, "UnitDefinitions");
    // Add Unit nodes and attributes here...
    rootNode->append_node(unitDefsNode);

    // Add LogCategories node
    rapidxml::xml_node<>* logCatNode = doc_ptr->allocate_node(rapidxml::node_element, "LogCategories");
    // Add Category nodes and attributes here...
    rootNode->append_node(logCatNode);

    // Add DefaultExperiment node
    rapidxml::xml_node<>* defaultExpNode = doc_ptr->allocate_node(rapidxml::node_element, "DefaultExperiment");
    defaultExpNode->append_attribute(doc_ptr->allocate_attribute("startTime", "0.0"));
    defaultExpNode->append_attribute(doc_ptr->allocate_attribute("stopTime", "10.0"));
    defaultExpNode->append_attribute(doc_ptr->allocate_attribute("tolerance", "1E-06"));
    rootNode->append_node(defaultExpNode);

    // Add ModelVariables node
    rapidxml::xml_node<>* modelVarsNode = doc_ptr->allocate_node(rapidxml::node_element, "ModelVariables");
    // WARNING: rapidxml does not copy the strings that we pass to print, but it just keeps the addresses until it's time to print them
    // so we cannot use a temporary string to convert the number to string and then recycle it
    std::vector<std::string> valueref_str;
    valueref_str.reserve(scalarVariables.size());
    for (auto& vs: scalarVariables){
        // Create a ScalarVariable node
        rapidxml::xml_node<>* scalarVarNode = doc_ptr->allocate_node(rapidxml::node_element, "ScalarVariable");
        scalarVarNode->append_attribute(doc_ptr->allocate_attribute("name", vs.name.c_str()));

        valueref_str.push_back(std::to_string(vs.valueReference));
        scalarVarNode->append_attribute(doc_ptr->allocate_attribute("valueReference", valueref_str.back().c_str()));

        if (!vs.description.empty()) scalarVarNode->append_attribute(doc_ptr->allocate_attribute("description", vs.description.c_str()));
        if (!vs.causality.empty())   scalarVarNode->append_attribute(doc_ptr->allocate_attribute("causality",   vs.causality.c_str()));
        if (!vs.variability.empty()) scalarVarNode->append_attribute(doc_ptr->allocate_attribute("variability", vs.variability.c_str()));
        if (!vs.initial.empty())     scalarVarNode->append_attribute(doc_ptr->allocate_attribute("initial",     vs.initial.c_str()));
        modelVarsNode->append_node(scalarVarNode);

        // TODO
        //switch (vs.type)
        //{
        //case FmuScalarVariable::FmuScalarVariableType::FMU_REAL:
        //    // Add a Real node as a child of ScalarVariable
        //    rapidxml::xml_node<>* realNode = doc_ptr->allocate_node(rapidxml::node_element, "Real");
        //    realNode->append_attribute(doc_ptr->allocate_attribute("unit", "m/s2"));
        //    scalarVarNode->append_node(realNode);
        //    break;
        //case FmuScalarVariable::FmuScalarVariableType::FMU_INTEGER:
        //    // Add a Real node as a child of ScalarVariable
        //    rapidxml::xml_node<>* realNode = doc_ptr->allocate_node(rapidxml::node_element, "Integer");
        //    realNode->append_attribute(doc_ptr->allocate_attribute("unit", "m/s2"));
        //    scalarVarNode->append_node(realNode);
        //    break;
        //case FmuScalarVariable::FmuScalarVariableType::FMU_REAL:
        //    // Add a Real node as a child of ScalarVariable
        //    rapidxml::xml_node<>* realNode = doc_ptr->allocate_node(rapidxml::node_element, "Real");
        //    realNode->append_attribute(doc_ptr->allocate_attribute("unit", "m/s2"));
        //    scalarVarNode->append_node(realNode);
        //    break;
        //default:
        //    break;  
        //}

    }

    rootNode->append_node(modelVarsNode);

    // Add ModelStructure node
    rapidxml::xml_node<>* modelStructNode = doc_ptr->allocate_node(rapidxml::node_element, "ModelStructure");
    // Add Outputs, Derivatives, and InitialUnknowns nodes and attributes here...
    rootNode->append_node(modelStructNode);

    // Save the XML document to a file
    std::ofstream outFile(path + "/modelDescriptionTEST.xml");
    outFile << *doc_ptr;
    outFile.close();

    delete doc_ptr;

}


            
//////////////// FMU FUNCTIONS /////////////////


fmi2Component fmi2Instantiate(fmi2String instanceName, fmi2Type fmuType, fmi2String fmuGUID, fmi2String fmuResourceLocation, const fmi2CallbackFunctions* functions, fmi2Boolean visible, fmi2Boolean loggingOn){
    ChFmuComponent* fmu = fmi2Instantiate_getPointer(instanceName, fmuType, fmuGUID);
    fmu->callbackFunctions = *functions;
    fmu->loggingOn = loggingOn;
    
    return reinterpret_cast<void*>(fmu);
}

void createModelDescription(const std::string& path){
    ChFmuComponent* fmu = fmi2Instantiate_getPointer("", fmi2Type::fmi2CoSimulation, "");
    fmu->ExportModelDescription(path);
    delete fmu;
}

const char* fmi2GetTypesPlatform(void){
    return "default";
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


// TODO: profile this piece of code
//fmi2Status fmi2GetReal(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, fmi2Real value[]){
//    auto& fmi2Real_map = reinterpret_cast<ChFmuComponent*>(c)->fmi2Real_map;
//    for (size_t s = 0; s<nvr; ++s){
//        auto it = fmi2Real_map.find(vr[s]);
//        if (it != fmi2Real_map.end())
//            value[s] = *it->second;
//        else return fmi2Status::fmi2Error;
//    }
//    return fmi2Status::fmi2OK;
//}

fmi2Status fmi2GetReal(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, fmi2Real value[]){
    auto& scalarVariables = reinterpret_cast<ChFmuComponent*>(c)->scalarVariables;

    for (size_t s = 0; s<nvr; ++s){
        double valref = vr[s];
        auto predicate_samevalreftype = [vr, s](const FmuScalarVariable& var) {
            return var.valueReference == vr[s];
            return var.type == FmuScalarVariable::FmuScalarVariableType::FMU_REAL;
        };
        auto it = std::find_if(scalarVariables.begin(), scalarVariables.end(), predicate_samevalreftype);
        if (it != scalarVariables.end())
            value[s] = *it->ptr.fmi2Real_ptr;
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
