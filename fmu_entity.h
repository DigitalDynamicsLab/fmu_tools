
#pragma once
#include "FmuToolsCommon.h"
#include "fmi2_headers/fmi2Functions.h"
#include <cassert>
#include <vector>
#include <array>
#include <fstream>

#include <map>
#include <iostream>
#include <string>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <limits>
#include "rapidxml.hpp"
#include "rapidxml_print.hpp"

#include "variant/variant.hpp"

extern const std::unordered_set<UnitDefinitionType, UnitDefinitionType::Hash> common_unitdefinitions;

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

        unitDefinitions["1"] = UnitDefinitionType("1"); // guarantee the existence of the default unit


    }

    virtual ~ChFmuComponent(){}


protected:
    std::string instanceName;
    fmi2String fmuGUID;

    static const std::set<std::string> logCategories_available;
    std::set<std::string> logCategories;


public:
    bool initializationMode = false;

    // DefaultExperiment
    double startTime = 0;
    double stopTime = 1;
    double stepSize = 1e-3;
    double tolerance = -1;
    double time = 0;

    const bool fmi2Type_CoSimulation_available;
    const bool fmi2Type_ModelExchange_available;

    const std::string modelIdentifier;

    fmi2Type fmuType;

    std::map<FmuVariable::Type, unsigned int> valueReferenceCounter;

    std::set<FmuVariable> scalarVariables;
    std::unordered_map<std::string, UnitDefinitionType> unitDefinitions;

    std::set<FmuVariable>::iterator ChFmuComponent::findByValrefType(fmi2ValueReference vr, FmuVariable::Type vartype);


    fmi2CallbackFunctions callbackFunctions;

    void AddCallbackLoggerCategory(std::string cat){
        if (logCategories_available.find(cat) == logCategories_available.end())
            throw std::runtime_error(std::string("Log category \"") + cat + std::string("\" is not valid."));
        logCategories.insert(cat);
    }

    bool loggingOn = true;

    virtual fmi2Status DoStep(double stepSize = -1){ return fmi2Status::fmi2Error; };

    void ExportModelDescription(std::string path){
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

            rootNode->append_node(unitNode);
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
            stringbuf.push_back(it->GetStartVal());
            unitNode->append_attribute(doc_ptr->allocate_attribute("start", stringbuf.back().c_str()));
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

protected:

    void AddUnitDefinition(const UnitDefinitionType& newunitdefinition){
        unitDefinitions[newunitdefinition.name] = newunitdefinition;
    }

    void ClearUnitDefinitions(){
        unitDefinitions.clear();
    }

    // DEV: unfortunately it is not possible to retrieve the fmi2 type based on the var_ptr only; the reason is that :
    // e.g. both fmi2Integer and fmi2Boolean are actually alias of type int, thus impeding any possible splitting depending on type
    // if we accept to have both fmi2Integer and fmi2Boolean considered as the same type we can drop the 'scalartype' argument
    // but the risk is that a variable might end up being flagged as Integer while it's actually a Boolean and it is not nice
    // At least, in this way, we do not have any redundant code at least
                //start_type startval = std::numeric_limits<decltype(*ptr_type)>>::quiet_NaN(),

    const FmuVariable& addFmuVariable(
            ptr_type var_ptr,
            std::string name,
            FmuVariable::Type scalartype = FmuVariable::Type::FMU_REAL,
            std::string unitname = "",
            std::string description = "",
            std::string causality = "",
            std::string variability = "",
            start_type startval = start_type(0.0),
            std::string initial = "")
    {

        // check if unit definition exists
        auto match_unit = unitDefinitions.find(unitname);
        if (match_unit == unitDefinitions.end()){
            auto predicate_samename = [unitname](const UnitDefinitionType& var) { return var.name == unitname; };
            auto match_commonunit = std::find_if(common_unitdefinitions.begin(), common_unitdefinitions.end(), predicate_samename);
            if (match_commonunit == common_unitdefinitions.end()){
                throw std::runtime_error("Variable unit is not registered within this ChFmuComponent. Call AddUnitDefinition first.");
            }
            else{
                AddUnitDefinition(*match_commonunit);
            }
        }


        // create new variable
        // check if same-name variable exists
        auto predicate_samename = [name](const FmuVariable& var) { return var.GetName() == name; };
        auto it = std::find_if(scalarVariables.begin(), scalarVariables.end(), predicate_samename);
        if (it!=scalarVariables.end())
            throw std::runtime_error("Cannot add two Fmu Variables with the same name.");


        FmuVariable newvar(name, scalartype);
        newvar.SetUnitName(unitname);
        newvar.SetValueReference(++valueReferenceCounter[scalartype]);
        newvar.SetPtr(var_ptr);
        newvar.SetDescription(description);
        newvar.SetCausalityVariabilityInitial(causality, variability, initial);
        newvar.SetStartVal(startval);


        std::pair<std::set<FmuVariable>::iterator, bool> ret = scalarVariables.insert(newvar);
        assert(ret.second && "Cannot insert new variable into FMU.");

        return *(ret.first);
    }

    
    void sendToLog(std::string msg, fmi2Status status, std::string msg_cat){
        if (logCategories_available.find(msg_cat) == logCategories_available.end())
            throw std::runtime_error(std::string("Log category \"") + msg_cat + std::string("\" is not valid."));

        if (logCategories.find(msg_cat) != logCategories.end()){
            callbackFunctions.logger(callbackFunctions.componentEnvironment, instanceName.c_str(), status, msg_cat.c_str(), msg.c_str());
        }
    }

};

ChFmuComponent* fmi2Instantiate_getPointer(fmi2String instanceName, fmi2Type fmuType, fmi2String fmuGUID);




