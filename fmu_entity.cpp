
#include "fmu_entity.h"
#include <cassert>
#include <vector>
#include <array>
#include <map>
#include <list>
#include <iostream>
#include <fstream>
#include "rapidxml_ext.hpp"


const std::unordered_set<UnitDefinitionType, UnitDefinitionType::Hash> common_unitdefinitions = {UD_kg, UD_m, UD_s, UD_A, UD_K, UD_mol, UD_cd, UD_rad, UD_m_s, UD_m_s2, UD_rad_s, UD_rad_s2};

const std::set<std::string> FmuComponent::logCategories_available = {
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
    FmuComponent* fmu = fmi2Instantiate_getPointer("", fmi2Type::fmi2CoSimulation, "");
    fmu->ExportModelDescription(path);
    delete fmu;
}

void FmuComponent::ExportModelDescription(std::string path){
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
    rootNode->append_attribute(doc_ptr->allocate_attribute("modelName", modelIdentifier.c_str())); // modelName can be anything else
    rootNode->append_attribute(doc_ptr->allocate_attribute("guid", "{16ce9076-4f15-4484-9e18-fefd58f15f51}"));
    rootNode->append_attribute(doc_ptr->allocate_attribute("generationTool", "fmu_generator_standalone"));
    rootNode->append_attribute(doc_ptr->allocate_attribute("variableNamingConvention", "structured"));
    rootNode->append_attribute(doc_ptr->allocate_attribute("numberOfEventIndicators", "0"));
    doc_ptr->append_node(rootNode);

    // Add CoSimulation node
    rapidxml::xml_node<>* coSimNode = doc_ptr->allocate_node(rapidxml::node_element, "CoSimulation");
    coSimNode->append_attribute(doc_ptr->allocate_attribute("modelIdentifier", modelIdentifier.c_str()));
    coSimNode->append_attribute(doc_ptr->allocate_attribute("canHandleVariableCommunicationStepSize", "true"));
    coSimNode->append_attribute(doc_ptr->allocate_attribute("canInterpolateInputs", "true"));
    coSimNode->append_attribute(doc_ptr->allocate_attribute("maxOutputDerivativeOrder", "1"));
    coSimNode->append_attribute(doc_ptr->allocate_attribute("canGetAndSetFMUstate", "false"));
    coSimNode->append_attribute(doc_ptr->allocate_attribute("canSerializeFMUstate", "false"));
    coSimNode->append_attribute(doc_ptr->allocate_attribute("providesDirectionalDerivative", "false"));
    rootNode->append_node(coSimNode);

    // Add UnitDefinitions node
    std::list<std::string> stringbuf;
    rapidxml::xml_node<>* unitDefsNode = doc_ptr->allocate_node(rapidxml::node_element, "UnitDefinitions");
    
    for (auto& ud_pair: unitDefinitions)
    {
        auto& ud = ud_pair.second;
        rapidxml::xml_node<>* unitNode = doc_ptr->allocate_node(rapidxml::node_element, "Unit");
        unitNode->append_attribute(doc_ptr->allocate_attribute("name", ud.name.c_str()));

        rapidxml::xml_node<>* baseUnitNode = doc_ptr->allocate_node(rapidxml::node_element, "BaseUnit");
        if (ud.kg != 0) {stringbuf.push_back(std::to_string(ud.kg)); baseUnitNode->append_attribute(doc_ptr->allocate_attribute("kg", stringbuf.back().c_str())); }
        if (ud.m != 0) {stringbuf.push_back(std::to_string(ud.m)); baseUnitNode->append_attribute(doc_ptr->allocate_attribute("m", stringbuf.back().c_str())); }
        if (ud.s != 0) {stringbuf.push_back(std::to_string(ud.s)); baseUnitNode->append_attribute(doc_ptr->allocate_attribute("s", stringbuf.back().c_str())); }
        if (ud.A != 0) {stringbuf.push_back(std::to_string(ud.A)); baseUnitNode->append_attribute(doc_ptr->allocate_attribute("A", stringbuf.back().c_str())); }
        if (ud.K != 0) {stringbuf.push_back(std::to_string(ud.K)); baseUnitNode->append_attribute(doc_ptr->allocate_attribute("K", stringbuf.back().c_str())); }
        if (ud.mol != 0) {stringbuf.push_back(std::to_string(ud.mol)); baseUnitNode->append_attribute(doc_ptr->allocate_attribute("mol", stringbuf.back().c_str())); }
        if (ud.cd != 0) {stringbuf.push_back(std::to_string(ud.cd)); baseUnitNode->append_attribute(doc_ptr->allocate_attribute("cd", stringbuf.back().c_str())); }
        if (ud.rad != 0) {stringbuf.push_back(std::to_string(ud.rad)); baseUnitNode->append_attribute(doc_ptr->allocate_attribute("rad", stringbuf.back().c_str())); }
        unitNode->append_node(baseUnitNode);

        unitDefsNode->append_node(unitNode);
    }
    rootNode->append_node(unitDefsNode);

    // Add LogCategories node
    rapidxml::xml_node<>* logCategoriesNode = doc_ptr->allocate_node(rapidxml::node_element, "LogCategories");
    for (auto& lc: logCategories){
        rapidxml::xml_node<>* logCategoryNode = doc_ptr->allocate_node(rapidxml::node_element, "Category");
        logCategoryNode->append_attribute(doc_ptr->allocate_attribute("name", lc.c_str()));
        logCategoriesNode->append_node(logCategoryNode);
    }
    rootNode->append_node(logCategoriesNode);

    // Add DefaultExperiment node
    std::string startTime_str = std::to_string(startTime);
    std::string stopTime_str = std::to_string(stopTime);
    std::string stepSize_str = std::to_string(stepSize);
    std::string tolerance_str = std::to_string(tolerance);
    rapidxml::xml_node<>* defaultExpNode = doc_ptr->allocate_node(rapidxml::node_element, "DefaultExperiment");
    defaultExpNode->append_attribute(doc_ptr->allocate_attribute("startTime", startTime_str.c_str()));
    defaultExpNode->append_attribute(doc_ptr->allocate_attribute("stopTime", stopTime_str.c_str()));
    if(stepSize>0) defaultExpNode->append_attribute(doc_ptr->allocate_attribute("stepSize", stepSize_str.c_str()));
    if(tolerance>0) defaultExpNode->append_attribute(doc_ptr->allocate_attribute("tolerance", tolerance_str.c_str()));
    rootNode->append_node(defaultExpNode);

    // Add ModelVariables node
    rapidxml::xml_node<>* modelVarsNode = doc_ptr->allocate_node(rapidxml::node_element, "ModelVariables");

    // WARNING: rapidxml does not copy the strings that we pass to print, but it just keeps the addresses until it's time to print them
    // so we cannot use a temporary string to convert the number to string and then recycle it
    // we cannot use std::vector because, in case of reallocation, it might move the array somewhere else thus invalidating the addresses
    std::list<std::string> valueref_str;

    //TODO: move elsewhere
    const std::unordered_map<FmuVariable::Type, std::string> Type_strings = {
        {FmuVariable::Type::FMU_REAL, "Real"},
        {FmuVariable::Type::FMU_INTEGER, "Integer"},
        {FmuVariable::Type::FMU_BOOLEAN, "Boolean"},
        {FmuVariable::Type::FMU_UNKNOWN, "Unknown"},
        {FmuVariable::Type::FMU_STRING, "String"}
    };


    // TODO: std::set::iterator breaks unions!!!
    for (std::set<FmuVariable>::const_iterator it = scalarVariables.begin(); it!=scalarVariables.end(); ++it){
        // Create a ScalarVariable node
        rapidxml::xml_node<>* scalarVarNode = doc_ptr->allocate_node(rapidxml::node_element, "ScalarVariable");
        scalarVarNode->append_attribute(doc_ptr->allocate_attribute("name", it->GetName().c_str()));

        valueref_str.push_back(std::to_string(it->GetValueReference()));
        scalarVarNode->append_attribute(doc_ptr->allocate_attribute("valueReference", valueref_str.back().c_str()));

        if (!it->GetDescription().empty()) scalarVarNode->append_attribute(doc_ptr->allocate_attribute("description", it->GetDescription().c_str()));
        if (!it->GetCausality().empty())   scalarVarNode->append_attribute(doc_ptr->allocate_attribute("causality",   it->GetCausality().c_str()));
        if (!it->GetVariability().empty()) scalarVarNode->append_attribute(doc_ptr->allocate_attribute("variability", it->GetVariability().c_str()));
        if (!it->GetInitial().empty())     scalarVarNode->append_attribute(doc_ptr->allocate_attribute("initial",     it->GetInitial().c_str()));
        modelVarsNode->append_node(scalarVarNode);

        rapidxml::xml_node<>* unitNode = doc_ptr->allocate_node(rapidxml::node_element, Type_strings.at(it->GetType()).c_str());
        unitNode->append_attribute(doc_ptr->allocate_attribute("unit", it->GetUnitName().c_str()));
        if (it->HasStartVal()){
            stringbuf.push_back(it->GetStartVal());
            unitNode->append_attribute(doc_ptr->allocate_attribute("start", stringbuf.back().c_str()));
        }
        scalarVarNode->append_node(unitNode);       

    }

    rootNode->append_node(modelVarsNode);

    // Add ModelStructure node
    rapidxml::xml_node<>* modelStructNode = doc_ptr->allocate_node(rapidxml::node_element, "ModelStructure");
    // Add Outputs, Derivatives, and InitialUnknowns nodes and attributes here...
    rootNode->append_node(modelStructNode);

    // Save the XML document to a file
    std::ofstream outFile(path + "/modelDescription.xml");
    outFile << *doc_ptr;
    outFile.close();

    delete doc_ptr;
}

void FmuComponent::CheckVariables() const
{
    for (auto& sv: scalarVariables){

    }
}
            
//////////////// FMU FUNCTIONS /////////////////

fmi2Component fmi2Instantiate(fmi2String instanceName, fmi2Type fmuType, fmi2String fmuGUID, fmi2String fmuResourceLocation, const fmi2CallbackFunctions* functions, fmi2Boolean visible, fmi2Boolean loggingOn){
    FmuComponent* fmu_ptr = fmi2Instantiate_getPointer(instanceName, fmuType, fmuGUID);
    fmu_ptr->SetCallbackFunctions(functions);
    fmu_ptr->SetLogging(loggingOn==fmi2True ? true : false);    
    return reinterpret_cast<void*>(fmu_ptr);
}

const char* fmi2GetTypesPlatform(void){
    return fmi2TypesPlatform;
}

const char* fmi2GetVersion(void){
    return fmi2Version;
}

fmi2Status fmi2SetDebugLogging(fmi2Component c, fmi2Boolean loggingOn, size_t nCategories, const fmi2String categories[]){
    FmuComponent* fmu_ptr = reinterpret_cast<FmuComponent*>(c);
    fmu_ptr->SetLogging(loggingOn==fmi2True ? true : false);
    for (auto cs = 0; cs < nCategories; ++cs){
        fmu_ptr->AddCallbackLoggerCategory(categories[cs]);
    }

    return fmi2Status::fmi2OK;
}


void fmi2FreeInstance(fmi2Component c){
    delete reinterpret_cast<FmuComponent*>(c);
}

fmi2Status fmi2SetupExperiment(fmi2Component c, fmi2Boolean toleranceDefined, fmi2Real tolerance, fmi2Real startTime, fmi2Boolean stopTimeDefined, fmi2Real stopTime){
    assert(toleranceDefined==fmi2False);
    assert(stopTimeDefined==fmi2False);
    reinterpret_cast<FmuComponent*>(c)->SetDefaultExperiment(toleranceDefined, tolerance, startTime, stopTimeDefined, stopTime);
    return fmi2Status::fmi2OK;
}

fmi2Status fmi2EnterInitializationMode(fmi2Component c){
    reinterpret_cast<FmuComponent*>(c)->SetInitializationMode(true);
    return fmi2Status::fmi2OK;
}

fmi2Status fmi2ExitInitializationMode(fmi2Component c){ 
    reinterpret_cast<FmuComponent*>(c)->SetInitializationMode(false);
    return fmi2Status::fmi2OK;
}
fmi2Status fmi2Terminate(fmi2Component c){ return fmi2Status::fmi2OK; }
fmi2Status fmi2Reset(fmi2Component c){ return fmi2Status::fmi2OK; }

std::set<FmuVariable>::iterator FmuComponent::findByValrefType(fmi2ValueReference vr, FmuVariable::Type vartype){
    auto predicate_samevalreftype = [vr, vartype](const FmuVariable& var) {
        return var.GetValueReference() == vr;
        return var.GetType() == vartype;
    };
    return std::find_if(scalarVariables.begin(), scalarVariables.end(), predicate_samevalreftype);
}


fmi2Status fmi2GetReal(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, fmi2Real value[]){
    return reinterpret_cast<FmuComponent*>(c)->fmi2GetVariable(vr, nvr, value, FmuVariable::Type::FMU_REAL);
}

fmi2Status fmi2GetInteger(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, fmi2Integer value[]){
    return reinterpret_cast<FmuComponent*>(c)->fmi2GetVariable(vr, nvr, value, FmuVariable::Type::FMU_INTEGER);
}

fmi2Status fmi2GetBoolean(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, fmi2Boolean value[]){
    return reinterpret_cast<FmuComponent*>(c)->fmi2GetVariable(vr, nvr, value, FmuVariable::Type::FMU_BOOLEAN);
}

fmi2Status fmi2GetString(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, fmi2String value[]){
    return reinterpret_cast<FmuComponent*>(c)->fmi2GetVariable(vr, nvr, value, FmuVariable::Type::FMU_STRING);
}



fmi2Status fmi2SetReal(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, const fmi2Real value[]){
    return reinterpret_cast<FmuComponent*>(c)->fmi2SetVariable(vr, nvr, value, FmuVariable::Type::FMU_REAL);
}

fmi2Status fmi2SetInteger(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, const fmi2Integer value[]){
    return reinterpret_cast<FmuComponent*>(c)->fmi2SetVariable(vr, nvr, value, FmuVariable::Type::FMU_INTEGER);
}

fmi2Status fmi2SetBoolean(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, const fmi2Boolean value[]){
    return reinterpret_cast<FmuComponent*>(c)->fmi2SetVariable(vr, nvr, value, FmuVariable::Type::FMU_BOOLEAN);
}

fmi2Status fmi2SetString(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, const fmi2String value[]){
    return reinterpret_cast<FmuComponent*>(c)->fmi2SetVariable(vr, nvr, value, FmuVariable::Type::FMU_STRING);
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
        double candidateStepSize = currentCommunicationPoint + communicationStepSize - reinterpret_cast<FmuComponent*>(c)->GetTime();
        if (candidateStepSize < -1e-10)
            return fmi2Status::fmi2Warning;
        else
        {
            if (candidateStepSize < 1e-10)
                break;

            fmi2DoStep_status = reinterpret_cast<FmuComponent*>(c)->DoStep(candidateStepSize);
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
