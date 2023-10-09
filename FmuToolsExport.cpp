
#include "FmuToolsExport.h"
#include <cassert>
#include <vector>
#include <array>
#include <map>
#include <list>
#include <iostream>
#include <fstream>
#include "rapidxml_ext.hpp"


const std::unordered_set<UnitDefinitionType, UnitDefinitionType::Hash> common_unitdefinitions = {UD_kg, UD_m, UD_s, UD_A, UD_K, UD_mol, UD_cd, UD_rad, UD_m_s, UD_m_s2, UD_rad_s, UD_rad_s2};

const std::set<std::string> FmuComponentBase::logCategories_available = {
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

bool is_pointer_variant(const FmuVariableBindType& myVariant) {
    return varns::visit([](auto&& arg) -> bool {
        return std::is_pointer_v<std::decay_t<decltype(arg)>>;
    }, myVariant);
}


void createModelDescription(const std::string& path, fmi2Type fmutype, fmi2String guid){
    FmuComponentBase* fmu = fmi2Instantiate_getPointer("", fmi2Type::fmi2CoSimulation, guid);
    fmu->ExportModelDescription(path);
    delete fmu;
}

void FmuComponentBase::ExportModelDescription(std::string path){

    _preModelDescriptionExport();

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
    rootNode->append_attribute(doc_ptr->allocate_attribute("fmiVersion", fmi2GetVersion()));
    rootNode->append_attribute(doc_ptr->allocate_attribute("modelName", modelIdentifier.c_str())); // modelName can be anything else
    rootNode->append_attribute(doc_ptr->allocate_attribute("guid", fmuGUID.c_str()));
    rootNode->append_attribute(doc_ptr->allocate_attribute("generationTool", "rapidxml"));
    rootNode->append_attribute(doc_ptr->allocate_attribute("variableNamingConvention", "structured"));
    rootNode->append_attribute(doc_ptr->allocate_attribute("numberOfEventIndicators", "0"));
    doc_ptr->append_node(rootNode);

    // Add CoSimulation node
    if (is_cosimulation_available()){
        rapidxml::xml_node<>* coSimNode = doc_ptr->allocate_node(rapidxml::node_element, "CoSimulation");
        coSimNode->append_attribute(doc_ptr->allocate_attribute("modelIdentifier", modelIdentifier.c_str()));
        coSimNode->append_attribute(doc_ptr->allocate_attribute("canHandleVariableCommunicationStepSize", "true"));
        coSimNode->append_attribute(doc_ptr->allocate_attribute("canInterpolateInputs", "true"));
        coSimNode->append_attribute(doc_ptr->allocate_attribute("maxOutputDerivativeOrder", "1"));
        coSimNode->append_attribute(doc_ptr->allocate_attribute("canGetAndSetFMUstate", "false"));
        coSimNode->append_attribute(doc_ptr->allocate_attribute("canSerializeFMUstate", "false"));
        coSimNode->append_attribute(doc_ptr->allocate_attribute("providesDirectionalDerivative", "false"));
        rootNode->append_node(coSimNode);
    }

    // Add ModelExchange node
    if (is_modelexchange_available()){
        rapidxml::xml_node<>* modExNode = doc_ptr->allocate_node(rapidxml::node_element, "ModelExchange");
        rootNode->append_node(modExNode);
        throw std::runtime_error("Developer error: ModelExchange not supported in modelDescription.xml");
    }

    // Add UnitDefinitions node
    std::list<std::string> stringbuf;
    rapidxml::xml_node<>* unitDefsNode = doc_ptr->allocate_node(rapidxml::node_element, "UnitDefinitions");
    
    for (auto& ud_pair: unitDefinitions)
    {
        auto& ud = ud_pair.second;
        rapidxml::xml_node<>* unitNode = doc_ptr->allocate_node(rapidxml::node_element, "Unit");
        unitNode->append_attribute(doc_ptr->allocate_attribute("name", ud.name.c_str()));

        rapidxml::xml_node<>* baseUnitNode = doc_ptr->allocate_node(rapidxml::node_element, "BaseUnit");
        if (ud.kg != 0)  {stringbuf.push_back(std::to_string(ud.kg)); baseUnitNode->append_attribute(doc_ptr->allocate_attribute("kg", stringbuf.back().c_str())); }
        if (ud.m != 0)   {stringbuf.push_back(std::to_string(ud.m)); baseUnitNode->append_attribute(doc_ptr->allocate_attribute("m", stringbuf.back().c_str())); }
        if (ud.s != 0)   {stringbuf.push_back(std::to_string(ud.s)); baseUnitNode->append_attribute(doc_ptr->allocate_attribute("s", stringbuf.back().c_str())); }
        if (ud.A != 0)   {stringbuf.push_back(std::to_string(ud.A)); baseUnitNode->append_attribute(doc_ptr->allocate_attribute("A", stringbuf.back().c_str())); }
        if (ud.K != 0)   {stringbuf.push_back(std::to_string(ud.K)); baseUnitNode->append_attribute(doc_ptr->allocate_attribute("K", stringbuf.back().c_str())); }
        if (ud.mol != 0) {stringbuf.push_back(std::to_string(ud.mol)); baseUnitNode->append_attribute(doc_ptr->allocate_attribute("mol", stringbuf.back().c_str())); }
        if (ud.cd != 0)  {stringbuf.push_back(std::to_string(ud.cd)); baseUnitNode->append_attribute(doc_ptr->allocate_attribute("cd", stringbuf.back().c_str())); }
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
        {FmuVariable::Type::Real, "Real"},
        {FmuVariable::Type::Integer, "Integer"},
        {FmuVariable::Type::Boolean, "Boolean"},
        {FmuVariable::Type::Unknown, "Unknown"},
        {FmuVariable::Type::String, "String"}
    };

    const std::unordered_map<FmuVariable::InitialType, std::string> InitialType_strings = {
        {FmuVariable::InitialType::exact, "exact"},
        {FmuVariable::InitialType::approx, "approx"},
        {FmuVariable::InitialType::calculated, "calculated"}
    };

    const std::unordered_map<FmuVariable::VariabilityType, std::string> VariabilityType_strings = {
        {FmuVariable::VariabilityType::constant, "constant"},
        {FmuVariable::VariabilityType::fixed, "fixed"},
        {FmuVariable::VariabilityType::tunable, "tunable"},
        {FmuVariable::VariabilityType::discrete, "discrete"},
        {FmuVariable::VariabilityType::continuous, "continuous"}
    };

    const std::unordered_map<FmuVariable::CausalityType, std::string> CausalityType_strings = {
        {FmuVariable::CausalityType::parameter, "parameter"},
        {FmuVariable::CausalityType::calculatedParameter, "calculatedParameter"},
        {FmuVariable::CausalityType::input, "input"},
        {FmuVariable::CausalityType::output, "output"},
        {FmuVariable::CausalityType::local, "local"},
        {FmuVariable::CausalityType::independent, "independent"}
    };


    for (std::set<FmuVariableExport>::const_iterator it = scalarVariables.begin(); it!=scalarVariables.end(); ++it){
        // Create a ScalarVariable node
        rapidxml::xml_node<>* scalarVarNode = doc_ptr->allocate_node(rapidxml::node_element, "ScalarVariable");
        scalarVarNode->append_attribute(doc_ptr->allocate_attribute("name", it->GetName().c_str()));

        valueref_str.push_back(std::to_string(it->GetValueReference()));
        scalarVarNode->append_attribute(doc_ptr->allocate_attribute("valueReference", valueref_str.back().c_str()));

        if (!it->GetDescription().empty()) scalarVarNode->append_attribute(doc_ptr->allocate_attribute("description", it->GetDescription().c_str()));
        if (it->GetCausality() != FmuVariable::CausalityType::local)          scalarVarNode->append_attribute(doc_ptr->allocate_attribute("causality",   CausalityType_strings.at(it->GetCausality()).c_str()));
        if (it->GetVariability() != FmuVariable::VariabilityType::continuous) scalarVarNode->append_attribute(doc_ptr->allocate_attribute("variability", VariabilityType_strings.at(it->GetVariability()).c_str()));
        if (it->GetInitial() != FmuVariable::InitialType::none)               scalarVarNode->append_attribute(doc_ptr->allocate_attribute("initial",     InitialType_strings.at(it->GetInitial()).c_str()));
        modelVarsNode->append_node(scalarVarNode);

        rapidxml::xml_node<>* unitNode = doc_ptr->allocate_node(rapidxml::node_element, Type_strings.at(it->GetType()).c_str());
        if (it->GetType() == FmuVariable::Type::Real && !it->GetUnitName().empty())
            unitNode->append_attribute(doc_ptr->allocate_attribute("unit", it->GetUnitName().c_str()));
        if (it->HasStartVal()){
            stringbuf.push_back(it->GetStartVal_toString());
            unitNode->append_attribute(doc_ptr->allocate_attribute("start", stringbuf.back().c_str()));
        }
        scalarVarNode->append_node(unitNode);       

    }

    rootNode->append_node(modelVarsNode);

    // Add ModelStructure node
    rapidxml::xml_node<>* modelStructNode = doc_ptr->allocate_node(rapidxml::node_element, "ModelStructure");
    // TODO: Add Outputs, Derivatives, and InitialUnknowns nodes and attributes here...
    rootNode->append_node(modelStructNode);

    // Save the XML document to a file
    std::ofstream outFile(path + "/modelDescription.xml");
    outFile << *doc_ptr;
    outFile.close();

    delete doc_ptr;

     _postModelDescriptionExport();
}

std::set<FmuVariableExport>::iterator FmuComponentBase::findByValrefType(fmi2ValueReference vr, FmuVariable::Type vartype){
    auto predicate_samevalreftype = [vr, vartype](const FmuVariable& var) {
        return var.GetValueReference() == vr && var.GetType() == vartype;
    };
    return std::find_if(scalarVariables.begin(), scalarVariables.end(), predicate_samevalreftype);
}

std::set<FmuVariableExport>::iterator FmuComponentBase::findByName(const std::string& name){
    auto predicate_samename = [name](const FmuVariable& var) {
        return !var.GetName().compare(name);
    };
    return std::find_if(scalarVariables.begin(), scalarVariables.end(), predicate_samename);
}
            
//////////////// FMU FUNCTIONS /////////////////

fmi2Component fmi2Instantiate(fmi2String instanceName, fmi2Type fmuType, fmi2String fmuGUID, fmi2String fmuResourceLocation, const fmi2CallbackFunctions* functions, fmi2Boolean visible, fmi2Boolean loggingOn){
    FmuComponentBase* fmu_ptr = fmi2Instantiate_getPointer(instanceName, fmuType, fmuGUID);
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
    FmuComponentBase* fmu_ptr = reinterpret_cast<FmuComponentBase*>(c);
    fmu_ptr->SetLogging(loggingOn==fmi2True ? true : false);
    for (auto cs = 0; cs < nCategories; ++cs){
        fmu_ptr->AddCallbackLoggerCategory(categories[cs]);
    }

    return fmi2Status::fmi2OK;
}


void fmi2FreeInstance(fmi2Component c){
    delete reinterpret_cast<FmuComponentBase*>(c);
}

fmi2Status fmi2SetupExperiment(fmi2Component c, fmi2Boolean toleranceDefined, fmi2Real tolerance, fmi2Real startTime, fmi2Boolean stopTimeDefined, fmi2Real stopTime){
    assert(toleranceDefined==fmi2False);
    assert(stopTimeDefined==fmi2False);
    reinterpret_cast<FmuComponentBase*>(c)->SetDefaultExperiment(toleranceDefined, tolerance, startTime, stopTimeDefined, stopTime);
    return fmi2Status::fmi2OK;
}

fmi2Status fmi2EnterInitializationMode(fmi2Component c){
    reinterpret_cast<FmuComponentBase*>(c)->EnterInitializationMode();
    return fmi2Status::fmi2OK;
}

fmi2Status fmi2ExitInitializationMode(fmi2Component c){ 
    reinterpret_cast<FmuComponentBase*>(c)->ExitInitializationMode();
    return fmi2Status::fmi2OK;
}
fmi2Status fmi2Terminate(fmi2Component c){ return fmi2Status::fmi2OK; }
fmi2Status fmi2Reset(fmi2Component c){ return fmi2Status::fmi2OK; }


fmi2Status fmi2GetReal(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, fmi2Real value[]){
    return reinterpret_cast<FmuComponentBase*>(c)->fmi2GetVariable(vr, nvr, value, FmuVariable::Type::Real);
}

fmi2Status fmi2GetInteger(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, fmi2Integer value[]){
    return reinterpret_cast<FmuComponentBase*>(c)->fmi2GetVariable(vr, nvr, value, FmuVariable::Type::Integer);
}

fmi2Status fmi2GetBoolean(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, fmi2Boolean value[]){
    return reinterpret_cast<FmuComponentBase*>(c)->fmi2GetVariable(vr, nvr, value, FmuVariable::Type::Boolean);
}

fmi2Status fmi2GetString(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, fmi2String value[]){
    return reinterpret_cast<FmuComponentBase*>(c)->fmi2GetVariable(vr, nvr, value, FmuVariable::Type::String);
}



fmi2Status fmi2SetReal(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, const fmi2Real value[]){
    return reinterpret_cast<FmuComponentBase*>(c)->fmi2SetVariable(vr, nvr, value, FmuVariable::Type::Real);
}

fmi2Status fmi2SetInteger(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, const fmi2Integer value[]){
    return reinterpret_cast<FmuComponentBase*>(c)->fmi2SetVariable(vr, nvr, value, FmuVariable::Type::Integer);
}

fmi2Status fmi2SetBoolean(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, const fmi2Boolean value[]){
    return reinterpret_cast<FmuComponentBase*>(c)->fmi2SetVariable(vr, nvr, value, FmuVariable::Type::Boolean);
}

fmi2Status fmi2SetString(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, const fmi2String value[]){
    return reinterpret_cast<FmuComponentBase*>(c)->fmi2SetVariable(vr, nvr, value, FmuVariable::Type::String);
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
    
    return reinterpret_cast<FmuComponentBase*>(c)->DoStep(currentCommunicationPoint, communicationStepSize, noSetFMUStatePriorToCurrentPoint);

}

fmi2Status fmi2CancelStep(fmi2Component c){ return fmi2Status::fmi2OK; }
fmi2Status fmi2GetStatus(fmi2Component c, const fmi2StatusKind s, fmi2Status* value){ return fmi2Status::fmi2OK; }
fmi2Status fmi2GetRealStatus(fmi2Component c, const fmi2StatusKind s, fmi2Real* value){ return fmi2Status::fmi2OK; }
fmi2Status fmi2GetIntegerStatus(fmi2Component c, const fmi2StatusKind s, fmi2Integer* value){ return fmi2Status::fmi2OK; }
fmi2Status fmi2GetBooleanStatus(fmi2Component c, const fmi2StatusKind s, fmi2Boolean* value){ return fmi2Status::fmi2OK; }
fmi2Status fmi2GetStringStatus(fmi2Component c, const fmi2StatusKind s, fmi2String* value){ return fmi2Status::fmi2OK; }